#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"
#include "Resources/FGVehicleDescriptor.h"
#include "FGBuildCategory.h"
#include "FGBuildSubCategory.h"
#include "Engine/Texture2D.h"
#include "TsabsPassengerRailcar.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsDescriptorTest, "Tsabs.Passenger.Content.NativeDescriptor", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsDescriptorTest::RunTest(const FString& Parameters)
{
    UClass* Class = LoadClass<UFGVehicleDescriptor>(nullptr, TEXT("/Script/TsabsPassengerWagon.TsabsPassengerDescriptor"));
    if (!TestNotNull(TEXT("Concrete native passenger vehicle descriptor exists"), Class)) return false;
    TestTrue(TEXT("Native concrete descriptor"), Class->HasAnyClassFlags(CLASS_Native) && !Class->HasAnyClassFlags(CLASS_Abstract));
    const UFGVehicleDescriptor* Descriptor = Class->GetDefaultObject<UFGVehicleDescriptor>();
    TestEqual(TEXT("Describes existing passenger car, not freight"), Descriptor->mVehicleClass.Get(), ATsabsPassengerRailcar::StaticClass());
    TestEqual(TEXT("Name uses real virtual instance accessor"), Descriptor->GetItemNameFromInstance().ToString(), FString(TEXT("Vagón de pasajeros")));
    TestEqual(TEXT("Description reflects accepted one-passenger behavior without trial claims"), Descriptor->GetItemDescriptionFromInstance().ToString(), FString(TEXT("Vagón para viajar como pasajero sin controlar el tren. Cámara exterior giratoria, sin inventario de carga. Capacidad actual: 1 pasajero. Solo se puede subir y bajar a menos de 30 km/h.")));
    UClass* Category = Descriptor->GetCategoryFromInstance();
    TestNotNull(TEXT("Category resolves"), Category);
    if (Category) {
        TestTrue(TEXT("Build category type"), Category->IsChildOf(UFGBuildCategory::StaticClass()));
        TestEqual(TEXT("Existing vehicles category"), Category->GetPathName(), FString(TEXT("/Game/FactoryGame/Interface/UI/InGame/BuildMenu/BuildCategories/BC_Vehicles.BC_Vehicles_C")));
    }
    // Starter asset metadata is a reference contract, not shipping build-menu acceptance.
    UClass* FreightClass = LoadClass<UFGVehicleDescriptor>(nullptr, TEXT("/Game/FactoryGame/Buildable/Vehicle/Train/Wagon/Desc_FreightWagon.Desc_FreightWagon_C"));
    UClass* TrainsClass = LoadClass<UFGBuildSubCategory>(nullptr, TEXT("/Game/FactoryGame/Interface/UI/InGame/BuildMenu/BuildCategories/Sub_Transport/SC_Trains.SC_Trains_C"));
    TestNotNull(TEXT("Native freight descriptor resolves"), FreightClass);
    TestNotNull(TEXT("Railway transport subcategory resolves"), TrainsClass);
    if (TrainsClass) TestTrue(TEXT("Railway transport is a build subcategory"), TrainsClass->IsChildOf(UFGBuildSubCategory::StaticClass()));
    if (FreightClass) {
        const UFGVehicleDescriptor* Freight = FreightClass->GetDefaultObject<UFGVehicleDescriptor>();
        TestEqual(TEXT("Same transport category as native freight"), Descriptor->GetCategoryFromInstance().Get(), Freight->GetCategoryFromInstance().Get());
        TArray<TSubclassOf<UFGCategory>> PassengerSubCategories;
        TArray<TSubclassOf<UFGCategory>> FreightSubCategories;
        Descriptor->GetSubCategoriesFromInstance(PassengerSubCategories);
        Freight->GetSubCategoriesFromInstance(FreightSubCategories);
        TestEqual(TEXT("Passenger has exactly one railway subcategory"), PassengerSubCategories.Num(), 1);
        TestEqual(TEXT("Native freight has exactly one railway subcategory"), FreightSubCategories.Num(), 1);
        TestTrue(TEXT("Passenger subcategories match native freight"), PassengerSubCategories == FreightSubCategories);
        if (PassengerSubCategories.Num() == 1) TestEqual(TEXT("Passenger belongs to railway transport"), PassengerSubCategories[0].Get(), TrainsClass);
        if (FreightSubCategories.Num() == 1) TestEqual(TEXT("Native freight belongs to railway transport"), FreightSubCategories[0].Get(), TrainsClass);
        // The generated SDK GetMenuPriority body is a stub: read the actual CDO property.
        const FFloatProperty* Priority = FindFProperty<FFloatProperty>(Class, TEXT("mMenuPriority"));
        const FFloatProperty* FreightPriority = FindFProperty<FFloatProperty>(FreightClass, TEXT("mMenuPriority"));
        TestNotNull(TEXT("Passenger menu priority float property exists"), Priority);
        TestNotNull(TEXT("Native freight menu priority float property exists"), FreightPriority);
        if (Priority && FreightPriority) {
            const float PassengerOrder = Priority->GetPropertyValue_InContainer(Descriptor);
            const float FreightOrder = FreightPriority->GetPropertyValue_InContainer(Freight);
            TestEqual(TEXT("Native freight priority in CL513885 reference"), FreightOrder, 50.0f);
            TestEqual(TEXT("Passenger menu priority immediately after freight"), PassengerOrder, 51.0f);
            TestTrue(TEXT("Passenger sorts after native freight"), PassengerOrder > FreightOrder);
        }
    }
    for (const FName Name : {FName(TEXT("mSmallIcon")), FName(TEXT("mPersistentBigIcon"))}) {
        const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Class, Name);
        if (TestNotNull(TEXT("Icon property exists"), Property)) {
            UObject* Icon = Property->GetObjectPropertyValue_InContainer(Descriptor);
            TestNotNull(TEXT("Icon resolves"), Icon);
            TestTrue(TEXT("Icon is texture, not placeholder default copy"), Icon && Icon->IsA<UTexture2D>());
        }
    }
    return true;
}
#endif
