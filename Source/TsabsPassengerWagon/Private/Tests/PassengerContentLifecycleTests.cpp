#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Module/ModModule.h"
#include "TsabsPassengerWorldModule.h"
#include <type_traits>
#if __has_include("PassengerContentPolicy.h")
#include "PassengerContentPolicy.h"
#define TSABS_HAS_CONTENT_POLICY 1
#else
#define TSABS_HAS_CONTENT_POLICY 0
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsContentLifecycleTest, "Tsabs.Passenger.Content.LifecycleAuthorityPolicy", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsContentLifecycleTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Lifecycle belongs to this root, not inherited base"), (std::is_same_v<decltype(&UTsabsPassengerWorldModule::DispatchLifecycleEvent), void (UTsabsPassengerWorldModule::*)(ELifecyclePhase)>));
#if TSABS_HAS_CONTENT_POLICY
    using namespace TsabsPassenger;
    for (const ELifecyclePhase Phase : { ELifecyclePhase::CONSTRUCTION, ELifecyclePhase::INITIALIZATION, ELifecyclePhase::POST_INITIALIZATION }) {
        TestEqual(TEXT("Registration only INIT, no client authority filter"), ShouldRegisterPassengerRecipe(Phase, true, true, false), Phase == ELifecyclePhase::INITIALIZATION);
        TestFalse(TEXT("No world cannot register"), ShouldRegisterPassengerRecipe(Phase, false, true, false));
        TestFalse(TEXT("No runtime owner cannot register"), ShouldRegisterPassengerRecipe(Phase, true, false, false));
        TestFalse(TEXT("Idempotent registration"), ShouldRegisterPassengerRecipe(Phase, true, true, true));
#if defined(TSABS_AVAILABILITY_POLICY_VERSION)
        for (bool Client : {false, true}) for (bool Authority : {false, true}) {
            const auto Action = DecidePassengerAvailability(Phase, true, true, Client, Authority, true, true, false, true, false);
            TestTrue(TEXT("Freight grant only POSTINIT authority nonclient"), (Action == EPassengerAvailabilityAction::Grant) == (Phase == ELifecyclePhase::POST_INITIALIZATION && !Client && Authority));
            TestTrue(TEXT("Locked Freight never grants"), DecidePassengerAvailability(Phase, true, true, Client, Authority, true, true, false, false, false) == EPassengerAvailabilityAction::None);
        }
#else
        AddError(TEXT("Missing Freight lifecycle decision; legacy unconditional grant is not progression"));
#endif
    }
    UTsabsPassengerWorldModule* First = NewObject<UTsabsPassengerWorldModule>();
    UTsabsPassengerWorldModule* Second = NewObject<UTsabsPassengerWorldModule>();
    TestFalse(TEXT("Fresh world instance registration not claimed"), First->GetContentState().bRegistrationVerified);
    TestFalse(TEXT("Fresh world instance availability not claimed"), First->GetContentState().bAvailabilityVerified);
    AddExpectedError(TEXT("TSABS_CONTENT invalid-context"), EAutomationExpectedErrorFlags::Contains, 2);
    First->DispatchLifecycleEvent(ELifecyclePhase::INITIALIZATION);
    First->DispatchLifecycleEvent(ELifecyclePhase::POST_INITIALIZATION);
    TestFalse(TEXT("No-world call never attempts mutation"), First->GetContentState().bRegistrationAttempted || First->GetContentState().bAvailabilityAttempted);
    TestFalse(TEXT("Another instance is not skipped by static state"), Second->GetContentState().bRegistrationAttempted || Second->GetContentState().bAvailabilityAttempted);
#else
    AddError(TEXT("Production lifecycle authority policy absent"));
#endif
    return true;
}
#endif
