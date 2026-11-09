#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "TsabsPassengerRailcar.h"
#include "TsabsPassengerAnchor.h"
#include "TsabsPassengerHUD.h"
#include "FGRailroadVehicleMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsTwoPassengerIsolationTest,
    "Tsabs.Passenger.NativeFirst.Capacity.TwoLocalCameraAndHudInstancesStayIndependent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsTwoPassengerIsolationTest::RunTest(const FString& Parameters)
{
    // Two actual Engine local contexts/widgets. SetPawn and mapping DATA injection
    // only; NOT two connected players, Possess/RPC execution, Slate rendering or exit.
    UWorld* World=UWorld::CreateWorld(EWorldType::EditorPreview,false);
    if (!TestNotNull(TEXT("Isolated world"),World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    auto* Car=World->SpawnActor<ATsabsPassengerRailcar>();
    if (!TestTrue(TEXT("Actual stable two-place creation"),Car->EnsurePassengerAnchors())) return false;
    auto* First=Car->GetPilotAnchor();
    auto* Field=FindFProperty<FObjectPropertyBase>(Car->GetClass(),TEXT("SecondPassengerAnchor"));
    auto* Second=Field?Cast<ATsabsPassengerAnchor>(Field->GetObjectPropertyValue_InContainer(Car)):nullptr;
    if (!TestNotNull(TEXT("Actual second stable actor"),Second)) return false;
    auto* Arm1=First->FindComponentByClass<USpringArmComponent>();
    auto* Arm2=Second->FindComponentByClass<USpringArmComponent>();
    if (!TestNotNull(TEXT("First camera rig"),Arm1) || !TestNotNull(TEXT("Second camera rig"),Arm2)) return false;
    auto* MappingField=FindFProperty<FObjectPropertyBase>(AFGDriveablePawn::StaticClass(),TEXT("mMappingContext"));
    if (!TestNotNull(TEXT("Native mapping reference metadata"),MappingField)) return false;
    auto* Mapping1=Cast<UInputMappingContext>(MappingField->GetObjectPropertyValue_InContainer(First));
    auto* Mapping2=Cast<UInputMappingContext>(MappingField->GetObjectPropertyValue_InContainer(Second));
    if (!TestNotNull(TEXT("First actual mapping"),Mapping1) || !TestNotNull(TEXT("Second actual mapping"),Mapping2)) return false;
    TestTrue(TEXT("Camera rigs and mapping instances are separate objects"),
        Arm1!=Arm2 && Mapping1!=Mapping2 && Mapping1->GetOuter()==First && Mapping2->GetOuter()==Second);
    const FRotator BeforeSecond=Arm2->GetRelativeRotation();
    First->ApplyOrbitInput(FVector2D(37.0,-4.0));
    TestTrue(TEXT("First look input does not rotate second camera"),Arm2->GetRelativeRotation().Equals(BeforeSecond));
    const FRotator AfterFirst=Arm1->GetRelativeRotation();
    Second->ApplyOrbitInput(FVector2D(-29.0,3.0));
    TestTrue(TEXT("Second look input does not rotate first camera"),Arm1->GetRelativeRotation().Equals(AfterFirst));
    auto* Leave=LoadObject<UInputAction>(nullptr,TEXT("/Game/FactoryGame/Inputs/Vehicles/Actions/IA_Vehicles_Leave.IA_Vehicles_Leave"));
    if (!TestNotNull(TEXT("Native leave action asset"),Leave)) return false;
    auto MakeContext=[&](ATsabsPassengerAnchor* Anchor,FKey Key)
    {
        auto* PC=World->SpawnActor<APlayerController>();
        PC->SetPlayer(NewObject<ULocalPlayer>(GEngine));World->AddController(PC);PC->SetPawn(Anchor);
        auto* Input=NewObject<UEnhancedPlayerInput>(PC);PC->PlayerInput=Input;
        auto& Mappings=const_cast<TArray<FEnhancedActionKeyMapping>&>(Input->GetEnhancedActionMappings());
        Mappings.Emplace(Leave,Key);
        return PC;
    };
    auto* PC1=MakeContext(First,EKeys::F);auto* PC2=MakeContext(Second,EKeys::Q);
    Car->GetRailroadVehicleMovementComponent()->SetForwardSpeed(-1000.0f);
    auto* HUD1=CreateWidget<UTsabsPassengerHUD>(PC1,UTsabsPassengerHUD::StaticClass());
    auto* HUD2=CreateWidget<UTsabsPassengerHUD>(PC2,UTsabsPassengerHUD::StaticClass());
    if (!TestNotNull(TEXT("First real widget"),HUD1) || !TestNotNull(TEXT("Second real widget"),HUD2)) return false;
    TestTrue(TEXT("Each widget resolves only its own pawn"),HUD1->GetPassengerAnchor()==First && HUD2->GetPassengerAnchor()==Second);
    TestEqual(TEXT("First player gets first effective binding"),HUD1->ResolveLeaveKey().ToString(),FString(TEXT("F")));
    TestEqual(TEXT("Second player gets independent effective binding"),HUD2->ResolveLeaveKey().ToString(),FString(TEXT("Q")));
    auto* Text1=Cast<UTextBlock>(HUD1->GetWidgetFromName(TEXT("SpeedinKMHText")));
    auto* Text2=Cast<UTextBlock>(HUD2->GetWidgetFromName(TEXT("SpeedinKMHText")));
    if (!TestNotNull(TEXT("First readout"),Text1) || !TestNotNull(TEXT("Second readout"),Text2)) return false;
    TestTrue(TEXT("Speed texts are independent owned objects"),Text1!=Text2 && Text1->GetOuter()!=Text2->GetOuter());
    auto* Host=NewObject<UCanvasPanel>(PC1);Host->AddChild(HUD1);Host->AddChild(HUD2);
    Car->GetRailroadVehicleMovementComponent()->SetForwardSpeed(2500.0f);
    HUD1->NativeTick(FGeometry(),0.01f);HUD2->NativeTick(FGeometry(),0.01f);
    TestTrue(TEXT("Both informational readouts update from the same physical car"),
        Text1->GetText().EqualTo(UTsabsPassengerHUD::FormatPassengerSpeed(2500.0f,true)) && Text2->GetText().EqualTo(Text1->GetText()));
    PC1->SetPawn(nullptr);HUD1->NativeTick(FGeometry(),0.01f);HUD2->NativeTick(FGeometry(),0.01f);
    TestNull(TEXT("First pawn loss removes only first widget"),HUD1->GetParent());
    TestTrue(TEXT("Second widget and pawn remain current"),HUD2->GetParent()==Host && HUD2->GetPassengerAnchor()==Second && PC2->GetPawn()==Second);
    TestTrue(TEXT("First readout invalidates, second continues normally"),
        Text1->GetText().EqualTo(UTsabsPassengerHUD::FormatPassengerSpeed(0,false)) &&
        Text2->GetText().EqualTo(UTsabsPassengerHUD::FormatPassengerSpeed(2500.0f,true)));
    TestEqual(TEXT("First teardown never changes second key"),HUD2->ResolveLeaveKey().ToString(),FString(TEXT("Q")));
    PC2->SetPawn(nullptr);HUD2->NativeTick(FGeometry(),0.01f);
    TestNull(TEXT("Second widget also cleans up locally"),HUD2->GetParent());
    return true;
}
#endif
