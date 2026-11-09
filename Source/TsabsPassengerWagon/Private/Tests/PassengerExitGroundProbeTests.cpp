#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "PassengerExitGeometry.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsPassengerSafeExitAlternativesTest,
    "Tsabs.Passenger.Boarding.SafeExitAlternatives",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTsabsPassengerSafeExitAlternativesTest::RunTest(const FString& Parameters)
{
    const FBox Bounds(FVector(-700, -245, 150), FVector(700, 245, 322));
    FVector Selected(999, 999, 999);
    TArray<FVector> Checked;
    auto Validate = [&](const FVector& Candidate)
    {
        Checked.Add(Candidate);
        // Only the second location is clear; all candidates require native approval.
        return Checked.Num() == 2;
    };
    const bool bFound = TsabsPassenger::TryFindSafeExitOffset(Bounds, 40, 96, Validate, Selected);
    TestTrue(TEXT("A nearby alternative is tried when the central exit is blocked"), bFound);
    TestEqual(TEXT("The validated alternative is returned"), Checked.Num(), 2);
    if (Checked.Num() == 2) TestTrue(TEXT("Output is exactly a validated candidate"), Selected.Equals(Checked[1]));
    Selected = FVector(999, 999, 999);
    int32 Calls = 0;
    const bool bAllBlocked = TsabsPassenger::TryFindSafeExitOffset(Bounds, 40, 96,
        [&](const FVector&) { ++Calls; return false; }, Selected);
    TestFalse(TEXT("Never accept when native validation rejects every candidate"), bAllBlocked);
    TestTrue(TEXT("Failed search does not manufacture an exit"), Selected.Equals(FVector(999, 999, 999)));
    TestTrue(TEXT("Candidate search is bounded"), Calls > 1 && Calls <= 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsPassengerNativeGroundProbeTest,
    "Tsabs.Passenger.Boarding.NativeExitProbeReachesFlatGround",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTsabsPassengerNativeGroundProbeTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
    ON_SCOPE_EXIT
    {
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
    };
    const FVector Origin(30000.0, 40000.0, 5000.0);
    auto MakeBox = [&](const FVector& Location, const FVector& Extent)
    {
        AActor* Actor = World->SpawnActor<AActor>();
        UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
        Actor->SetRootComponent(Box);
        Box->SetBoxExtent(Extent);
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Box->SetCollisionResponseToAllChannels(ECR_Block);
        Box->RegisterComponent();
        Actor->SetActorLocation(Location);
        return Box;
    };
    // Chassis dimensions and capsule size observed in the failing 0.1.1 trial.
    const FBox LocalBody(FVector(-716.733, -245.405, 152.924), FVector(716.733, 245.408, 322.007));
    UBoxComponent* Chassis = MakeBox(Origin + LocalBody.GetCenter(), LocalBody.GetExtent());
    UBoxComponent* Floor = MakeBox(Origin + FVector(0, 0, -25), FVector(5000, 5000, 25));
    AActor* Anchor = World->SpawnActor<AActor>();
    AActor* Driver = World->SpawnActor<AActor>();
    UCapsuleComponent* Capsule = NewObject<UCapsuleComponent>(Driver);
    Capsule->InitCapsuleSize(40.0f, 96.0f);
    const FCollisionShape Shape = Capsule->GetCollisionShape(0.1f);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(TsabsExitGroundRegression), false);
    Query.AddIgnoredActor(Anchor);
    Query.AddIgnoredActor(Driver);

    UBoxComponent* BodyProxy = nullptr;
    // Replay of the COLLISION QUERY sequence observed in the installed native
    // GetSafeExitLocation (RVA 0x78A1C0): this is real engine physics, not a mock
    // of that SDK stub and not evidence of an actual player entering/leaving.
    auto NativeQuerySequenceAccepts = [&](FVector Offset)
    {
        for (int32 Side = 0; Side < 2; ++Side)
        {
            if (Side == 1) Offset.Y = -Offset.Y;
            const FVector Destination = Origin + Offset;
            FHitResult Lateral;
            const FBox SweepBounds = BodyProxy ? Chassis->Bounds.GetBox() + BodyProxy->Bounds.GetBox() : Chassis->Bounds.GetBox();
            World->SweepSingleByChannel(Lateral, SweepBounds.GetCenter(), Destination,
                FQuat::Identity, ECC_Pawn, Shape, Query);
            if (!Lateral.bBlockingHit) return true;
            FHitResult Down;
            World->SweepSingleByChannel(Down, Destination + FVector(0, 0, 300), Destination,
                FQuat::Identity, ECC_Pawn, Shape, Query);
            // The native fallback accepts a non-initial contact; NO hit is not
            // accepted by this branch. A target above the floor can stop too soon.
            if (Down.bBlockingHit && Down.Time > 0.0f) return true;
        }
        return false;
    };
    FVector Offset;
    if (!TestTrue(TEXT("Production geometry makes a lateral probe"),
        TsabsPassenger::TryMakeExitOffset(LocalBody, 40.0, 96.0, Offset))) return false;
    TestTrue(TEXT("Native-compatible query accepts a clear exit on flat ground"), NativeQuerySequenceAccepts(Offset));
    // Check the approximate new body proxy in the actual collision-query replay.
    // These blockers close the central landing on BOTH sides, not the whole car.
    BodyProxy = MakeBox(Origin + FVector(0, 0, 480), FVector(635, 200, 220));
    TestTrue(TEXT("Proxy still permits a clear flat-ground native query"), NativeQuerySequenceAccepts(Offset));
    MakeBox(Origin + FVector(0, 325, 120), FVector(65, 60, 120));
    MakeBox(Origin + FVector(0, -325, 120), FVector(65, 60, 120));
    TestFalse(TEXT("Blocked central landing is rejected on both sides"), NativeQuerySequenceAccepts(Offset));
    FVector Alternative;
    const bool bAlternative = TsabsPassenger::TryFindSafeExitOffset(LocalBody, 40.0, 96.0,
        [&](const FVector& Probe) { return NativeQuerySequenceAccepts(Probe); }, Alternative);
    TestTrue(TEXT("A clear nearby landing is found using the native-query replay"), bAlternative);
    if (bAlternative)
    {
        TestTrue(TEXT("Alternative lies near the carriage"), FMath::Abs(Alternative.X - Offset.X) <= 200.0);
        TestTrue(TEXT("Alternative was accepted by collision query"), NativeQuerySequenceAccepts(Alternative));
    }
    Floor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FVector Unused;
    TestFalse(TEXT("No candidate is accepted over missing ground in this blocked fixture"),
        TsabsPassenger::TryFindSafeExitOffset(LocalBody, 40.0, 96.0,
            [&](const FVector& Probe) { return NativeQuerySequenceAccepts(Probe); }, Unused));
    TestFalse(TEXT("Missing ground still fails when the chassis blocks the lateral sweep"), NativeQuerySequenceAccepts(Offset));
    return true;
}
#endif
