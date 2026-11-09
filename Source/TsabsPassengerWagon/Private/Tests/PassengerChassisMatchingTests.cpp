#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "PassengerChassisCompatibility.h"
#include "PassengerChassisSlotMapping.h"

namespace
{
FReferenceSkeleton MakeReference(bool bReordered, bool bWrongParent = false, bool bWrongPose = false, bool bMissing = false)
{
    FReferenceSkeleton Ref;
    {
        FReferenceSkeletonModifier Modifier(Ref, nullptr);
        auto Add = [&](const TCHAR* Name, int32 Parent, const FTransform& Pose)
        {
            Modifier.Add(FMeshBoneInfo(FName(Name), FString(Name), Parent), Pose);
        };
        Add(TEXT("root"), INDEX_NONE, FTransform::Identity);
        if (bReordered)
        {
            Add(TEXT("right"), 0, FTransform::Identity);
            Add(TEXT("left"), 0, FTransform::Identity);
            if (!bMissing) Add(TEXT("left_child"), bWrongParent ? 1 : 2,
                FTransform(FQuat::Identity, FVector(bWrongPose ? 50.0 : 10.0, 0.0, 0.0)));
        }
        else
        {
            Add(TEXT("left"), 0, FTransform::Identity);
            if (!bMissing) Add(TEXT("left_child"), bWrongParent ? 0 : 1,
                FTransform(FQuat::Identity, FVector(bWrongPose ? 50.0 : 10.0, 0.0, 0.0)));
            Add(TEXT("right"), 0, FTransform::Identity);
        }
    }
    return Ref;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsChassisReferenceEquivalenceTest,
    "Tsabs.Passenger.NativeFirst.Visual.ChassisReferenceEquivalence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTsabsChassisReferenceEquivalenceTest::RunTest(const FString& Parameters)
{
    const FReferenceSkeleton Native = MakeReference(false);
    TestTrue(TEXT("identical reference is accepted"), PassengerChassisReferenceMatches(Native, MakeReference(false)));
    TestTrue(TEXT("reordered identical named hierarchy and pose is accepted"), PassengerChassisReferenceMatches(Native, MakeReference(true)));
    TestFalse(TEXT("different named parent is rejected"), PassengerChassisReferenceMatches(Native, MakeReference(true, true)));
    TestFalse(TEXT("different local reference pose is rejected"), PassengerChassisReferenceMatches(Native, MakeReference(true, false, true)));
    TestFalse(TEXT("missing bone is rejected"), PassengerChassisReferenceMatches(Native, MakeReference(true, false, false, true)));
    TestFalse(TEXT("empty skeletons are rejected"), PassengerChassisReferenceMatches(FReferenceSkeleton(), FReferenceSkeleton()));
    const FName Freight(TEXT("/Game/FactoryGame/Buildable/Vehicle/Train/Wagon/Mesh/SK_Freight_Wagon.SK_Freight_Wagon"));
    const FName Other(TEXT("/Game/Other.Other"));
    TestEqual(TEXT("freight paint material resolves to the observed native slot"), PassengerChassisNativeSlot(Freight, FName(TEXT("MI_Train"))), FName(TEXT("M_TrainEngine")));
    TestEqual(TEXT("freight normal decal retains its matching native slot"), PassengerChassisNativeSlot(Freight, FName(TEXT("Decal_Normal"))), FName(TEXT("Decal_Normal")));
    TestEqual(TEXT("freight color decal resolves to the observed native slot"), PassengerChassisNativeSlot(Freight, FName(TEXT("DecalColor_Masked"))), FName(TEXT("Decal_Color")));
    TestEqual(TEXT("freight factory material resolves to the observed native slot"), PassengerChassisNativeSlot(Freight, FName(TEXT("MI_Locomotive"))), FName(TEXT("Factory_Inst")));
    TestEqual(TEXT("unknown slot remains subject to ordinary exact match"), PassengerChassisNativeSlot(Freight, FName(TEXT("Unexpected"))), FName(TEXT("Unexpected")));
    TestEqual(TEXT("a different native asset never receives freight aliases"), PassengerChassisNativeSlot(Other, FName(TEXT("MI_Train"))), FName(TEXT("MI_Train")));
    return true;
}
#endif
