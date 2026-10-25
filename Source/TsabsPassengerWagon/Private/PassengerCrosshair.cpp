#include "PassengerCrosshair.h"
#include "TsabsPassengerHUD.h"

namespace
{
    TMap<uint64, TWeakObjectPtr<UTsabsPassengerHUD>> Registrations;
    uint64 NextToken = 0;
    FDelegateHandle HookHandle;
}

uint64 TsabsPassenger::RegisterPassengerCrosshair(UTsabsPassengerHUD* Widget)
{
    check(IsInGameThread());
    for (auto It = Registrations.CreateIterator(); It; ++It)
        if (!It.Value().IsValid()) It.RemoveCurrent();
    if (!IsValid(Widget)) return 0;
    const uint64 Token = ++NextToken;
    Registrations.Add(Token, Widget);
    return Token;
}

void TsabsPassenger::UnregisterPassengerCrosshair(uint64 Token)
{
    check(IsInGameThread());
    if (Token) Registrations.Remove(Token);
}

void TsabsPassenger::ForwardPassengerCrosshair(TCallScope<FCrosshairUpdateSignature>& Scope,
    const FMulticastScriptDelegate& Delegate, bool bVisible, UTexture2D* Texture, FLinearColor Color)
{
    // Never rewrite HUD preference/force-hide flags. Only narrow this exact
    // local passenger's presentation event. Pass through any hidden input.
    if (bVisible && IsInGameThread())
        for (const auto& Entry : Registrations)
            if (const UTsabsPassengerHUD* Widget = Entry.Value.Get())
                if (Widget->ShouldSuppressPassengerCrosshair(Delegate)) { bVisible = false; break; }
    // Same SML scope, same delegate reference, texture and color; do not Cancel
    // or bypass other hook subscribers. The scope prevents double forwarding.
    Scope(Delegate, bVisible, Texture, Color);
}

void TsabsPassenger::InstallPassengerCrosshairHook()
{
    // The stub editor is deliberately never detoured. Compile the exact macro
    // and ABI in Editor too, but execute the subscription only in game builds.
    // Widget tokens never install/reinstall this one module-owned hook.
    if (!WITH_EDITOR && !HookHandle.IsValid())
        HookHandle = SUBSCRIBE_METHOD(FOnCrosshairUpdated_DelegateWrapper, &TsabsPassenger::ForwardPassengerCrosshair);
}

void TsabsPassenger::UninstallPassengerCrosshairHook()
{
    if (!WITH_EDITOR && HookHandle.IsValid())
    {
        UNSUBSCRIBE_METHOD(FOnCrosshairUpdated_DelegateWrapper, HookHandle);
        HookHandle.Reset();
    }
    Registrations.Empty();
}
