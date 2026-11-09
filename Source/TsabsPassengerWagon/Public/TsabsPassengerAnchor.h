#pragma once

#include "CoreMinimal.h"
#include "FGPassengerSeat.h"
#include "TsabsPassengerAnchor.generated.h"

/** Invisible possession anchor. No interior, chair, driving or inventory. */
UCLASS(NotBlueprintable)
class TSABSPASSENGERWAGON_API ATsabsPassengerAnchor : public AFGPassengerSeat
{
    GENERATED_BODY()
public:
    ATsabsPassengerAnchor();

    /** Server-only: the railcar initializes an EMPTY technical anchor. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Passenger Anchor")
    bool InitializePassengerAnchor(class ATsabsPassengerRailcar* Vehicle, const FTransform& RelativeTransform);

    /** Server-only release. Refuses to detach or recycle an occupied/pending pawn. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Passenger Anchor")
    bool ResetPassengerAnchor();

    /** Valid attached outer car only; no velocity policy or entry authorization. */
    class ATsabsPassengerRailcar* GetPassengerRailcar() const;

    /** Chassis-derived candidate + native safe-exit query, no teleport or occupant mutation. */
    bool TryGetPassengerExitOffset(AFGCharacterPlayer* Character, FVector& OutOffset);

    // Driver is the SDK's term for the passenger possessing THIS anchor only.
    virtual bool CanDriverEnter(AFGCharacterPlayer* Character) override;
    virtual bool DriverEnter(AFGCharacterPlayer* Character) override;
    virtual bool CanLeaveVehicle(AFGCharacterPlayer* Character) override;
    virtual bool DriverLeave(bool bKeepDriving = false) override;
    virtual void Server_DriverLeave_Implementation() override;

    virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;
    virtual void UnPossessed() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Native exit queries need chassis bounds even though this pilot has no collision. */
    virtual FBox GetComponentsBoundingBox(bool bNonColliding = false, bool bIncludeFromChildActors = false) const override;

    /** Local-only orbit input, no character movement or train controls. */
    UFUNCTION()
    void ApplyOrbitInput(FVector2D Delta);

protected:
    virtual void SetupPlayerInputComponent(UInputComponent* InInputComponent) override;
    // Deliberately do not call Super: only these two passenger actions are bound.
    virtual void AddInputBindings(UInputComponent* InInputComponent) override;

private:
    void RemovePassengerHUD();
    TWeakObjectPtr<class UTsabsPassengerHUD> PassengerHUD;
    // Bounded diagnostics: at most one line per exit outcome for each pilot.
    uint8 ExitDiagnosticStates = 0;
    uint8 CameraDiagnosticSamples = 0;
    bool bLocomotiveFramingUsed = false;
    UPROPERTY()
    TSubclassOf<class AFGLocomotive> LocomotiveCameraReference;
    void InputLook(const struct FInputActionValue& Value);
    void InputLeave(const struct FInputActionValue& Value);
    UPROPERTY()
    TObjectPtr<class UInputAction> LookAction;
    UPROPERTY()
    TObjectPtr<class UInputAction> LeaveAction;
    friend class FTsabsAnchorInputTest;
    UPROPERTY(VisibleDefaultsOnly, Category="Passenger Camera")
    TObjectPtr<class USpringArmComponent> OrbitArm;
    UPROPERTY(VisibleDefaultsOnly, Category="Passenger Camera")
    TObjectPtr<class UCameraComponent> OrbitCamera;
};
