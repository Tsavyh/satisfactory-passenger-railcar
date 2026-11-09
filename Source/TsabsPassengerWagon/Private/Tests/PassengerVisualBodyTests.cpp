#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "TsabsPassengerRailcar.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "FGFreightWagon.h"
#include "FGInventoryComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsVisualBodyComponentTest,
    "Tsabs.Passenger.NativeFirst.Visual.BodyAttachedWithoutCargo",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTsabsVisualBodyComponentTest::RunTest(const FString& Parameters)
{
    ATsabsPassengerRailcar* Car = GetMutableDefault<ATsabsPassengerRailcar>();
    UStaticMeshComponent* Body = FindObject<UStaticMeshComponent>(Car, TEXT("PassengerBodyVisual"));
    TestNotNull(TEXT("Passenger body visual component exists"), Body);
    TestFalse(TEXT("Passenger car does not inherit cargo behavior"), Car->IsA(AFGFreightWagon::StaticClass()));
    TestNull(TEXT("Passenger car has no cargo inventory"), Car->FindComponentByClass<UFGInventoryComponent>());
    if (Body)
    {
        TestTrue(TEXT("Body is attached to the wagon skeletal component"), Body->GetAttachParent() == Car->GetPassengerMesh());
        TestEqual(TEXT("Body follows the chassis body bone"), Body->GetAttachSocketName(), FName(TEXT("body")));
        TestEqual(TEXT("Added body has no gameplay collision"), Body->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
        if (TestNotNull(TEXT("Body has a private prototype mesh"), Body->GetStaticMesh().Get()))
            TestEqual(TEXT("Body uses the closed-shell prototype"), Body->GetStaticMesh()->GetPathName(),
                FString(TEXT("/TsabsPassengerWagon/VisualPrivate/Study30/PassengerBody30_PRIVATE.PassengerBody30_PRIVATE")));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsBodyCollisionSelectionTest,
    "Tsabs.Passenger.NativeFirst.Visual.BodyCollisionAndSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTsabsBodyCollisionSelectionTest::RunTest(const FString& Parameters)
{
    ATsabsPassengerRailcar* Car = GetMutableDefault<ATsabsPassengerRailcar>();
    UStaticMeshComponent* Body = FindObject<UStaticMeshComponent>(Car, TEXT("PassengerBodyVisual"));
    UBoxComponent* Bounds = FindObject<UBoxComponent>(Car, TEXT("PassengerBodyCollision"));
    if (!TestNotNull(TEXT("Container collision and target component exists"), Bounds)) return false;
    TestTrue(TEXT("Collision follows the visible body"), Bounds->GetAttachParent() == Body);
    TestTrue(TEXT("Container blocks player characters"), Bounds->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);
    TestTrue(TEXT("Build gun can select the body"), Bounds->GetCollisionResponseToChannel(ECC_GameTraceChannel5) == ECR_Block);
    TestTrue(TEXT("Interaction can hit the body"), Bounds->GetCollisionResponseToChannel(ECC_GameTraceChannel13) != ECR_Ignore);
    TestTrue(TEXT("Body volume is substantial"), Bounds->GetUnscaledBoxExtent().GetMin() > 50.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsVisualMaterialsTest,
    "Tsabs.Passenger.NativeFirst.Visual.MaterialSlotsNotPlaceholders",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTsabsVisualMaterialsTest::RunTest(const FString& Parameters)
{
    ATsabsPassengerRailcar* Car = GetMutableDefault<ATsabsPassengerRailcar>();
    UStaticMeshComponent* Body = FindObject<UStaticMeshComponent>(Car, TEXT("PassengerBodyVisual"));
    if (!TestNotNull(TEXT("Body visual exists"), Body) || !TestNotNull(TEXT("Body mesh exists"), Body->GetStaticMesh().Get())) return false;
    auto Check = [&](const TCHAR* SlotName, const TCHAR* ExpectedPath)
    {
        const int32 Index = Body->GetStaticMesh()->GetMaterialIndex(FName(SlotName));
        if (!TestTrue(FString::Printf(TEXT("Slot %s exists"), SlotName), Index != INDEX_NONE)) return;
        const UMaterialInterface* Material = Body->GetMaterial(Index);
        if (TestNotNull(FString::Printf(TEXT("Slot %s has material"), SlotName), Material))
            TestEqual(FString::Printf(TEXT("Slot %s uses assigned material"), SlotName), Material->GetPathName(), FString(ExpectedPath));
    };
    Check(TEXT("Study02 | opaque glass"), TEXT("/TsabsPassengerWagon/VisualPrivate/Materials/M_WindowOpaque.M_WindowOpaque"));
    Check(TEXT("Study02 | preview orange"), TEXT("/TsabsPassengerWagon/VisualPrivate/Materials/M_PrimaryPaint.M_PrimaryPaint"));
    Check(TEXT("Study02 | charcoal metal"), TEXT("/TsabsPassengerWagon/VisualPrivate/Materials/M_SecondaryPaint.M_SecondaryPaint"));
    Check(TEXT("MI_Freight_Cargo"), TEXT("/Game/FactoryGame/Buildable/Vehicle/Train/Material/MI_Freight_Cargo.MI_Freight_Cargo"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsChassisFailureDiagnosticTest,
    "Tsabs.Passenger.NativeFirst.Visual.ChassisFailureDiagnostic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTsabsChassisFailureDiagnosticTest::RunTest(const FString& Parameters)
{
    class FCapture final : public FOutputDevice
    {
    public:
        FString Text;
        virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override
        {
            if (Message && Category == FName(TEXT("LogTsabsPassengerWagon"))) Text += Message;
        }
    } Capture;
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
    ATsabsPassengerRailcar* Car = World->SpawnActor<ATsabsPassengerRailcar>();
    Car->GetPassengerMesh()->SetSkeletalMeshAsset(nullptr);
    GLog->AddOutputDevice(&Capture);
    Car->DispatchBeginPlay();
    GLog->RemoveOutputDevice(&Capture);
    TestTrue(TEXT("Missing native chassis identifies exact fallback reason"),
        Capture.Text.Contains(TEXT("TSABS_CHASSIS_DETAIL reason=native_mesh_missing")));
    ATsabsPassengerRailcar* WithMissingSlot = World->SpawnActor<ATsabsPassengerRailcar>();
    USkeletalMesh* MatFixture = LoadObject<USkeletalMesh>(nullptr,
        TEXT("/TsabsPassengerWagon/VisualPrivate/Study26/Skeletal/PassengerChassis26_PRIVATE.PassengerChassis26_PRIVATE"));
    if (TestNotNull(TEXT("Material diagnostic fixture loads"), MatFixture))
    {
        USkeletalMesh* Altered = DuplicateObject<USkeletalMesh>(MatFixture, GetTransientPackage());
        Altered->GetMaterials()[0].MaterialSlotName = FName(TEXT("MissingDiagnosticSlot"));
        WithMissingSlot->GetPassengerMesh()->SetSkeletalMeshAsset(Altered);
        Capture.Text.Empty();
        GLog->AddOutputDevice(&Capture);
        WithMissingSlot->DispatchBeginPlay();
        GLog->RemoveOutputDevice(&Capture);
        TestTrue(TEXT("Material mismatch includes native asset identity"), Capture.Text.Contains(TEXT("native_asset=")));
        TestTrue(TEXT("Material mismatch includes actual material paths"), Capture.Text.Contains(TEXT("native_materials=")));
        TestTrue(TEXT("Material mismatch preserves its actual native slot"), Capture.Text.Contains(TEXT("MissingDiagnosticSlot")));
    }
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsVisualChassisComponentTest,
    "Tsabs.Passenger.NativeFirst.Visual.ChassisFollowerIsCosmetic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTsabsVisualChassisComponentTest::RunTest(const FString& Parameters)
{
    ATsabsPassengerRailcar* Car = GetMutableDefault<ATsabsPassengerRailcar>();
    USkeletalMeshComponent* Chassis = FindObject<USkeletalMeshComponent>(Car, TEXT("PassengerChassisVisual"));
    if (TestNotNull(TEXT("Clipped chassis visual component exists"), Chassis))
    {
        TestTrue(TEXT("Original chassis remains the component parent"), Chassis->GetAttachParent() == Car->GetPassengerMesh());
        TestEqual(TEXT("Visual chassis has no physics collision"), Chassis->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
        TestNotNull(TEXT("Visual chassis has a private prototype mesh"), Chassis->GetSkeletalMeshAsset());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsVisualChassisFollowTest,
    "Tsabs.Passenger.NativeFirst.Visual.ChassisFollowsOriginalSafely",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTsabsVisualChassisFollowTest::RunTest(const FString& Parameters)
{
    USkeletalMesh* Fixture = LoadObject<USkeletalMesh>(nullptr,
        TEXT("/TsabsPassengerWagon/VisualPrivate/Study26/Skeletal/PassengerChassis26_PRIVATE.PassengerChassis26_PRIVATE"));
    if (!TestNotNull(TEXT("Private chassis reference fixture loads"), Fixture)) return false;
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
    ATsabsPassengerRailcar* Car = World->SpawnActor<ATsabsPassengerRailcar>();
    USkeletalMeshComponent* Original = Car->GetPassengerMesh();
    USkeletalMeshComponent* Visual = FindObject<USkeletalMeshComponent>(Car, TEXT("PassengerChassisVisual"));
    if (TestNotNull(TEXT("Visual chassis component exists"), Visual))
    {
        Original->SetSkeletalMeshAsset(Fixture);
        UMaterialInterface* ReferenceMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/TsabsPassengerWagon/VisualPrivate/Materials/M_PrimaryPaint.M_PrimaryPaint"));
        TestNotNull(TEXT("Chassis material fixture is available"), ReferenceMaterial);
        Original->SetMaterial(0, ReferenceMaterial);
        const ECollisionEnabled::Type OriginalCollision = Original->GetCollisionEnabled();
        Car->DispatchBeginPlay();
        TestTrue(TEXT("Visual chassis follows original pose"), Visual->LeaderPoseComponent.Get() == Original);
        TestTrue(TEXT("Visual chassis retains native material overrides"), Visual->GetMaterial(0) == ReferenceMaterial);
        TestTrue(TEXT("Matching visual chassis is shown"), Visual->IsVisible());
        TestFalse(TEXT("Only native chassis rendering is hidden"), Original->IsVisible());
        TestEqual(TEXT("Original gameplay collision remains unchanged"), Original->GetCollisionEnabled(), OriginalCollision);
        TestEqual(TEXT("Visual chassis never gains collision"), Visual->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
    }
    ATsabsPassengerRailcar* Mismatch = World->SpawnActor<ATsabsPassengerRailcar>();
    USkeletalMeshComponent* MismatchNative = Mismatch->GetPassengerMesh();
    USkeletalMeshComponent* MismatchVisual = FindObject<USkeletalMeshComponent>(Mismatch, TEXT("PassengerChassisVisual"));
    USkeletalMesh* WrongPose = LoadObject<USkeletalMesh>(nullptr,
        TEXT("/Game/ResearchPrivatePassengerSkeletal3/PassengerChassis21_PRIVATE.PassengerChassis21_PRIVATE"));
    if (TestNotNull(TEXT("Incompatible pose fixture loads"), WrongPose) && TestNotNull(TEXT("Mismatch visual exists"), MismatchVisual))
    {
        MismatchNative->SetSkeletalMeshAsset(WrongPose);
        Mismatch->DispatchBeginPlay();
        TestTrue(TEXT("Incompatible pose preserves native rendering"), MismatchNative->IsVisible());
        TestFalse(TEXT("Incompatible pose never shows visual chassis"), MismatchVisual->IsVisible());
        TestNull(TEXT("Incompatible pose never gains a leader"), MismatchVisual->LeaderPoseComponent.Get());
    }
    ATsabsPassengerRailcar* Missing = World->SpawnActor<ATsabsPassengerRailcar>();
    USkeletalMeshComponent* OriginalMissing = Missing->GetPassengerMesh();
    USkeletalMeshComponent* VisualMissing = FindObject<USkeletalMeshComponent>(Missing, TEXT("PassengerChassisVisual"));
    OriginalMissing->SetSkeletalMeshAsset(nullptr);
    Missing->DispatchBeginPlay();
    if (TestNotNull(TEXT("Fallback visual exists"), VisualMissing))
    {
        TestFalse(TEXT("Unmatched visual remains hidden"), VisualMissing->IsVisible());
        TestTrue(TEXT("Native render is not hidden when chassis cannot be matched"), OriginalMissing->IsVisible());
    }
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsHologramBodyTemplateAlignmentTest,
    "Tsabs.Passenger.NativeFirst.Visual.HologramBodyTemplateAlignment",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTsabsHologramBodyTemplateAlignmentTest::RunTest(const FString& Parameters)
{
    ATsabsPassengerRailcar* Defaults = GetMutableDefault<ATsabsPassengerRailcar>();
    UStaticMeshComponent* Body = FindObject<UStaticMeshComponent>(Defaults, TEXT("PassengerBodyVisual"));
    USkeletalMeshComponent* Chassis = FindObject<USkeletalMeshComponent>(Defaults, TEXT("PassengerChassisVisual"));
    if (!TestNotNull(TEXT("Default body template exists before BeginPlay"), Body)
        || !TestNotNull(TEXT("Default cosmetic chassis template exists"), Chassis)
        || !TestNotNull(TEXT("Default cosmetic chassis mesh exists"), Chassis->GetSkeletalMeshAsset())) return false;
    const FReferenceSkeleton& Ref = Chassis->GetSkeletalMeshAsset()->GetRefSkeleton();
    const int32 BodyBone = Ref.FindBoneIndex(TEXT("body"));
    if (!TestTrue(TEXT("Visual chassis fixture has a body bone"), BodyBone != INDEX_NONE)) return false;
    FTransform BoneToComponent = FTransform::Identity;
    for (int32 Index = BodyBone; Index != INDEX_NONE; Index = Ref.GetParentIndex(Index))
        BoneToComponent = BoneToComponent * Ref.GetRefBonePose()[Index];
    TestTrue(TEXT("Before BeginPlay the visual body template cancels the chassis body-bone reference transform"),
        (Body->GetRelativeTransform() * BoneToComponent).Equals(FTransform::Identity, 0.01f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsVisualBodyReferenceAlignmentTest,
    "Tsabs.Passenger.NativeFirst.Visual.BodyReferenceAlignment",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTsabsVisualBodyReferenceAlignmentTest::RunTest(const FString& Parameters)
{
    USkeletalMesh* Fixture = LoadObject<USkeletalMesh>(nullptr,
        TEXT("/Game/ResearchPrivatePassengerSkeletal3/PassengerChassis21_PRIVATE.PassengerChassis21_PRIVATE"));
    if (!TestNotNull(TEXT("Private 32-bone chassis fixture loads"), Fixture)) return false;
    const FReferenceSkeleton& Ref = Fixture->GetRefSkeleton();
    const int32 BoneIndex = Ref.FindBoneIndex(TEXT("body"));
    if (!TestTrue(TEXT("Fixture contains body bone"), BoneIndex != INDEX_NONE)) return false;
    FTransform BoneToComponent = FTransform::Identity;
    for (int32 Index = BoneIndex; Index != INDEX_NONE; Index = Ref.GetParentIndex(Index))
        BoneToComponent = BoneToComponent * Ref.GetRefBonePose()[Index];

    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
    ATsabsPassengerRailcar* Car = World->SpawnActor<ATsabsPassengerRailcar>();
    UStaticMeshComponent* Body = FindObject<UStaticMeshComponent>(Car, TEXT("PassengerBodyVisual"));
    if (TestNotNull(TEXT("Body exists on actor instance"), Body))
    {
        Car->GetPassengerMesh()->SetSkeletalMeshAsset(Fixture);
        Car->DispatchBeginPlay();
        const FTransform Combined = Body->GetRelativeTransform() * BoneToComponent;
        TestTrue(TEXT("Body retains chassis origin when attached to the body bone"),
                 Combined.Equals(FTransform::Identity, 0.01f));
        TestTrue(TEXT("Body is visible when reference bone is valid"), Body->IsVisible());
        UBoxComponent* BodyCollision = FindObject<UBoxComponent>(Car, TEXT("PassengerBodyCollision"));
        if (TestNotNull(TEXT("Body collision exists on actor instance"), BodyCollision))
        {
            TestEqual(TEXT("Visible body enables collision queries"), BodyCollision->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);
            FHitResult Hit;
            const FVector Center = BodyCollision->GetComponentLocation();
            const FVector Outward = BodyCollision->GetRightVector() * 400.0;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(PassengerBodyTrace), false);
            const bool bHit = World->LineTraceSingleByChannel(Hit, Center + Outward, Center, ECC_GameTraceChannel5, Params);
            TestTrue(TEXT("Build gun trace hits body collision"), bHit && Hit.GetComponent() == BodyCollision);
            FHitResult PawnHit;
            const FVector PawnStart = Center + Outward + FVector(0, 0, 40);
            const FVector PawnEnd = Center + FVector(0, 0, 40);
            const bool bPawnHit = World->SweepSingleByChannel(PawnHit, PawnStart, PawnEnd, FQuat::Identity,
                ECC_Pawn, FCollisionShape::MakeCapsule(40, 96), Params);
            TestTrue(TEXT("Player capsule sweep hits container"), bPawnHit && PawnHit.GetComponent() == BodyCollision);
            TArray<FHitResult> InteractionHits;
            World->LineTraceMultiByChannel(InteractionHits, Center + Outward, Center, ECC_GameTraceChannel13, Params);
            bool bBodyInteraction = false;
            for (const FHitResult& Interaction : InteractionHits)
                bBodyInteraction |= Interaction.GetComponent() == BodyCollision;
            TestTrue(TEXT("Interaction trace sees the container"), bBodyInteraction);
        }
    }
    ATsabsPassengerRailcar* Missing = World->SpawnActor<ATsabsPassengerRailcar>();
    UStaticMeshComponent* MissingBody = FindObject<UStaticMeshComponent>(Missing, TEXT("PassengerBodyVisual"));
    Missing->GetPassengerMesh()->SetSkeletalMeshAsset(nullptr);
    Missing->DispatchBeginPlay();
    if (TestNotNull(TEXT("Missing-fixture body exists"), MissingBody))
        TestFalse(TEXT("Body fails closed when no skeletal reference is available"), MissingBody->IsVisible());
    UBoxComponent* MissingCollision = FindObject<UBoxComponent>(Missing, TEXT("PassengerBodyCollision"));
    if (TestNotNull(TEXT("Missing-fixture collider exists"), MissingCollision))
        TestEqual(TEXT("Invisible body has no invisible blocking collider"), MissingCollision->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}
#endif
