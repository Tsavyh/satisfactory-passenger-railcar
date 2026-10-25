#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TsabsPassengerRailcar.h"
#include "TsabsPassengerAnchor.h"
#include "PassengerBoardingReservation.h"
#include "FGUseableInterface.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsReservationInvalidationTest,
    "Tsabs.Passenger.Lifecycle.ReservationReleasesInvalidatedIdentity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsReservationInvalidationTest::RunTest(const FString& Parameters)
{
    // UObject invalidation inside a synchronous callback, NOT death/disconnection.
    UObject* Requester = NewObject<UFGUseState_Valid>();
    UObject* Next = NewObject<UFGUseState_Valid>();
    FPassengerBoardingReservation Reservation;
    {
        FScopedPassengerBoardingReservation Scope(Reservation, Requester);
        if (!TestTrue(TEXT("Initial transaction acquired"), Scope.IsAcquired())) return false;
        Requester->MarkAsGarbage(); // No GC runs while the opaque scope identity is in use.
        TestFalse(TEXT("Invalidated requester no longer has a boarding grant"), Reservation.IsHeldFor(Requester));
        TestTrue(TEXT("Transaction remains locked until its scope unwinds"), Reservation.IsReserved());
        TestFalse(TEXT("Another request cannot steal an invalidated active transaction"), Reservation.TryAcquire(Next));
        TestFalse(TEXT("Wrong identity cannot release the transaction"), Reservation.Release(Next));
        TestFalse(TEXT("Null identity cannot release the transaction"), Reservation.Release(nullptr));
    }
    TestFalse(TEXT("Scope destructor releases even after requester invalidation"), Reservation.IsReserved());
    TestTrue(TEXT("Next request acquires after callback scope returns"), Reservation.TryAcquire(Next));
    TestTrue(TEXT("Next matching identity releases normally"), Reservation.Release(Next));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsEmptyCarDestructionTest,
    "Tsabs.Passenger.Lifecycle.ActorDestructionCleansEmptyAnchor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsEmptyCarDestructionTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    if (!TestNotNull(TEXT("Isolated editor world"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    ATsabsPassengerRailcar* Car = World->SpawnActor<ATsabsPassengerRailcar>();
    if (!TestNotNull(TEXT("Own car instance"), Car)) return false;
    // EditorPreview spawn does not initialize actors for gameplay. RouteEndPlay
    // deliberately skips uninitialized actors, so initialize the engine fixture
    // before asserting Destroy -> EndPlay (not a Satisfactory world startup).
    Car->PostInitializeComponents();
    Car->DispatchBeginPlay();
    if (!TestTrue(TEXT("Fixture actor initialized before destruction routing"), Car->IsActorInitialized()) ||
        !TestTrue(TEXT("Fixture actor completed BeginPlay"), Car->HasActorBegunPlay())) return false;
    ATsabsPassengerAnchor* Anchor = Car->GetPilotAnchor();
    if (!TestNotNull(TEXT("BeginPlay creates the technical anchor"), Anchor)) return false;
    TestFalse(TEXT("Empty anchor has no protected occupant state"), Car->HasProtectedPilot());
    // Actual engine Destroy dispatches EndPlay; not a direct callback-only test.
    TestTrue(TEXT("Engine accepts destruction of the empty fixture car"), Car->Destroy());
    TestTrue(TEXT("Car enters actual destruction"), Car->IsActorBeingDestroyed());
    TestNull(TEXT("EndPlay clears the owned empty pilot reference"), Car->GetPilotAnchor());
    TestTrue(TEXT("The owned empty pilot is destroyed too"), Anchor->IsActorBeingDestroyed());
    TestNull(TEXT("Cleanup clears native outer reference before destruction"), Anchor->GetOuterVehicle());
    TestNull(TEXT("Cleanup clears owner before destruction"), Anchor->GetOwner());
    TestNull(TEXT("Cleanup detaches before destruction"), Anchor->GetAttachParentActor());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsDestroyedAnchorSafetyTest,
    "Tsabs.Passenger.Lifecycle.DestroyedAnchorFailsClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsDestroyedAnchorSafetyTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    if (!TestNotNull(TEXT("Isolated editor world"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    ATsabsPassengerRailcar* Car = World->SpawnActor<ATsabsPassengerRailcar>();
    if (!TestNotNull(TEXT("Own car instance"), Car) ||
        !TestTrue(TEXT("Initial pilot creation"), Car->EnsurePilotAnchor())) return false;
    ATsabsPassengerAnchor* Anchor = Car->GetPilotAnchor();
    if (!TestNotNull(TEXT("Own anchor"), Anchor)) return false;
    TestTrue(TEXT("Engine destroys the isolated empty anchor"), Anchor->Destroy());
    TestTrue(TEXT("Anchor destruction was real"), Anchor->IsActorBeingDestroyed());
    TestTrue(TEXT("Broken reference protects against dismantling unknown state"), Car->HasProtectedPilot());
    TestFalse(TEXT("Broken reference cannot be released as an empty valid pilot"), Car->ReleaseEmptyPilot());
    TestFalse(TEXT("Ensure does not silently replace a destroyed referenced pilot"), Car->EnsurePilotAnchor());
    Car->PostLoadGame_Implementation(0, 0); // Hook only; not disk save/load.
    TestTrue(TEXT("Repeated restoration does not silently replace unknown state"), Car->GetPilotAnchor() == Anchor);
    int32 LiveAnchors = 0;
    for (TActorIterator<ATsabsPassengerAnchor> It(World); It; ++It)
        if (IsValid(*It) && !It->IsActorBeingDestroyed()) ++LiveAnchors;
    TestEqual(TEXT("No extra live anchors were spawned during failed repair"), LiveAnchors, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsRestorationIdempotenceTest,
    "Tsabs.Passenger.Lifecycle.RepeatedRestorationHookKeepsSinglePilot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsRestorationIdempotenceTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    if (!TestNotNull(TEXT("Isolated editor world"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    ATsabsPassengerRailcar* Car = World->SpawnActor<ATsabsPassengerRailcar>();
    if (!TestNotNull(TEXT("Own car instance"), Car)) return false;
    // Exercise native override repeatedly, not save serialization or a real game world.
    Car->PostLoadGame_Implementation(0, 0);
    ATsabsPassengerAnchor* Anchor = Car->GetPilotAnchor();
    if (!TestNotNull(TEXT("Restoration hook creates missing empty anchor"), Anchor)) return false;
    for (int32 Iteration = 0; Iteration < 4; ++Iteration)
    {
        Anchor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        Car->PostLoadGame_Implementation(0, 0);
        TestTrue(TEXT("Repeated hook preserves exact pilot identity"), Car->GetPilotAnchor() == Anchor);
        TestTrue(TEXT("Repeated hook repairs empty attachment"), Anchor->GetAttachParentActor() == Car);
        TestTrue(TEXT("Repeated hook retains native outer"), Anchor->GetOuterVehicle() == Car);
        TestTrue(TEXT("Repeated hook retains owner"), Anchor->GetOwner() == Car);
    }
    ATsabsPassengerRailcar* ClientCar = World->SpawnActor<ATsabsPassengerRailcar>();
    if (!TestNotNull(TEXT("Role-only client fixture"), ClientCar)) return false;
    ClientCar->SetRole(ROLE_SimulatedProxy);
    ClientCar->PostLoadGame_Implementation(0, 0);
    TestNull(TEXT("Client-role restoration cannot create a pilot"), ClientCar->GetPilotAnchor());
    ClientCar->SetRole(ROLE_Authority);
    int32 LiveAnchors = 0;
    for (TActorIterator<ATsabsPassengerAnchor> It(World); It; ++It)
        if (IsValid(*It) && !It->IsActorBeingDestroyed()) ++LiveAnchors;
    TestEqual(TEXT("Exactly one live pilot after all callbacks"), LiveAnchors, 1);
    return true;
}
#endif
