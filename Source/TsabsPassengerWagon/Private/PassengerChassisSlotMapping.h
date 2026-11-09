#pragma once

#include "CoreMinimal.h"

// Aliases are valid only for the exact native freight mesh observed at runtime.
// Other assets and unknown slots retain the existing strict name lookup.
inline FName PassengerChassisNativeSlot(FName NativeAssetPath, FName VisualSlot)
{
    static const FName FreightAsset(TEXT("/Game/FactoryGame/Buildable/Vehicle/Train/Wagon/Mesh/SK_Freight_Wagon.SK_Freight_Wagon"));
    if (NativeAssetPath != FreightAsset) return VisualSlot;
    if (VisualSlot == FName(TEXT("MI_Train"))) return FName(TEXT("M_TrainEngine"));
    if (VisualSlot == FName(TEXT("DecalColor_Masked"))) return FName(TEXT("Decal_Color"));
    if (VisualSlot == FName(TEXT("MI_Locomotive"))) return FName(TEXT("Factory_Inst"));
    return VisualSlot;
}
