#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "TsabsPassengerHUD.h"

// Same test exercises the real legacy formatter for RED and real reduced API for GREEN.
template<class T>
static FText NativeFirstExitHint(const FText& Key)
{
    if constexpr (requires { T::FormatExitPrompt(Key); }) return T::FormatExitPrompt(Key);
    else return T::FormatExitPrompt(Key, false);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsPassengerExitHintTest,
    "Tsabs.Passenger.NativeFirst.Interface.ExitPromptUsesNativeKeyWithoutSpeedPolicy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsPassengerExitHintTest::RunTest(const FString& Parameters)
{
    UFunction* Format = UTsabsPassengerHUD::StaticClass()->FindFunctionByName(TEXT("FormatExitPrompt"));
    if (!TestNotNull(TEXT("Own minimal exit-key formatter"), Format)) return false;
    TestEqual(TEXT("Only key and return value, no speed-policy flag"), Format->NumParms, uint8(2));
    for (const TCHAR* Key : {TEXT("E"), TEXT("F"), TEXT("Left Alt + Q")})
    {
        const FString Prompt = NativeFirstExitHint<UTsabsPassengerHUD>(FText::FromString(Key)).ToString();
        TestTrue(TEXT("Actual binding uses standard key capsule markup"), Prompt.Contains(FString::Printf(TEXT("<Key>%s</>"), Key)));
        TestTrue(TEXT("Action describes dismounting"), Prompt.Contains(TEXT("bajar del vagón")));
        TestFalse(TEXT("Normal hint has no invented speed requirement"), Prompt.Contains(TEXT("km/h")));
    }
    TestFalse(TEXT("Unmapped action never invents a binding"),
        NativeFirstExitHint<UTsabsPassengerHUD>(FText::GetEmpty()).ToString().Contains(TEXT("<Key>")));
    return true;
}
#endif
