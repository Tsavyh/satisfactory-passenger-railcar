#include "TsabsPassengerHUD.h"
#include "TsabsPassengerAnchor.h"
#include "TsabsPassengerWagonLog.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
#include "FGInputLibrary.h"
#include "PassengerAccessPolicy.h"
#include "FGPlayerController.h"
#include "FGHUD.h"
#include "UI/FGGameUI.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Components/TextBlock.h"
#include "PassengerNativeSpeedometer.h"
#include "TsabsPassengerRailcar.h"
#include "FGRailroadVehicleMovementComponent.h"
#include "PassengerCrosshair.h"

UTsabsPassengerHUD::UTsabsPassengerHUD(const FObjectInitializer& Initializer) : Super(Initializer)
{
    SetIsFocusable(false);
    // Do not resolve game Blueprints from the CDO during pre-engine startup.
    // Their transitive Niagara dependencies can precede module initialization.
}

ATsabsPassengerAnchor* UTsabsPassengerHUD::GetPassengerAnchor() const
{
    return PassengerAnchor.Get();
}

void UTsabsPassengerHUD::BeginCrosshairSuppression()
{
    APlayerController* PC = GetOwningPlayer();
    AFGHUD* HUD = IsValid(PC) ? Cast<AFGHUD>(PC->GetHUD()) : nullptr;
    ATsabsPassengerAnchor* Anchor = PassengerAnchor.Get();
    if (!IsValid(PC) || !PC->IsLocalController() || !IsValid(HUD) || !IsValid(Anchor) ||
        PC->GetPawn() != Anchor || HUD->GetPawnHUD() != this)
    {
        EndCrosshairSuppression();
        return;
    }
    if (CrosshairToken && CrosshairHUD.Get() == HUD && CrosshairController.Get() == PC) return;
    EndCrosshairSuppression();
    CrosshairHUD = HUD;
    CrosshairController = PC;
    CrosshairToken = TsabsPassenger::RegisterPassengerCrosshair(this);
    PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &UTsabsPassengerHUD::OnPassengerPawnChanged);
    // Request a presentation refresh, not a preference or force-hide mutation.
    // SDK .cpp is a stub; real native event dispatch remains a runtime gate.
    HUD->SetCrosshairState(HUD->GetCrosshairState());
}

bool UTsabsPassengerHUD::ShouldSuppressPassengerCrosshair(const FMulticastScriptDelegate& Delegate) const
{
    APlayerController* PC = CrosshairController.Get();
    AFGHUD* HUD = CrosshairHUD.Get();
    ATsabsPassengerAnchor* Anchor = PassengerAnchor.Get();
    return CrosshairToken && IsValid(PC) && PC == GetOwningPlayer() && PC->IsLocalController() &&
        IsValid(Anchor) && PC->GetPawn() == Anchor && IsValid(HUD) && PC->GetHUD() == HUD &&
        HUD->GetPawnHUD() == this && &Delegate == &HUD->mOnCrosshairUpdated;
}

void UTsabsPassengerHUD::EndCrosshairSuppression()
{
    if (!CrosshairToken) return;
    APlayerController* PC = CrosshairController.Get();
    AFGHUD* HUD = CrosshairHUD.Get();
    const uint64 OwnToken = CrosshairToken;
    CrosshairToken = 0; // Retire before callbacks/refresh; never touch a replacement token.
    TsabsPassenger::UnregisterPassengerCrosshair(OwnToken);
    if (IsValid(PC)) PC->OnPossessedPawnChanged.RemoveDynamic(this, &UTsabsPassengerHUD::OnPassengerPawnChanged);
    CrosshairController.Reset();
    CrosshairHUD.Reset();
    if (IsValid(PC) && PC->IsLocalController() && IsValid(HUD) && PC->GetHUD() == HUD)
        HUD->SetCrosshairState(HUD->GetCrosshairState());
}

void UTsabsPassengerHUD::OnPassengerPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
    if (NewPawn == PassengerAnchor.Get()) return;
    EndCrosshairSuppression();
    RefreshPassengerSpeed();
    RemoveFromParent();
}

void UTsabsPassengerHUD::NativeDestruct()
{
    EndCrosshairSuppression();
    Super::NativeDestruct();
}

FText UTsabsPassengerHUD::FormatExitPrompt(const FText& LeaveKey, bool bWithinSpeedLimit)
{
    if (!bWithinSpeedLimit) return FText::Format(NSLOCTEXT("TsabsPassenger", "LeaveSpeedLimit",
        "El tren debe ir a menos de {0} km/h para bajar"), FText::AsNumber(TsabsPassenger::PassengerAccessSpeedLimitKmh));
    if (LeaveKey.IsEmpty()) return NSLOCTEXT("TsabsPassenger", "LeaveStandardUnmapped",
        "Bajar del vagón: acción sin tecla asignada");
    return FText::Format(NSLOCTEXT("TsabsPassenger", "LeaveStandardKey",
        "Pulsa <Key>{0}</> para bajar del vagón"), LeaveKey);
}

FText UTsabsPassengerHUD::FormatPassengerSpeed(float SpeedCmPerSecond, bool bAvailable)
{
    const uint32 Magnitude = std::bit_cast<uint32>(SpeedCmPerSecond) & 0x7fffffffu;
    if (!bAvailable || Magnitude >= 0x7f800000u)
        return NSLOCTEXT("TsabsPassenger", "SpeedUnavailable", "—");
    FNumberFormattingOptions Options;
    Options.SetMaximumFractionalDigits(0);
    Options.SetUseGrouping(false);
    return FText::AsNumber(static_cast<double>(std::bit_cast<float>(Magnitude)) * 0.036, &Options);
}

bool UTsabsPassengerHUD::CopyNativeInteractionLayout(UCanvasPanelSlot* Destination) const
{
    if (!Destination) return false;
    APlayerController* PC = GetOwningPlayer();
    AFGHUD* HUD = IsValid(PC) ? Cast<AFGHUD>(PC->GetHUD()) : nullptr;
    UFGGameUI* GameUI = IsValid(HUD) ? HUD->GetGameUI() : nullptr;
    if (!IsValid(GameUI)) return false;
    // The normal interaction is inside a VerticalBox. Its first canvas ancestor
    // carries the screen position. Read it; never mutate the shared normal HUD.
    UWidget* Native = GameUI->GetWidgetFromName(TEXT("mPlayerInteractionMessages"));
    for (UWidget* Item = Native; Item; Item = Item->GetParent())
    {
        if (const UCanvasPanelSlot* NativeSlot = Cast<UCanvasPanelSlot>(Item->Slot))
        {
            Destination->SetLayout(NativeSlot->GetLayout());
            Destination->SetAutoSize(NativeSlot->GetAutoSize());
            return true;
        }
    }
    return false;
}

void UTsabsPassengerHUD::InitializeSpeedometer(UCanvasPanel* Canvas)
{
    auto* Class = Cast<UWidgetBlueprintGeneratedClass>(LoadClass<UUserWidget>(nullptr,
        TEXT("/Game/FactoryGame/Buildable/Vehicle/Train/Locomotive/UI/Widget_Locomotive_HUD.Widget_Locomotive_HUD_C")));
    auto* TreeOwner = Class ? Class->FindWidgetTreeOwningClass() : nullptr;
    SpeedText = TsabsPassenger::ComposeNativeSpeedometer(WidgetTree, Canvas, TreeOwner ? TreeOwner->GetWidgetTreeArchetype() : nullptr);
    if (!SpeedText)
    {
        UE_LOG(LogTsabsPassengerWagon, Warning, TEXT("TSABS_PASSENGER_HUD speed template contract unavailable; speed display omitted; no native HUD changed"));
        return;
    }
    RefreshPassengerSpeed();
}

void UTsabsPassengerHUD::RefreshPassengerSpeed()
{
    if (!SpeedText) return;
    ATsabsPassengerAnchor* Anchor = PassengerAnchor.Get();
    auto* Car = Anchor ? Cast<ATsabsPassengerRailcar>(Anchor->GetOuterVehicle()) : nullptr;
    auto* Movement = IsValid(Car) ? Car->GetRailroadVehicleMovementComponent() : nullptr;
    const bool bValid = IsValid(Anchor) && GetOwningPlayerPawn() == Anchor && IsValid(Car) && !Car->IsTemplate() &&
        Car->GetWorld() == Anchor->GetWorld() && Anchor->GetAttachParentActor() == Car && IsValid(Movement);
    const FText Current = FormatPassengerSpeed(bValid ? Movement->GetForwardSpeed() : 0.0f, bValid);
    // Compare actual text instead of caching only rounded speed: culture changes,
    // invalid->valid recovery and external style synchronization remain coherent.
    if (!SpeedText->GetText().EqualTo(Current)) SpeedText->SetText(Current);
}

void UTsabsPassengerHUD::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    PassengerAnchor = Cast<ATsabsPassengerAnchor>(GetOwningPlayerPawn());
    if (!PassengerAnchor.IsValid() || !WidgetTree) return;
    // Resolve only for the local possessed widget, after normal engine startup.
    InteractionWidgetClass = LoadClass<UUserWidget>(nullptr,
        TEXT("/Game/FactoryGame/Interface/UI/InGame/Widget_PlayerInteraction.Widget_PlayerInteraction_C"));
    LeaveAction = LoadObject<UInputAction>(nullptr,
        TEXT("/Game/FactoryGame/Inputs/Vehicles/Actions/IA_Vehicles_Leave.IA_Vehicles_Leave"));
    if (!InteractionWidgetClass) return;
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PassengerInteractionCanvas"));
    WidgetTree->RootWidget = Canvas;
    SetVisibility(ESlateVisibility::HitTestInvisible);
    InitializeSpeedometer(Canvas);
    InteractionWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), InteractionWidgetClass);
    if (!InteractionWidget)
    {
        UE_LOG(LogTsabsPassengerWagon, Warning, TEXT("TSABS_PASSENGER_HUD standard interaction widget unavailable"));
        return;
    }
    InteractionWidget->SetIsFocusable(false);
    UCanvasPanelSlot* PromptSlot = Canvas->AddChildToCanvas(InteractionWidget);
    // Fallback is the serialized BP_GameUI MidScreenMessageVerticalBox slot;
    // real gameplay copies the current native layout instead of this fallback.
    PromptSlot->SetAnchors(FAnchors(0.5f, 0.5f));
    PromptSlot->SetAlignment(FVector2D(0.5, 0.0));
    PromptSlot->SetOffsets(FMargin(0.0f, 45.0f, 0.0f, 0.0f));
    PromptSlot->SetAutoSize(true);
    const bool bNativeLayout = CopyNativeInteractionLayout(PromptSlot);
    RefreshExitHint(true);
    UE_LOG(LogTsabsPassengerWagon, Display,
        TEXT("TSABS_PASSENGER_HUD standard interaction initialized native_layout=%d"), bNativeLayout);
}

void UTsabsPassengerHUD::NativeConstruct()
{
    Super::NativeConstruct();
    // Rebuilds can reuse this UObject without another AddPawnHUD/input setup.
    // Begin is idempotent and accepts only the exact current local pawn HUD.
    BeginCrosshairSuppression();
    // Apply after the stock child's Construct graph as well as initialization.
    RefreshExitHint(true);
}

FText UTsabsPassengerHUD::ResolveLeaveKey() const
{
    APlayerController* PC = GetOwningPlayer();
    const UEnhancedPlayerInput* Input = IsValid(PC) ? Cast<UEnhancedPlayerInput>(PC->PlayerInput) : nullptr;
    if (!Input || !IsValid(LeaveAction)) return FText::GetEmpty();
    const AFGPlayerController* FGPC = Cast<AFGPlayerController>(PC);
    const bool bGamepad = FGPC && FGPC->GetPlayerInputDeviceType() != EInputDeviceType::MouseAndKeyboard;
    for (const FEnhancedActionKeyMapping& Mapping : Input->GetEnhancedActionMappings())
    {
        if (Mapping.Action != LeaveAction || !Mapping.Key.IsValid() || Mapping.Key.IsGamepadKey() != bGamepad) continue;
        FKey Primary = Mapping.Key;
        TArray<FKey> Modifiers;
        const FName MappingName = Mapping.GetMappingName();
        if (!MappingName.IsNone())
        {
            FKey Remapped;
            if (UFGInputLibrary::GetCurrentMappingForAction(PC, MappingName, Remapped, Modifiers))
            {
                if (!Remapped.IsValid()) continue;
                Primary = Remapped;
            }
        }
        auto KeyText = [](const FKey& Key)
        {
            FText Short = UFGInputLibrary::GetAbbreviatedKeyName(Key);
            return Short.IsEmpty() ? Key.GetDisplayName() : Short;
        };
        FText Result = FText::GetEmpty();
        for (const FKey& Modifier : Modifiers)
        {
            if (!Modifier.IsValid()) continue;
            Result = Result.IsEmpty() ? KeyText(Modifier) : FText::Format(
                NSLOCTEXT("TsabsPassenger", "LeaveKeyChord", "{0} + {1}"), Result, KeyText(Modifier));
        }
        return Result.IsEmpty() ? KeyText(Primary) : FText::Format(
            NSLOCTEXT("TsabsPassenger", "LeaveKeyChord", "{0} + {1}"), Result, KeyText(Primary));
    }
    // Do not invent E or display a generic controller X for an unmapped action.
    return FText::GetEmpty();
}

bool UTsabsPassengerHUD::ApplyInteractionText(const FText& Text)
{
    if (!IsValid(InteractionWidget)) return false;
    UFunction* Setter = InteractionWidget->FindFunction(TEXT("SetOverwriteInteractionText"));
    FTextProperty* TextParam = Setter ? FindFProperty<FTextProperty>(Setter, TEXT("Text")) : nullptr;
    FBoolProperty* FlagParam = Setter ? FindFProperty<FBoolProperty>(Setter, TEXT("ShouldOverwriteText")) : nullptr;
    if (!Setter || !TextParam || !FlagParam || Setter->NumParms != 2)
    {
        if (!bReportedMissingContract)
        {
            bReportedMissingContract = true;
            UE_LOG(LogTsabsPassengerWagon, Warning, TEXT("TSABS_PASSENGER_HUD standard text contract missing; no global HUD modified"));
        }
        return false;
    }
    FStructOnScope Params(Setter);
    TextParam->SetPropertyValue_InContainer(Params.GetStructMemory(), Text);
    FlagParam->SetPropertyValue_InContainer(Params.GetStructMemory(), true);
    InteractionWidget->ProcessEvent(Setter, Params.GetStructMemory());
    return true;
}

void UTsabsPassengerHUD::NotifyExitSpeedRejected()
{
    ATsabsPassengerAnchor* Anchor = PassengerAnchor.Get();
    // A delayed rejection cannot replace another pawn's HUD or outlive slowing.
    if (!IsValid(Anchor) || GetOwningPlayerPawn() != Anchor || Anchor->IsWithinPassengerAccessSpeedLimit()) return;
    ExitSpeedWarningSecondsRemaining = 3.0f;
    RefreshExitHint(true);
}

void UTsabsPassengerHUD::RefreshExitHint(bool bForce)
{
    ATsabsPassengerAnchor* Anchor = PassengerAnchor.Get();
    if (!IsValid(Anchor) || !IsValid(InteractionWidget)) return;
    // Only advertise Leave while the strict speed policy allows it. Keep this
    // parent HUD ticking: collapsing the whole HUD could prevent reappearance.
    const bool bWithinSpeedLimit = Anchor->IsWithinPassengerAccessSpeedLimit();
    if (bWithinSpeedLimit) ExitSpeedWarningSecondsRemaining = 0.0f;
    const bool bShowRejection = ExitSpeedWarningSecondsRemaining > 0.0f;
    const bool bShowPrompt = bWithinSpeedLimit || bShowRejection;
    InteractionWidget->SetVisibility(bShowPrompt ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    const FText Prompt = bShowPrompt ? FormatExitPrompt(ResolveLeaveKey(), !bShowRejection) : FText::GetEmpty();
    if (!bForce && bHasPrompt && LastPrompt.EqualTo(Prompt)) return;
    if (ApplyInteractionText(Prompt))
    {
        LastPrompt = Prompt;
        bHasPrompt = true;
    }
}

void UTsabsPassengerHUD::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    RefreshPassengerSpeed();
    if (!PassengerAnchor.IsValid() || GetOwningPlayerPawn() != PassengerAnchor.Get())
    {
        EndCrosshairSuppression();
        RemoveFromParent();
        return;
    }
    if (FMath::IsFinite(DeltaTime) && DeltaTime > 0.0f)
        ExitSpeedWarningSecondsRemaining = FMath::Max(0.0f, ExitSpeedWarningSecondsRemaining - DeltaTime);
    RefreshExitHint();
}

