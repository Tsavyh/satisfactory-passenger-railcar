#include "TsabsPassengerDescriptor.h"
#include "TsabsPassengerRailcar.h"
#include "FGBuildCategory.h"
#include "FGBuildSubCategory.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

UTsabsPassengerDescriptor::UTsabsPassengerDescriptor()
{
    mVehicleClass = ATsabsPassengerRailcar::StaticClass();
    static ConstructorHelpers::FClassFinder<UFGBuildCategory> Category(TEXT("/Game/FactoryGame/Interface/UI/InGame/BuildMenu/BuildCategories/BC_Vehicles"));
    static ConstructorHelpers::FClassFinder<UFGBuildSubCategory> Trains(TEXT("/Game/FactoryGame/Interface/UI/InGame/BuildMenu/BuildCategories/Sub_Transport/SC_Trains"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> Icon(TEXT("/Game/FactoryGame/Buildable/Vehicle/Train/Wagon/UI/FreightCar_512.FreightCar_512"));
    mCategory = Category.Class;
    mSubCategories = { Trains.Class };
    mMenuPriority = 51.0f;
    mSmallIcon = Icon.Object;
    mPersistentBigIcon = Icon.Object;
}

FText UTsabsPassengerDescriptor::GetItemNameInternal() const
{
    return NSLOCTEXT("TsabsPassengerWagon", "TestWagonName", "Vagón de pasajeros");
}
FString UTsabsPassengerDescriptor::GetItemNameInternalAsString() const
{
    return GetItemNameInternal().ToString();
}
FText UTsabsPassengerDescriptor::GetItemDescriptionInternal() const
{
    return NSLOCTEXT("TsabsPassengerWagon", "TestWagonDescription", "Vagón para viajar como pasajero sin controlar el tren. Cámara exterior giratoria, sin inventario de carga. Capacidad actual: 1 pasajero. Solo se puede subir y bajar a menos de 30 km/h.");
}
