#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "TsabsPassengerRailcar.h"
#include "TsabsPassengerAnchor.h"
#include "Engine/World.h"
#include "FGConstructDisqualifier.h"
#include "FGCharacterPlayer.h"
#include "FGPlayerController.h"
#include "FGHealthComponent.h"
#include "FGRailroadVehicleMovementComponent.h"
#include <bit>
#include "UObject/UnrealType.h"
#include <type_traits>
#include "Engine/Engine.h"
#include "PassengerBoardingReservation.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsPilotLifecycleTest,
    "Tsabs.Passenger.Boarding.PilotLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsPilotLifecycleTest::RunTest(const FString& Parameters)
{
    FObjectProperty* Pilot = FindFProperty<FObjectProperty>(ATsabsPassengerRailcar::StaticClass(), TEXT("PilotAnchor"));
    if (!TestNotNull(TEXT("Car owns one saved and replicated pilot anchor reference"), Pilot)) return false;
    TestTrue(TEXT("Pilot reference is saved"), Pilot->HasAnyPropertyFlags(CPF_SaveGame));
    TestTrue(TEXT("Pilot reference is replicated"), Pilot->HasAnyPropertyFlags(CPF_Net));
    TestTrue(TEXT("Own BeginPlay implements pilot creation"), (std::is_same_v<decltype(&ATsabsPassengerRailcar::BeginPlay), void (ATsabsPassengerRailcar::*)()>));
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
    ATsabsPassengerRailcar* Car = World->SpawnActor<ATsabsPassengerRailcar>();
    Car->DispatchBeginPlay(); // Engine sets its BeginPlay state before the native override.
    ATsabsPassengerAnchor* Anchor = Cast<ATsabsPassengerAnchor>(Pilot->GetObjectPropertyValue_InContainer(Car));
    if (TestNotNull(TEXT("Authority creates pilot anchor"), Anchor))
    {
        TestTrue(TEXT("Outer vehicle initialized"), Anchor->GetOuterVehicle() == Car);
        TestTrue(TEXT("Attachment initialized"), Anchor->GetAttachParentActor() == Car);
        TestTrue(TEXT("Initial owner is railcar"), Anchor->GetOwner() == Car);
        TestTrue(TEXT("Pilot is not a visible interior"), !Anchor->GetActorEnableCollision());
    }
    ATsabsPassengerRailcar* ClientCar = World->SpawnActor<ATsabsPassengerRailcar>();
    ClientCar->SetRole(ROLE_SimulatedProxy);
    ClientCar->DispatchBeginPlay();
    TestNull(TEXT("Client never creates pilot"), Pilot->GetObjectPropertyValue_InContainer(ClientCar));
    ClientCar->SetRole(ROLE_Authority);
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsUseBoundaryTest,
    "Tsabs.Passenger.Boarding.StandardUseBoundary",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsUseBoundaryTest::RunTest(const FString& Parameters)
{
    using FUse = void (ATsabsPassengerRailcar::*)(AFGCharacterPlayer*, const FUseState&);
    using FDescription = FText (ATsabsPassengerRailcar::*)(AFGCharacterPlayer*, const FUseState&) const;
    using FState = void (ATsabsPassengerRailcar::*)(AFGCharacterPlayer*, const FVector&, UPrimitiveComponent*, FUseState&);
    TestTrue(TEXT("Own standard OnUse replaces freight/rerail use"), (std::is_same_v<decltype(&ATsabsPassengerRailcar::OnUse_Implementation), FUse>));
    TestTrue(TEXT("Own standard feedback"), (std::is_same_v<decltype(&ATsabsPassengerRailcar::GetLookAtDecription_Implementation), FDescription>));
    TestTrue(TEXT("Own standard use-state update"), (std::is_same_v<decltype(&ATsabsPassengerRailcar::UpdateUseState_Implementation), FState>));
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
    ATsabsPassengerRailcar* Car = World->SpawnActor<ATsabsPassengerRailcar>();
    Car->EnsurePilotAnchor();
    FUseState Forged;
    Forged.SetUseState(UFGUseState_Valid::StaticClass());
    Forged.bIsTraceHit = true;
    Forged.UseLocation = Car->GetActorLocation();
    TestTrue(TEXT("Railcar keeps use outline for feedback"), Car->IsUseable_Implementation());
    TestFalse(TEXT("Invalid requester has useful feedback"), Car->GetLookAtDecription_Implementation(nullptr, Forged).IsEmpty());
    ATsabsPassengerAnchor* Before = Car->GetPilotAnchor();
    Car->OnUse_Implementation(nullptr, Forged);
    TestTrue(TEXT("Forged client state cannot alter pilot identity"), Car->GetPilotAnchor() == Before);
    TestNull(TEXT("Null use cannot create driver"), Before->GetDriver());
    Car->SetRole(ROLE_SimulatedProxy);
    Car->OnUse_Implementation(nullptr, Forged);
    TestNull(TEXT("Client use cannot create driver"), Before->GetDriver());
    Car->SetRole(ROLE_Authority);
    TestFalse(TEXT("Never drive the railcar itself"), Car->DriverEnter(nullptr));
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsPilotProtectionTest,
    "Tsabs.Passenger.Boarding.ProtectedLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsPilotProtectionTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Own dismantle eligibility, not SDK false stub"), (std::is_same_v<decltype(&ATsabsPassengerRailcar::CanDismantle_Implementation), bool (ATsabsPassengerRailcar::*)() const>));
    TestTrue(TEXT("Own authoritative dismantle action"), (std::is_same_v<decltype(&ATsabsPassengerRailcar::Dismantle_Implementation), void (ATsabsPassengerRailcar::*)()>));
    TestTrue(TEXT("Own EndPlay handles empty cleanup"), (std::is_same_v<decltype(&ATsabsPassengerRailcar::EndPlay), void (ATsabsPassengerRailcar::*)(EEndPlayReason::Type)>));
    TestTrue(TEXT("Own saved-pilot restoration hook"), (std::is_same_v<decltype(&ATsabsPassengerRailcar::PostLoadGame_Implementation), void (ATsabsPassengerRailcar::*)(int32, int32)>));
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
    ATsabsPassengerRailcar* Car = World->SpawnActor<ATsabsPassengerRailcar>();
    TestTrue(TEXT("Pilot initializes"), Car->EnsurePilotAnchor());
    ATsabsPassengerAnchor* Anchor = Car->GetPilotAnchor();
    TestTrue(TEXT("Second initialization is idempotent"), Car->EnsurePilotAnchor() && Car->GetPilotAnchor() == Anchor);
    Anchor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    Car->PostLoadGame_Implementation(0, 0); // Restoration hook only, NOT a save/load test.
    TestTrue(TEXT("Empty saved pilot reattaches without duplication"), Car->GetPilotAnchor() == Anchor && Anchor->GetAttachParentActor() == Car);
    FObjectProperty* Driver = FindFProperty<FObjectProperty>(AFGDriveablePawn::StaticClass(), TEXT("mDriver"));
    if (TestNotNull(TEXT("Native occupant property available for guard data"), Driver))
    {
        // A sentinel identity in native state only: no fake successful possession/Super.
        Driver->SetObjectPropertyValue_InContainer(Anchor, GetMutableDefault<AFGCharacterPlayer>());
        TArray<TSubclassOf<UFGConstructDisqualifier>> Reasons;
        Car->GetDismantleDisqualifiers_Implementation(Reasons, {});
        TestTrue(TEXT("Native occupant has dismantle feedback"), Reasons.Contains(UFGDDVehicleHasDriver::StaticClass()));
        TestFalse(TEXT("Occupied car cannot dismantle"), Car->CanDismantle_Implementation());
        Car->Dismantle_Implementation();
        TestFalse(TEXT("Blocked dismantle does not destroy anchor"), Anchor->IsActorBeingDestroyed());
        TestTrue(TEXT("Blocked dismantle preserves native occupant"), Anchor->GetDriver() == GetMutableDefault<AFGCharacterPlayer>());
        TestFalse(TEXT("Occupied initialization cannot change attachment"), Anchor->ResetPassengerAnchor());
        AddExpectedMessagePlain(TEXT("Passenger pilot retained during EndPlay"), ELogVerbosity::Warning,
            EAutomationExpectedMessageFlags::Contains, 1);
        Car->EndPlay(EEndPlayReason::Destroyed);
        TestFalse(TEXT("Mod EndPlay does not destroy occupied pilot"), Anchor->IsActorBeingDestroyed());
        TestTrue(TEXT("Mod EndPlay does not eject native occupant data"), Anchor->GetDriver() == GetMutableDefault<AFGCharacterPlayer>());
        Driver->SetObjectPropertyValue_InContainer(Anchor, nullptr);
    }
    FObjectProperty* ControllerData = FindFProperty<FObjectProperty>(APawn::StaticClass(), TEXT("Controller"));
    if (TestNotNull(TEXT("Native controller property"), ControllerData))
    {
        ControllerData->SetObjectPropertyValue_InContainer(Anchor, GetMutableDefault<AFGPlayerController>());
        TestTrue(TEXT("Lingering controller protects pilot"), Car->HasProtectedPilot());
        TestFalse(TEXT("Lingering controller prevents release"), Car->ReleaseEmptyPilot());
        ControllerData->SetObjectPropertyValue_InContainer(Anchor, nullptr);
    }
    {
        // Exercise the actual railcar transaction, not a second test-only lock.
        FScopedPassengerBoardingReservation Scope(Car->BoardingReservation, Car);
        TestTrue(TEXT("Server transaction protects dismantle"), Car->HasProtectedPilot());
        TestFalse(TEXT("Reservation prevents empty cleanup"), Car->ReleaseEmptyPilot());
        TestFalse(TEXT("Reservation prevents reinitialization"), Car->EnsurePilotAnchor());
        Car->Dismantle_Implementation();
        TestFalse(TEXT("Reserved pilot is not destroyed"), Anchor->IsActorBeingDestroyed());
        TestFalse(TEXT("Transaction identity alone never authorizes a character"), Car->IsBoardingReservationValid(nullptr));
        TestFalse(TEXT("Reservation prevents direct anchor reinitialization"), Anchor->InitializePassengerAnchor(Car, FTransform::Identity));
        TestFalse(TEXT("Reservation prevents direct empty-anchor reset"), Anchor->ResetPassengerAnchor());
    }
    TestFalse(TEXT("Empty unreserved pilot is unprotected"), Car->HasProtectedPilot());
    Car->EndPlay(EEndPlayReason::Destroyed);
    TestTrue(TEXT("Only empty owned anchor is destroyed in cleanup"), Anchor->IsActorBeingDestroyed());
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsReservationTest,
    "Tsabs.Passenger.Boarding.ExclusiveReservation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsReservationTest::RunTest(const FString& Parameters)
{
    // Pure transaction identities, NOT simulated characters or native entry.
    UObject* First = NewObject<UFGUseState_Valid>();
    UObject* Second = NewObject<UFGUseState_Valid>();
    FPassengerBoardingReservation Reservation;
    TestFalse(TEXT("Empty transaction"), Reservation.IsReserved());
    TestFalse(TEXT("Null identity cannot reserve"), Reservation.TryAcquire(nullptr));
    {
        FScopedPassengerBoardingReservation Scope(Reservation, First);
        TestTrue(TEXT("First transaction acquired"), Scope.IsAcquired());
        TestTrue(TEXT("Exact request identity owns transaction"), Reservation.IsHeldFor(First));
        TestFalse(TEXT("Other identity has no grant"), Reservation.IsHeldFor(Second));
        TestFalse(TEXT("Reentrant same identity cannot enter twice"), Reservation.TryAcquire(First));
        TestFalse(TEXT("Second identity cannot enter concurrently"), Reservation.TryAcquire(Second));
        TestFalse(TEXT("Other identity cannot release first grant"), Reservation.Release(Second));
        TestTrue(TEXT("Failed release retains grant"), Reservation.IsHeldFor(First));
        FScopedPassengerBoardingReservation Reentrant(Reservation, Second);
        TestFalse(TEXT("Failed nested transaction owns nothing"), Reentrant.IsAcquired());
    }
    TestFalse(TEXT("All return paths release scoped transaction"), Reservation.IsReserved());
    TestTrue(TEXT("Next identity can reserve after scope"), Reservation.TryAcquire(Second));
    TestTrue(TEXT("Matching identity releases"), Reservation.Release(Second));
    return true;
}
#endif
