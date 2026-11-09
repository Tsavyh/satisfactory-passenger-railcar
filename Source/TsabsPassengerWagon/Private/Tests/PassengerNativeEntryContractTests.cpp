#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "TsabsPassengerAnchor.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsNativeEntryApiContractTest,
    "Tsabs.Passenger.NativeFirst.Entry.ServerNativeApiIsNotAnEntryRpc",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsNativeEntryApiContractTest::RunTest(const FString& Parameters)
{
    UFunction* Enter = ATsabsPassengerAnchor::StaticClass()->FindFunctionByName(TEXT("DriverEnter"));
    UFunction* Leave = ATsabsPassengerAnchor::StaticClass()->FindFunctionByName(TEXT("Server_DriverLeave"));
    if (!TestNotNull(TEXT("Inherited native server API remains registered"), Enter) ||
        !TestNotNull(TEXT("Possessed-pawn leave RPC remains registered"), Leave)) return false;
    TestTrue(TEXT("Blueprint entry retains native authority-only contract"), Enter->HasAnyFunctionFlags(FUNC_BlueprintAuthorityOnly));
    TestFalse(TEXT("Native C++/Blueprint entry is not a client-to-server entry RPC"), Enter->HasAnyFunctionFlags(FUNC_Net));
    TestTrue(TEXT("Manual exit retains native reliable server RPC"), Leave->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer | FUNC_NetReliable));
    TestNull(TEXT("No added client-selected entry RPC"), ATsabsPassengerAnchor::StaticClass()->FindFunctionByName(TEXT("Server_DriverEnter")));
    return true; // Reflection/metadata only, not a real RPC or save-load test.
}
#endif
