#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"
#include "Net/UnrealNetwork.h"
#include "TsabsPassengerRailcar.h"
#include "TsabsPassengerAnchor.h"
#include "FGCharacterPlayer.h"

// Same test calls real one-place implementation for RED and new pair for GREEN.
template<class T>
static bool EnsureNativeFirstPair(T* Car)
{
    if constexpr (requires { Car->EnsurePassengerAnchors(); }) return Car->EnsurePassengerAnchors();
    else return Car->EnsurePilotAnchor();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsTwoStablePlacesTest,
    "Tsabs.Passenger.NativeFirst.Capacity.TwoStableSavedIdentities",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsTwoStablePlacesTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    if (!TestNotNull(TEXT("Isolated Engine world, not a network game"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    auto* Car = World->SpawnActor<ATsabsPassengerRailcar>();
    auto* ForeignCar = World->SpawnActor<ATsabsPassengerRailcar>();
    if (!TestNotNull(TEXT("Own car"), Car) || !TestNotNull(TEXT("Foreign fixture car"), ForeignCar) ||
        !TestTrue(TEXT("First legacy place remains creatable"), Car->EnsurePilotAnchor())) return false;
    auto* Original = Car->GetPilotAnchor();
    if (!TestTrue(TEXT("Pair initialization uses production method"), EnsureNativeFirstPair(Car))) return false;
    int32 Count = 0;
    for (TActorIterator<ATsabsPassengerAnchor> It(World); It; ++It)
        if (IsValid(*It) && !It->IsActorBeingDestroyed()) ++Count;
    TestEqual(TEXT("Exactly two distinct live technical places"), Count, 2);
    TestTrue(TEXT("Original saved PilotAnchor identity is never replaced"), Car->GetPilotAnchor() == Original);
    FObjectPropertyBase* FirstProperty = FindFProperty<FObjectPropertyBase>(Car->GetClass(), TEXT("PilotAnchor"));
    FObjectPropertyBase* SecondProperty = FindFProperty<FObjectPropertyBase>(Car->GetClass(), TEXT("SecondPassengerAnchor"));
    if (!TestNotNull(TEXT("Original scalar saved property remains"), FirstProperty) ||
        !TestNotNull(TEXT("Second stable separately named saved reference"), SecondProperty)) return false;
    // Native net-driver prerequisite: RepIndex is assigned here, not by spawning
    // an actor in an EditorPreview world with no net driver (Class.cpp:5468ff).
    Car->GetClass()->SetUpRuntimeReplicationData();
    TArray<FLifetimeProperty> Replicated;
    Car->GetLifetimeReplicatedProps(Replicated);
    for (FObjectPropertyBase* Property : {FirstProperty, SecondProperty})
    {
        TestEqual(TEXT("Each stable reference remains scalar, not an array migration"), Property->ArrayDim, 1);
        TestTrue(TEXT("Each stable reference is SaveGame and replicated"), Property->HasAllPropertyFlags(CPF_SaveGame | CPF_Net));
        TestTrue(TEXT("Each stable reference has exactly the own anchor type"), Property->PropertyClass == ATsabsPassengerAnchor::StaticClass());
        TestTrue(TEXT("Actual replication registration includes each saved place"),
            Replicated.ContainsByPredicate([&](const FLifetimeProperty& Entry) { return Entry.RepIndex == Property->RepIndex; }));
    }
    auto* Second = Cast<ATsabsPassengerAnchor>(SecondProperty->GetObjectPropertyValue_InContainer(Car));
    if (!TestNotNull(TEXT("Second actual spawned place"), Second)) return false;
    TestTrue(TEXT("Separate actors, no alias of the first saved slot"), Second != Original);
    for (auto* Anchor : {Original, Second})
    {
        TestTrue(TEXT("Each place has exact same-world outer and attachment"),
            Anchor->GetWorld() == World && Anchor->GetOuterVehicle() == Car && Anchor->GetAttachParentActor() == Car);
        TestNull(TEXT("No fabricated native driver"), Anchor->GetDriver());
        TestNull(TEXT("No automatic controller"), Anchor->GetController());
    }
    for (int32 Iteration = 0; Iteration < 3; ++Iteration)
    {
        TestTrue(TEXT("Pair creation is idempotent"), EnsureNativeFirstPair(Car));
        TestTrue(TEXT("Repeated calls preserve BOTH exact saved identities"),
            Car->GetPilotAnchor() == Original && SecondProperty->GetObjectPropertyValue_InContainer(Car) == Second);
    }
    // Native driver DATA injection tests protection only; NOT DriverEnter/possession.
    auto* DriverProperty = FindFProperty<FObjectPropertyBase>(AFGDriveablePawn::StaticClass(), TEXT("mDriver"));
    auto* Character = World->SpawnActor<AFGCharacterPlayer>();
    if (!TestNotNull(TEXT("Native driver fixture field"), DriverProperty) || !TestNotNull(TEXT("Offline-shaped driver fixture"), Character)) return false;
    DriverProperty->SetObjectPropertyValue_InContainer(Second, Character);
    TestTrue(TEXT("Second-place occupant without controller still protects whole car"), Car->HasProtectedPilot());
    TestFalse(TEXT("Empty-place cleanup cannot destroy another occupied place"), Car->ReleaseEmptyPilot());
    TestTrue(TEXT("Repeated Ensure keeps occupied second identity"), EnsureNativeFirstPair(Car));
    TestTrue(TEXT("Occupied second and its native driver are unchanged"),
        SecondProperty->GetObjectPropertyValue_InContainer(Car) == Second && Second->GetDriver() == Character && !Second->IsActorBeingDestroyed());
    DriverProperty->SetObjectPropertyValue_InContainer(Second, nullptr);
    TestFalse(TEXT("Both valid empty places unprotect the occupancy predicate"), Car->HasProtectedPilot());
    SecondProperty->SetObjectPropertyValue_InContainer(Car, Original);
    TestTrue(TEXT("Duplicate saved slot identity is protected, never treated as capacity two"), Car->HasProtectedPilot());
    TestFalse(TEXT("Duplicate reference cannot be normalized into a new slot"), EnsureNativeFirstPair(Car));
    TestTrue(TEXT("Duplicate refusal does not change original identity"), Car->GetPilotAnchor() == Original);
    SecondProperty->SetObjectPropertyValue_InContainer(Car, Second);
    TestTrue(TEXT("Fixture foreign attachment"), Second->AttachToActor(ForeignCar, FAttachmentTransformRules::KeepWorldTransform));
    TestFalse(TEXT("Ensure never steals a foreign second-place parent"), EnsureNativeFirstPair(Car));
    TestTrue(TEXT("Foreign second relationship remains untouched"), Second->GetAttachParentActor() == ForeignCar && Second->GetOuterVehicle() == Car);
    TestTrue(TEXT("Broken second place blocks whole-car dismantling"), Car->HasProtectedPilot());
    TestFalse(TEXT("Broken second place cannot be destroyed as an empty pair"), Car->ReleaseEmptyPilot());
    Second->AttachToActor(Car, FAttachmentTransformRules::KeepWorldTransform);
    TestTrue(TEXT("Actual empty-pair cleanup succeeds"), Car->ReleaseEmptyPilot());
    TestNull(TEXT("Empty-pair cleanup clears original scalar"), Car->GetPilotAnchor());
    TestNull(TEXT("Empty-pair cleanup clears second scalar"), SecondProperty->GetObjectPropertyValue_InContainer(Car));
    TestTrue(TEXT("Both owned empty actors are actually destroyed"), Original->IsActorBeingDestroyed() && Second->IsActorBeingDestroyed());
    return true;
}
#endif
