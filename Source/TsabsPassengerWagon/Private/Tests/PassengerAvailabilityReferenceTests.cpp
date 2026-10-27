#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "FGRecipe.h"
#include "FGRecipeManager.h"
#include "TsabsPassengerWorldModule.h"
#include "UObject/UnrealType.h"
#include <type_traits>

// These are declaration contracts only; the SDK RecipeManager .cpp bodies are stubs.
static_assert(std::is_same_v<decltype(&AFGRecipeManager::AddAvailableRecipes),
    void (AFGRecipeManager::*)(TArray<TSubclassOf<UFGRecipe>>)>);
static_assert(std::is_same_v<decltype(&AFGRecipeManager::RemoveAvailableRecipes),
    void (AFGRecipeManager::*)(const TArray<TSubclassOf<UFGRecipe>>&)>);

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsAvailabilityReferenceTest,
    "Tsabs.Passenger.Content.NativeFreightReferenceAndTransientState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsAvailabilityReferenceTest::RunTest(const FString& Parameters)
{
    // Test-only class resolution. No claim about native placeholder CDO balance or unlocks.
    UClass* Native = LoadClass<UFGRecipe>(nullptr,
        TEXT("/Game/FactoryGame/Recipes/Vehicle/Train/Recipe_FreightWagon.Recipe_FreightWagon_C"));
    TestNotNull(TEXT("Authoritative native Freight path resolves as recipe class"), Native);
    if (Native) TestTrue(TEXT("Resolved native class derives from FGRecipe"), Native->IsChildOf(UFGRecipe::StaticClass()));
    FClassProperty* Reference = FindFProperty<FClassProperty>(UTsabsPassengerWorldModule::StaticClass(), TEXT("NativeFreightRecipe"));
    TestNotNull(TEXT("Runtime reference is GC tracked"), Reference);
    if (Reference)
    {
        TestTrue(TEXT("Runtime reference is transient"), Reference->HasAnyPropertyFlags(CPF_Transient));
        TestFalse(TEXT("No saved global availability toggle"), Reference->HasAnyPropertyFlags(CPF_SaveGame));
        UTsabsPassengerWorldModule* Module = NewObject<UTsabsPassengerWorldModule>();
        TestNull(TEXT("Constructor does not load native Blueprint"), Reference->GetObjectPropertyValue_InContainer(Module));
        TestFalse(TEXT("Fresh instance is not postinitialized"), Module->GetContentState().bPostInitialized);
        TestFalse(TEXT("Fresh instance is not reconciling"), Module->GetContentState().bReconciling);
        TestFalse(TEXT("Fresh instance is not tearing down"), Module->GetContentState().bTearingDown);
    }
    return true;
}
#endif
