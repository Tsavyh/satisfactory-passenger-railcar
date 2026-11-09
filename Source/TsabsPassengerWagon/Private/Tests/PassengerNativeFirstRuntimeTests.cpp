#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Subsystems/WorldSubsystem.h"
#include "TsabsPassengerRailcar.h"
#include "TsabsPassengerAnchor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsNativeFirstRetiredRestoreTest,
    "Tsabs.Passenger.NativeFirst.Runtime.NoCustomRestoreSubsystem",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsNativeFirstRetiredRestoreTest::RunTest(const FString& Parameters)
{
    TestNull(TEXT("Native-first module no longer registers the retired custom load coordinator"),
        FindObject<UClass>(nullptr, TEXT("/Script/TsabsPassengerWagon.TsabsPassengerRestoreSubsystem")));
    TestTrue(TEXT("Passenger railcar remains registered"), ATsabsPassengerRailcar::StaticClass() != nullptr);
    TestTrue(TEXT("Native-backed invisible passenger remains registered"), ATsabsPassengerAnchor::StaticClass() != nullptr);
    return true;
}
#endif
