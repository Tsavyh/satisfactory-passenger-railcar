#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TsabsPassengerHUD.generated.h"

class ATsabsPassengerAnchor;
class UInputAction;
class UCanvasPanelSlot;

/** Local standard interaction prompt; never binds or consumes controls. */
UCLASS(NotBlueprintable)
class TSABSPASSENGERWAGON_API UTsabsPassengerHUD : public UUserWidget
{
    GENERATED_BODY()
public:
    UTsabsPassengerHUD(const FObjectInitializer& Initializer);
    UFUNCTION(BlueprintPure, Category="Passenger Interface")
    static FText FormatExitPrompt(const FText& LeaveKey, bool bWithinSpeedLimit);
    /** Informational display only; never used by access guards. */
    UFUNCTION(BlueprintPure, Category="Passenger Interface")
    static FText FormatPassengerSpeed(float SpeedCmPerSecond, bool bAvailable);
    ATsabsPassengerAnchor* GetPassengerAnchor() const;
    /** Called only AFTER AddPawnHUD has assigned the exact owned widget. */
    void BeginCrosshairSuppression();
    void EndCrosshairSuppression();
    bool ShouldSuppressPassengerCrosshair(const FMulticastScriptDelegate& Delegate) const;
    /** Event-driven local feedback, never an availability/exit authority decision. */
    UFUNCTION()
    void NotifyExitSpeedRejected();
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual void NativeDestruct() override;
private:
    friend class FTsabsPassengerCrosshairTest;
    UFUNCTION()
    void OnPassengerPawnChanged(APawn* OldPawn, APawn* NewPawn);
    TWeakObjectPtr<class AFGHUD> CrosshairHUD;
    TWeakObjectPtr<APlayerController> CrosshairController;
    uint64 CrosshairToken = 0;
    friend class FTsabsResolvedExitKeyTest;
    friend class FTsabsPassengerSpeedLifecycleTest;
    void InitializeSpeedometer(class UCanvasPanel* Canvas);
    void RefreshPassengerSpeed();
    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> SpeedText;
    void RefreshExitHint(bool bForce = false);
    FText ResolveLeaveKey() const;
    bool ApplyInteractionText(const FText& Text);
    bool CopyNativeInteractionLayout(UCanvasPanelSlot* Destination) const;
    UPROPERTY()
    TSubclassOf<UUserWidget> InteractionWidgetClass;
    UPROPERTY()
    TObjectPtr<UInputAction> LeaveAction;
    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> InteractionWidget;
    TWeakObjectPtr<ATsabsPassengerAnchor> PassengerAnchor;
    FText LastPrompt;
    bool bHasPrompt = false;
    bool bReportedMissingContract = false;
    float ExitSpeedWarningSecondsRemaining = 0.0f;
};

