#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "FGPassengerSeat.h"
#include "Components/SceneComponent.h"
#include "Components/MeshComponent.h"
#include "FGInventoryComponent.h"
#include "UObject/UObjectGlobals.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "TsabsPassengerAnchor.h"
#include <limits>
#include "InputMappingContext.h"
#include "InputAction.h"
#include "EnhancedInputComponent.h"
#include "UObject/UnrealType.h"
#include "Engine/World.h"
#include "TsabsPassengerRailcar.h"
#include "FGRailroadVehicleMovementComponent.h"
#include <bit>
#include <type_traits>
#include "FGCharacterPlayer.h"
#include "FGFreightWagon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsInvisibleAnchorTest,
    "Tsabs.Passenger.Anchor.InvisibleStructure",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTsabsInvisibleAnchorTest::RunTest(const FString& Parameters)
{
    UClass* AnchorClass = FindObject<UClass>(nullptr, TEXT("/Script/TsabsPassengerWagon.TsabsPassengerAnchor"));
    if (!TestNotNull(TEXT("Own invisible passenger anchor class exists"), AnchorClass))
    {
        return false;
    }
    TestTrue(TEXT("Anchor inherits passenger possession, not a rail vehicle"),
        AnchorClass->IsChildOf(AFGPassengerSeat::StaticClass()));
    AFGPassengerSeat* Anchor = Cast<AFGPassengerSeat>(AnchorClass->GetDefaultObject());
    if (!TestNotNull(TEXT("Passenger anchor default object"), Anchor)) return false;
    TestNotNull(TEXT("Invisible scene root exists"), Anchor->GetRootComponent());
    TestNull(TEXT("No chair, interior or passenger mesh"), Anchor->FindComponentByClass<UMeshComponent>());
    TestNull(TEXT("No passenger inventory"), Anchor->FindComponentByClass<UFGInventoryComponent>());
    TestTrue(TEXT("Native driver attachment is enabled"), Anchor->ShouldAttachDriver());
    TestFalse(TEXT("Native driver remains invisible"), Anchor->IsDriverVisible());
    TestTrue(TEXT("Anchor replicates"), Anchor->GetIsReplicated());
    TestFalse(TEXT("Anchor has no actor collision"), Anchor->GetActorEnableCollision());
    TestTrue(TEXT("Never auto-possess a player"), Anchor->AutoPossessPlayer == EAutoReceiveInput::Disabled);
    TestTrue(TEXT("Never auto-spawn an AI controller"), Anchor->AutoPossessAI == EAutoPossessAI::Disabled);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsAnchorCameraTest,
    "Tsabs.Passenger.Anchor.ExteriorOrbitCamera",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTsabsAnchorCameraTest::RunTest(const FString& Parameters)
{
    ATsabsPassengerAnchor* Anchor = GetMutableDefault<ATsabsPassengerAnchor>();
    USpringArmComponent* Arm = Anchor->FindComponentByClass<USpringArmComponent>();
    UCameraComponent* Camera = Anchor->FindComponentByClass<UCameraComponent>();
    if (!TestNotNull(TEXT("Exterior orbit arm exists"), Arm) |
        !TestNotNull(TEXT("Own exterior camera exists"), Camera)) return false;
    TestTrue(TEXT("Arm orbits the invisible root"), Arm->GetAttachParent() == Anchor->GetRootComponent());
    TestTrue(TEXT("Camera follows the arm socket"), Camera->GetAttachParent() == Arm);
    TestTrue(TEXT("Exterior arm has positive reach"), Arm->TargetArmLength > 0.0f);
    TestTrue(TEXT("Camera tests world obstruction"), Arm->bDoCollisionTest);
    TestFalse(TEXT("Orbit is not the possessed character's view"), Arm->bUsePawnControlRotation);
    UFunction* Orbit = Anchor->FindFunction(TEXT("ApplyOrbitInput"));
    if (!TestNotNull(TEXT("Local look input updates the orbit"), Orbit)) return false;
    const FRotator Original = Arm->GetRelativeRotation();
    struct { FVector2D Delta; } Input{FVector2D(20.0, 10.0)};
    Anchor->ProcessEvent(Orbit, &Input);
    TestFalse(TEXT("Look changes yaw"), FMath::IsNearlyEqual(Arm->GetRelativeRotation().Yaw, Original.Yaw));
    TestFalse(TEXT("Look changes pitch"), FMath::IsNearlyEqual(Arm->GetRelativeRotation().Pitch, Original.Pitch));
    Input.Delta = FVector2D(0.0, 10000.0);
    Anchor->ProcessEvent(Orbit, &Input);
    TestTrue(TEXT("Orbit pitch does not flip"), FMath::Abs(Arm->GetRelativeRotation().Pitch) < 90.0);
    const FRotator BeforeInvalid = Arm->GetRelativeRotation();
    Input.Delta = FVector2D(std::numeric_limits<double>::quiet_NaN(), 0.0);
    Anchor->ProcessEvent(Orbit, &Input);
    TestTrue(TEXT("Invalid input cannot corrupt camera"), Arm->GetRelativeRotation().Equals(BeforeInvalid));
    FMinimalViewInfo View;
    View.FOV = 113.0f;
    Anchor->CalcCamera(0.0f, View);
    TestEqual(TEXT("Camera keeps the incoming player FOV like native driveable pawns"), View.FOV, 113.0f);
    TestTrue(TEXT("CalcCamera uses the own camera location"), View.Location.Equals(Camera->GetComponentLocation()));
    TestTrue(TEXT("CalcCamera uses the own camera rotation"), View.Rotation.Equals(Camera->GetComponentRotation()));
    Arm->SetRelativeRotation(Original);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsAnchorInputTest,
    "Tsabs.Passenger.Anchor.LookAndLeaveOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTsabsAnchorInputTest::RunTest(const FString& Parameters)
{
    ATsabsPassengerAnchor* Anchor = GetMutableDefault<ATsabsPassengerAnchor>();
    const UInputAction* Look = LoadObject<UInputAction>(nullptr,
        TEXT("/Game/FactoryGame/Inputs/Player/Movement/IA_LookAxis.IA_LookAxis"));
    const UInputAction* Leave = LoadObject<UInputAction>(nullptr,
        TEXT("/Game/FactoryGame/Inputs/Vehicles/Actions/IA_Vehicles_Leave.IA_Vehicles_Leave"));
    if (!TestNotNull(TEXT("Original look action resolves"), Look) |
        !TestNotNull(TEXT("Original leave action resolves"), Leave)) return false;
    const UInputMappingContext* Mapping = Anchor->mMappingContext;
    if (!TestNotNull(TEXT("Own filtered mapping exists"), Mapping)) return false;
    TestTrue(TEXT("Mapping belongs to the anchor, not MC_Trains"), Mapping->GetOuter() == Anchor);
    bool HasLook = false;
    bool HasLeave = false;
    for (const FEnhancedActionKeyMapping& Key : Mapping->GetMappings())
    {
        TestTrue(TEXT("Every mapped key is only look or leave"), Key.Action == Look || Key.Action == Leave);
        HasLook |= Key.Action == Look;
        HasLeave |= Key.Action == Leave;
    }
    TestTrue(TEXT("Look has a key mapping"), HasLook);
    TestTrue(TEXT("Leave has a key mapping"), HasLeave);
    UEnhancedInputComponent* Input = NewObject<UEnhancedInputComponent>(Anchor);
    Anchor->AddInputBindings(Input);
    TestEqual(TEXT("Exactly two event bindings; no throttle/brake/horn/inventory"), Input->GetActionEventBindings().Num(), 2);
    bool LookBound = false;
    bool LeaveBound = false;
    for (const auto& Binding : Input->GetActionEventBindings())
    {
        TestTrue(TEXT("Every event is only look or leave"), Binding->GetAction() == Look || Binding->GetAction() == Leave);
        LookBound |= Binding->GetAction() == Look && Binding->GetTriggerEvent() == ETriggerEvent::Triggered;
        LeaveBound |= Binding->GetAction() == Leave && Binding->GetTriggerEvent() == ETriggerEvent::Started;
    }
    TestTrue(TEXT("Look event is bound"), LookBound);
    TestTrue(TEXT("Exit event is bound once per press"), LeaveBound);
    return true;
}
// Transition fixture calls real legacy policy for RED, structural getter for GREEN.
template<class T>
static bool NativeFirstAttachedCar(T* Anchor, ATsabsPassengerRailcar* Car)
{
    if constexpr (requires { Anchor->GetPassengerRailcar(); }) return Anchor->GetPassengerRailcar() == Car;
    else return Anchor->IsWithinPassengerAccessSpeedLimit();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsAnchorAttachmentTest,
    "Tsabs.Passenger.NativeFirst.Anchor.StructuralAttachmentIndependentOfSpeed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTsabsAnchorAttachmentTest::RunTest(const FString& Parameters)
{
    UFunction* Initialize = ATsabsPassengerAnchor::StaticClass()->FindFunctionByName(TEXT("InitializePassengerAnchor"));
    UFunction* SpeedAccess = ATsabsPassengerAnchor::StaticClass()->FindFunctionByName(TEXT("IsWithinPassengerAccessSpeedLimit"));
    UFunction* Reset = ATsabsPassengerAnchor::StaticClass()->FindFunctionByName(TEXT("ResetPassengerAnchor"));
    if (!TestNotNull(TEXT("Server initialization API exists"), Initialize) |
        !TestNotNull(TEXT("Empty anchor cleanup API exists"), Reset)) return false;
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    if (!TestNotNull(TEXT("Isolated editor world exists, not a game session"), World)) return false;
    ATsabsPassengerAnchor* Anchor = World->SpawnActor<ATsabsPassengerAnchor>();
    ATsabsPassengerRailcar* Car = World->SpawnActor<ATsabsPassengerRailcar>();
    if (!TestNotNull(TEXT("Real own anchor instance"), Anchor) |
        !TestNotNull(TEXT("Real own car instance"), Car))
    {
        World->DestroyWorld(false);
        return false;
    }
    // Direct C++ calls: AActor::ProcessEvent intentionally ignores uninitialized editor worlds.
    TestNull(TEXT("Old custom speed-policy API is no longer registered"), SpeedAccess);
    auto QuerySpeedAccess = [&]() { return NativeFirstAttachedCar(Anchor, Car); };
    auto Init = [&](ATsabsPassengerRailcar* Vehicle, const FTransform& RelativeTransform)
    {
        return Anchor->InitializePassengerAnchor(Vehicle, RelativeTransform);
    };
    TestFalse(TEXT("Missing outer car is never a valid passenger attachment"), QuerySpeedAccess());
    TestFalse(TEXT("Null outer car cannot initialize"), Init(nullptr, FTransform::Identity));
    FTransform Invalid = FTransform::Identity;
    Invalid.SetTranslation(FVector(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0));
    TestFalse(TEXT("Invalid transform cannot initialize"), Init(Car, Invalid));
    TestNull(TEXT("Invalid initialization has no attachment side effects"), Anchor->GetAttachParentActor());
    const FTransform Relative(FRotator(0.0, 15.0, 0.0), FVector(25.0, 50.0, 75.0));
    TestTrue(TEXT("Authority initializes its own car attachment"), Init(Car, Relative));
    TestTrue(TEXT("Native replicated outer is own car"), Anchor->GetOuterVehicle() == Car);
    TestTrue(TEXT("Anchor attaches to own car"), Anchor->GetAttachParentActor() == Car);
    TestTrue(TEXT("Requested relative transform is retained"), Anchor->GetRootComponent()->GetRelativeTransform().Equals(Relative));
    ATsabsPassengerRailcar* OtherCar = World->SpawnActor<ATsabsPassengerRailcar>();
    if (TestNotNull(TEXT("Second own car for reassignment guard"), OtherCar))
    {
        TestFalse(TEXT("Cannot silently move initialized anchor to another car"), Init(OtherCar, Relative));
        TestTrue(TEXT("Rejected reassignment preserves original attachment"), Anchor->GetOuterVehicle() == Car && Anchor->GetAttachParentActor() == Car);
    }
    Anchor->mOuterVehicle = nullptr;
    TestFalse(TEXT("Missing replicated outer fails closed even while attached"), QuerySpeedAccess());
    Anchor->mOuterVehicle = GetMutableDefault<AFGFreightWagon>();
    TestFalse(TEXT("A freight reference is not an authorized passenger railcar"), QuerySpeedAccess());
    Anchor->mOuterVehicle = Car;
    UFGRailroadVehicleMovementComponent* Movement = Car->GetRailroadVehicleMovementComponent();
    if (TestNotNull(TEXT("Real speed-bearing movement component"), Movement))
    {
        struct FCase { uint32 Bits; bool Expected; };
        const FCase Cases[]{{0x00000000u, true}, {0x80000000u, true},
            {0x00000001u, true}, {0x80000001u, true}, {0x00800000u, true},
            {0x3f800000u, true}, {0xbf800000u, true}, {0x437a0000u, true}, {0xc37a0000u, true},
            {0x43960000u, true}, {0xc3960000u, true}, {0x443b8000u, true}, {0xc43b8000u, true},
            {0x44480000u, true}, {0xc4480000u, true},
            {0x44505555u, true}, {0xc4505555u, true}, {0x44505556u, true}, {0xc4505556u, true},
            {0x44610000u, true}, {0xc4610000u, true}, {0x7fc00000u, true},
            {0x7f800000u, true}, {0xff800000u, true}};
        for (const FCase& Case : Cases)
        {
            Movement->SetForwardSpeed(std::bit_cast<float>(Case.Bits));
            TestEqual(FString::Printf(TEXT("Structural attachment is independent of velocity bits %08x"), Case.Bits), QuerySpeedAccess(), Case.Expected);
        }
        Movement->SetForwardSpeed(0.0f);
    }
    // Simulate role only, NOT a network test or a player.
    Anchor->SetRole(ROLE_SimulatedProxy);
    TestFalse(TEXT("Client-role initialization is rejected"), Init(Car, Relative));
    TestFalse(TEXT("Client-role cleanup is rejected"), Anchor->ResetPassengerAnchor());
    Anchor->SetRole(ROLE_Authority);
    Anchor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    TestFalse(TEXT("Detached anchor cannot name a valid attached passenger car"), QuerySpeedAccess());
    TestTrue(TEXT("Empty anchor can reattach to same car"), Init(Car, Relative));
    TestTrue(TEXT("Authority resets empty anchor"), Anchor->ResetPassengerAnchor());
    TestNull(TEXT("Cleanup clears outer car"), Anchor->GetOuterVehicle());
    TestNull(TEXT("Cleanup detaches the invisible anchor"), Anchor->GetAttachParentActor());
    TestNull(TEXT("Cleanup clears owner"), Anchor->GetOwner());
    TestFalse(TEXT("Reset anchor has no attached passenger car"), QuerySpeedAccess());
    World->DestroyWorld(false);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsAnchorAccessTest,
    "Tsabs.Passenger.Anchor.AccessFailsClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTsabsAnchorAccessTest::RunTest(const FString& Parameters)
{
    // The SDK default bodies are stubs: ensure these are OUR overrides, not a
    // false-positive pass caused solely by the SDK's unconditional false return.
    using FCharacterGate = bool (ATsabsPassengerAnchor::*)(AFGCharacterPlayer*);
    using FLeave = bool (ATsabsPassengerAnchor::*)(bool);
    using FRpc = void (ATsabsPassengerAnchor::*)();
    TestTrue(TEXT("Own boarding query overrides the SDK default"),
        (std::is_same_v<decltype(&ATsabsPassengerAnchor::CanDriverEnter), FCharacterGate>));
    TestTrue(TEXT("Own server boarding overrides the SDK default"),
        (std::is_same_v<decltype(&ATsabsPassengerAnchor::DriverEnter), FCharacterGate>));
    TestTrue(TEXT("Own exit query overrides the SDK default"),
        (std::is_same_v<decltype(&ATsabsPassengerAnchor::CanLeaveVehicle), FCharacterGate>));
    TestTrue(TEXT("Own server exit overrides the SDK default"),
        (std::is_same_v<decltype(&ATsabsPassengerAnchor::DriverLeave), FLeave>));
    TestTrue(TEXT("Own exit RPC revalidation overrides the SDK default"),
        (std::is_same_v<decltype(&ATsabsPassengerAnchor::Server_DriverLeave_Implementation), FRpc>));
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    if (!TestNotNull(TEXT("Isolated guard-test world"), World)) return false;
    ATsabsPassengerAnchor* Anchor = World->SpawnActor<ATsabsPassengerAnchor>();
    ATsabsPassengerRailcar* Car = World->SpawnActor<ATsabsPassengerRailcar>();
    if (!TestNotNull(TEXT("Guard-test anchor"), Anchor) | !TestNotNull(TEXT("Guard-test own car"), Car))
    {
        World->DestroyWorld(false);
        return false;
    }
    TestFalse(TEXT("No car and null character cannot board"), Anchor->CanDriverEnter(nullptr));
    TestFalse(TEXT("No car and null character cannot enter on server"), Anchor->DriverEnter(nullptr));
    TestFalse(TEXT("Empty uninitialized anchor cannot exit"), Anchor->DriverLeave());
    TestTrue(TEXT("Guard fixture attaches own car"), Anchor->InitializePassengerAnchor(Car, FTransform::Identity));
    Car->GetRailroadVehicleMovementComponent()->SetForwardSpeed(0.0f);
    TestFalse(TEXT("Stopped car never grants null character entry"), Anchor->CanDriverEnter(nullptr));
    TestFalse(TEXT("Stopped car never grants null server entry"), Anchor->DriverEnter(nullptr));
    TestFalse(TEXT("Stopped car never grants null occupant exit"), Anchor->CanLeaveVehicle(nullptr));
    TestFalse(TEXT("Empty anchor cannot leave"), Anchor->DriverLeave());
    TestFalse(TEXT("keepDriving cannot bypass passenger exit restrictions"), Anchor->DriverLeave(true));
    AFGCharacterPlayer* Template = GetMutableDefault<AFGCharacterPlayer>();
    TestFalse(TEXT("A class template is not a player who can board"), Anchor->CanDriverEnter(Template));
    TestFalse(TEXT("A class template cannot enter on the server"), Anchor->DriverEnter(Template));
    TestFalse(TEXT("A class template is not the occupant"), Anchor->CanLeaveVehicle(Template));
    Anchor->SetRole(ROLE_SimulatedProxy);
    TestFalse(TEXT("Client-role server entry fails closed"), Anchor->DriverEnter(Template));
    TestFalse(TEXT("Client-role server exit fails closed"), Anchor->DriverLeave());
    Anchor->Server_DriverLeave_Implementation();
    TestNull(TEXT("Unauthorized exit RPC did not create a driver"), Anchor->GetDriver());
    Anchor->SetRole(ROLE_Authority);
    Anchor->Server_DriverLeave_Implementation();
    TestNull(TEXT("Empty server exit RPC does not create a driver"), Anchor->GetDriver());
    TestTrue(TEXT("Rejected accesses leave attachment intact"), Anchor->GetOuterVehicle() == Car && Anchor->GetAttachParentActor() == Car);
    World->DestroyWorld(false);
    return true;
}
#endif
