#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "PassengerContentPolicy.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsAvailabilityPolicyTest, "Tsabs.Passenger.Content.FreightAvailabilityDecision", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsAvailabilityPolicyTest::RunTest(const FString& Parameters)
{
#if defined(TSABS_AVAILABILITY_POLICY_VERSION)
    using namespace TsabsPassenger;
    for (bool Native : {false, true}) for (bool Own : {false, true})
    {
        const auto Expected = Native == Own ? EPassengerAvailabilityAction::None : Native ? EPassengerAvailabilityAction::Grant : EPassengerAvailabilityAction::Revoke;
        TestTrue(TEXT("Exact four-state parity"), DecidePassengerAvailability(ELifecyclePhase::POST_INITIALIZATION, true, true, false, true, true, true, false, Native, Own) == Expected);
        for (ELifecyclePhase Phase : {ELifecyclePhase::CONSTRUCTION, ELifecyclePhase::INITIALIZATION})
            TestTrue(TEXT("Not ready never writes"), DecidePassengerAvailability(Phase, true, true, false, true, true, true, false, Native, Own) == EPassengerAvailabilityAction::None);
        for (int32 Invalid = 0; Invalid < 5; ++Invalid)
            TestTrue(TEXT("World/owner/client/authority/reentry guard"), DecidePassengerAvailability(ELifecyclePhase::POST_INITIALIZATION, Invalid != 0, Invalid != 1, Invalid == 2, Invalid != 3, true, true, Invalid == 4, Native, Own) == EPassengerAvailabilityAction::None);
        for (bool Registered : {false, true}) for (bool Reference : {false, true})
        {
            const bool Desired = Native && Registered && Reference;
            const auto FailClosed = Desired == Own ? EPassengerAvailabilityAction::None : Desired ? EPassengerAvailabilityAction::Grant : EPassengerAvailabilityAction::Revoke;
            TestTrue(TEXT("Invalid grant prerequisites revoke ONLY legacy own availability"), DecidePassengerAvailability(ELifecyclePhase::POST_INITIALIZATION, true, true, false, true, Registered, Reference, false, Native, Own) == FailClosed);
        }
    }
#else
    AddError(TEXT("Missing production Freight parity decision: unconditional grant cannot revoke saved legacy availability"));
#endif
    return true;
}
#endif
