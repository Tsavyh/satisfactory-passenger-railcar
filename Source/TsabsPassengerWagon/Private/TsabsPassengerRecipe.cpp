#include "TsabsPassengerRecipe.h"
#include "TsabsPassengerDescriptor.h"
#include "Resources/FGItemDescriptor.h"
#include "UObject/ConstructorHelpers.h"

UTsabsPassengerRecipe::UTsabsPassengerRecipe()
{
    static ConstructorHelpers::FClassFinder<UFGItemDescriptor> ModularFrame(TEXT("/Game/FactoryGame/Resource/Parts/ModularFrame/Desc_ModularFrame.Desc_ModularFrame_C"));
    static ConstructorHelpers::FClassFinder<UFGItemDescriptor> SteelPipe(TEXT("/Game/FactoryGame/Resource/Parts/SteelPipe/Desc_SteelPipe.Desc_SteelPipe_C"));
    static ConstructorHelpers::FClassFinder<UFGItemDescriptor> SteelBeam(TEXT("/Game/FactoryGame/Resource/Parts/SteelPlate/Desc_SteelPlate.Desc_SteelPlate_C"));
    if (!ModularFrame.Class || !SteelPipe.Class || !SteelBeam.Class) {
        UE_LOG(LogTemp, Fatal, TEXT("Passenger recipe requires all three native ingredient classes; refusing an invalid construction cost."));
    }
    mIngredients = {
        FItemAmount(ModularFrame.Class, 5),
        FItemAmount(SteelPipe.Class, 10),
        FItemAmount(SteelBeam.Class, 10)
    };
    mProduct = { FItemAmount(UTsabsPassengerDescriptor::StaticClass(), 1) };
    mProducedIn = { TSoftClassPtr<UObject>(FSoftObjectPath(TEXT("/Game/FactoryGame/Equipment/BuildGun/BP_BuildGun.BP_BuildGun_C"))) };
    mManufactoringDuration = 1.0f;
    mManualManufacturingMultiplier = 1.0f;
    mRelevantEvents.Empty();
    mMaterialCustomizationRecipe = nullptr;
    mDisplayNameOverride = true;
    mDisplayName = NSLOCTEXT("TsabsPassengerWagon", "TestWagonName", "Vagón de pasajeros");
}
