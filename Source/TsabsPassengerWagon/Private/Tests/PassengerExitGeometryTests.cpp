#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "TsabsPassengerRailcar.h"
#include "TsabsPassengerAnchor.h"
#include "Components/BoxComponent.h"
#include "UObject/UObjectGlobals.h"
#include <limits>
#include "PassengerExitGeometry.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsPassengerExitGeometryTest,
    "Tsabs.Passenger.Boarding.ProvisionalExitGeometry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsPassengerExitGeometryTest::RunTest(const FString& Parameters)
{
    UBoxComponent* Body = NewObject<UBoxComponent>();
    Body->SetBoxExtent(FVector(2000, 150, 200));
    const FBox Bounds = Body->CalcBounds(FTransform::Identity).GetBox();
    FVector Offset = FVector::ZeroVector;
    TestTrue(TEXT("Geometric lateral exit candidate exists"), TsabsPassenger::TryMakeExitOffset(Bounds, 40.0, 90.0, Offset));
    TestTrue(TEXT("Capsule candidate clears lateral chassis bounds"), Offset.Y - 40.0 > Bounds.Max.Y);
    TestFalse(TEXT("Candidate is never zero-offset car centre"), Offset.IsNearlyZero());
    TestEqual(TEXT("Native fallback probes to the lower of anchor base and chassis bottom, not capsule-centre height"),
        Offset.Z, FMath::Min(0.0, Bounds.Min.Z));
    FVector Unchanged(1, 2, 3);
    TestFalse(TEXT("Missing physical bounds cannot invent exit"), TsabsPassenger::TryMakeExitOffset(FBox(ForceInit), 40, 90, Unchanged));
    TestFalse(TEXT("Degenerate bounds fail closed"), TsabsPassenger::TryMakeExitOffset(FBox(FVector::ZeroVector, FVector::ZeroVector), 40, 90, Unchanged));
    TestFalse(TEXT("Invalid capsule fails closed"), TsabsPassenger::TryMakeExitOffset(Bounds, 0, 90, Unchanged));
    TestFalse(TEXT("Nonfinite capsule fails closed"), TsabsPassenger::TryMakeExitOffset(Bounds, std::numeric_limits<double>::infinity(), 90, Unchanged));
    TestTrue(TEXT("Rejected geometry does not mutate caller output"), Unchanged == FVector(1, 2, 3));
    TestFalse(TEXT("Missing character cannot validate native safe exit"), GetMutableDefault<ATsabsPassengerAnchor>()->TryGetPassengerExitOffset(nullptr, Offset));
    return true;
}
#endif
