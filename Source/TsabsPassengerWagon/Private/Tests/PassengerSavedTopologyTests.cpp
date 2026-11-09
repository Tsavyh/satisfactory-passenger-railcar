#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TsabsPassengerRailcar.h"
#include "TsabsPassengerAnchor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsSavedForeignAttachmentTest,
    "Tsabs.Passenger.Restore.Topology.ForeignAttachmentPreserved",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsSavedForeignAttachmentTest::RunTest(const FString& Parameters)
{
    // Real Engine attachment/production Ensure path, NOT disk deserialization.
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    if (!TestNotNull(TEXT("Isolated world"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    ATsabsPassengerRailcar* Car = World->SpawnActor<ATsabsPassengerRailcar>();
    ATsabsPassengerRailcar* ForeignCar = World->SpawnActor<ATsabsPassengerRailcar>();
    if (!TestNotNull(TEXT("Own car"), Car) || !TestNotNull(TEXT("Foreign car"), ForeignCar) ||
        !TestTrue(TEXT("Ordinary new-car creation still works"), Car->EnsurePilotAnchor())) return false;
    ATsabsPassengerAnchor* Anchor = Car->GetPilotAnchor();
    if (!TestNotNull(TEXT("Original saved-place identity"), Anchor) ||
        !TestTrue(TEXT("Set up foreign attachment without changing saved outer"),
            Anchor->AttachToActor(ForeignCar, FAttachmentTransformRules::KeepWorldTransform))) return false;
    const FTransform Before = Anchor->GetActorTransform();
    AActor* OwnerBefore = Anchor->GetOwner();
    TestTrue(TEXT("Fixture has own outer and independently foreign parent"),
        Anchor->GetOuterVehicle() == Car && Anchor->GetAttachParentActor() == ForeignCar);
    TestFalse(TEXT("Ensure must not steal an existing foreign attachment"), Car->EnsurePilotAnchor());
    TestTrue(TEXT("Foreign parent remains byte-independent native relationship"), Anchor->GetAttachParentActor() == ForeignCar);
    TestTrue(TEXT("Original slot actor is never replaced"), Car->GetPilotAnchor() == Anchor);
    TestTrue(TEXT("Saved outer remains unchanged"), Anchor->GetOuterVehicle() == Car);
    TestTrue(TEXT("Owner remains unchanged"), Anchor->GetOwner() == OwnerBefore);
    TestTrue(TEXT("Transform remains unchanged on refusal"), Anchor->GetActorTransform().Equals(Before));
    TestTrue(TEXT("Inconsistent topology continues to block dismantling"), Car->HasProtectedPilot());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsSavedForeignCleanupTest,
    "Tsabs.Passenger.Restore.Topology.ConflictingRelationshipsCannotBeReset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsSavedForeignCleanupTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    if (!TestNotNull(TEXT("Isolated cleanup world"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    for (int32 Variant = 0; Variant < 4; ++Variant)
    {
        ATsabsPassengerRailcar* Car = World->SpawnActor<ATsabsPassengerRailcar>();
        ATsabsPassengerRailcar* ForeignCar = World->SpawnActor<ATsabsPassengerRailcar>();
        if (!TestNotNull(TEXT("Cleanup own car"), Car) || !TestNotNull(TEXT("Cleanup foreign car"), ForeignCar) ||
            !TestTrue(TEXT("Ordinary initial anchor"), Car->EnsurePilotAnchor())) return false;
        ATsabsPassengerAnchor* Anchor = Car->GetPilotAnchor();
        if (!TestNotNull(TEXT("Cleanup anchor identity"), Anchor)) return false;
        if (Variant == 0)
        {
            if (!TestTrue(TEXT("Foreign parent fixture"), Anchor->AttachToActor(ForeignCar,
                FAttachmentTransformRules::KeepWorldTransform))) return false;
        }
        if (Variant == 1) Anchor->mOuterVehicle = ForeignCar; // Fixture data, never a production setter.
        if (Variant == 2) Anchor->mOuterVehicle = nullptr;
        if (Variant == 3) Anchor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        auto* OuterBefore = Anchor->GetOuterVehicle();
        AActor* ParentBefore = Anchor->GetAttachParentActor();
        AActor* OwnerBefore = Anchor->GetOwner();
        const FTransform TransformBefore = Anchor->GetActorTransform();
        TestFalse(FString::Printf(TEXT("Variant %d inconsistent relationships refuse Reset"), Variant), Anchor->ResetPassengerAnchor());
        TestTrue(TEXT("Rejected reset preserves exact outer"), Anchor->GetOuterVehicle() == OuterBefore);
        TestTrue(TEXT("Rejected reset preserves exact parent"), Anchor->GetAttachParentActor() == ParentBefore);
        TestTrue(TEXT("Rejected reset preserves owner"), Anchor->GetOwner() == OwnerBefore);
        TestTrue(TEXT("Rejected reset preserves transform"), Anchor->GetActorTransform().Equals(TransformBefore));
        TestFalse(TEXT("Car cannot release ambiguous saved topology"), Car->ReleaseEmptyPilot());
        TestTrue(TEXT("Original slot actor remains referenced and alive"), Car->GetPilotAnchor() == Anchor && !Anchor->IsActorBeingDestroyed());
    }
    return true;
}
#endif
