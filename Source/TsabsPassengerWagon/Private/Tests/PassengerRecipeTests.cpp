#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"
#include "FGRecipe.h"
#include "FGRecipeProducerInterface.h"
#include "Equipment/FGBuildGun.h"
#include "TsabsPassengerDescriptor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsNativeCostRecipeTest, "Tsabs.Passenger.Content.NativeCostBuildGunRecipe", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsNativeCostRecipeTest::RunTest(const FString& Parameters)
{
    UClass* Class = LoadClass<UFGRecipe>(nullptr, TEXT("/Script/TsabsPassengerWagon.TsabsPassengerRecipe"));
    if (!TestNotNull(TEXT("Native passenger recipe exists"), Class)) return false;
    TestTrue(TEXT("Concrete native recipe"), Class->HasAnyClassFlags(CLASS_Native) && !Class->HasAnyClassFlags(CLASS_Abstract));
    const UFGRecipe* Recipe = Class->GetDefaultObject<UFGRecipe>();
    // The instance accessor is inline raw mIngredients; static SDK GetIngredients is a stub.
    const auto& Ingredients = Recipe->GetIngredients();
    TestEqual(TEXT("Exactly three base ingredients, never free or partial"), Ingredients.Num(), 3);
    const FArrayProperty* RawIngredients = FindFProperty<FArrayProperty>(Class, TEXT("mIngredients"));
    if (TestNotNull(TEXT("Raw ingredient array exists"), RawIngredients)) {
        FScriptArrayHelper Raw(RawIngredients, RawIngredients->ContainerPtrToValuePtr<void>(Recipe));
        TestEqual(TEXT("Raw and inline ingredient counts agree"), Raw.Num(), Ingredients.Num());
        TestEqual(TEXT("Raw recipe has all three costs"), Raw.Num(), 3);
    }
    const TCHAR* Paths[] = {
        TEXT("/Game/FactoryGame/Resource/Parts/ModularFrame/Desc_ModularFrame.Desc_ModularFrame_C"),
        TEXT("/Game/FactoryGame/Resource/Parts/SteelPipe/Desc_SteelPipe.Desc_SteelPipe_C"),
        TEXT("/Game/FactoryGame/Resource/Parts/SteelPlate/Desc_SteelPlate.Desc_SteelPlate_C")
    };
    const int32 Amounts[] = {5, 10, 10}; // Shipped CL513885 Docs, not placeholder freight CDO data.
    for (int32 Index = 0; Index < 3; ++Index) {
        UClass* Item = LoadClass<UFGItemDescriptor>(nullptr, Paths[Index]);
        TestNotNull(FString::Printf(TEXT("Native ingredient resolves: %s"), Paths[Index]), Item);
        if (Item) TestTrue(TEXT("Ingredient is a concrete item descriptor"), Item->IsChildOf(UFGItemDescriptor::StaticClass()) && !Item->HasAnyClassFlags(CLASS_Abstract));
        if (Ingredients.IsValidIndex(Index)) {
            TestNotNull(TEXT("Actual ingredient is not null"), Ingredients[Index].ItemClass.Get());
            TestEqual(TEXT("Exact native ingredient class (normal modular frame / pipe / beam)"), Ingredients[Index].ItemClass.Get(), Item);
            TestEqual(TEXT("Unscaled native base ingredient amount"), Ingredients[Index].Amount, Amounts[Index]);
        }
    }
    const auto& Products = Recipe->GetProducts();
    if (TestEqual(TEXT("One product"), Products.Num(), 1)) {
        TestEqual(TEXT("Product is passenger descriptor"), Products[0].ItemClass.Get(), UTsabsPassengerDescriptor::StaticClass());
        TestEqual(TEXT("One wagon"), Products[0].Amount, 1);
    }
    TArray<TSubclassOf<UObject>> Producers;
    Recipe->GetProducedIn(Producers); // Real soft-class resolving implementation; never IsProducedIn stub.
    if (TestEqual(TEXT("One resolved producer"), Producers.Num(), 1)) {
        UClass* Producer = Producers[0].Get();
        TestNotNull(TEXT("Resolved producer"), Producer);
        TestTrue(TEXT("BuildGun subclass"), Producer && Producer->IsChildOf(AFGBuildGun::StaticClass()));
        TestTrue(TEXT("Recipe producer interface"), Producer && Producer->ImplementsInterface(UFGRecipeProducerInterface::StaticClass()));
        if (Producer) TestEqual(TEXT("Exact existing BuildGun"), Producer->GetPathName(), FString(TEXT("/Game/FactoryGame/Equipment/BuildGun/BP_BuildGun.BP_BuildGun_C")));
    }
    const FArrayProperty* ProducedIn = FindFProperty<FArrayProperty>(Class, TEXT("mProducedIn"));
    if (TestNotNull(TEXT("Soft class array"), ProducedIn)) {
        TestNotNull(TEXT("Array holds soft classes, not hard subclasses"), CastField<FSoftClassProperty>(ProducedIn->Inner));
        FScriptArrayHelper Entries(ProducedIn, ProducedIn->ContainerPtrToValuePtr<void>(Recipe));
        TestEqual(TEXT("Exactly one raw reference (no silently dropped invalid entries)"), Entries.Num(), 1);
    }
    TestEqual(TEXT("Explicit technical duration"), Recipe->GetManufacturingDuration(), 1.0f);
    TestEqual(TEXT("Explicit manual multiplier"), Recipe->GetManualManufacturingDuration(), 1.0f);
    const FArrayProperty* Events = FindFProperty<FArrayProperty>(Class, TEXT("mRelevantEvents"));
    if (TestNotNull(TEXT("Event array"), Events)) TestEqual(TEXT("No seasonal gate"), FScriptArrayHelper(Events, Events->ContainerPtrToValuePtr<void>(Recipe)).Num(), 0);
    const FObjectPropertyBase* Custom = FindFProperty<FObjectPropertyBase>(Class, TEXT("mMaterialCustomizationRecipe"));
    if (TestNotNull(TEXT("Customization field"), Custom)) TestNull(TEXT("No extra material cost recipe"), Custom->GetObjectPropertyValue_InContainer(Recipe));
    return true;
}
#endif
