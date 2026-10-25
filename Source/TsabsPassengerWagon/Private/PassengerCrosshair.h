#pragma once
#include "CoreMinimal.h"
#include "FGHUD.h"
#include "Patching/NativeHookManager.h"
class UTsabsPassengerHUD;
namespace TsabsPassenger
{
    using FCrosshairUpdateSignature = void(*)(const FMulticastScriptDelegate&, bool, UTexture2D*, FLinearColor);
    static_assert(std::is_same_v<decltype(&FOnCrosshairUpdated_DelegateWrapper), FCrosshairUpdateSignature>, "Native UHT wrapper ABI must match exactly");
    void InstallPassengerCrosshairHook();
    void UninstallPassengerCrosshairHook();
    uint64 RegisterPassengerCrosshair(UTsabsPassengerHUD* Widget);
    void UnregisterPassengerCrosshair(uint64 Token);
    void ForwardPassengerCrosshair(TCallScope<FCrosshairUpdateSignature>& Scope,
        const FMulticastScriptDelegate& Delegate, bool bVisible, UTexture2D* Texture, FLinearColor Color);
}
