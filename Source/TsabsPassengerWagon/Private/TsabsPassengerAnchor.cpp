#include "TsabsPassengerAnchor.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "TsabsPassengerSpringArm.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "UObject/ConstructorHelpers.h"
#include "TsabsPassengerRailcar.h"
#include "FGCharacterPlayer.h"
#include "FGPlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "PassengerExitGeometry.h"
#include "TsabsPassengerWagonLog.h"
#include "TsabsPassengerHUD.h"
#include "FGHUD.h"
#include "FGLocomotive.h"
#include "Engine/World.h"

ATsabsPassengerAnchor::ATsabsPassengerAnchor()
{
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("PassengerAnchorRoot")));
    bReplicates = true;
    SetReplicateMovement(true);
    SetActorEnableCollision(false);
    AutoPossessPlayer = EAutoReceiveInput::Disabled;
    AutoPossessAI = EAutoPossessAI::Disabled;
    mShouldAttachDriver = true;
    mIsDriverVisible = false;

    // Provisional exterior framing; requires visual review in the real game.
    OrbitArm = CreateDefaultSubobject<UTsabsPassengerSpringArm>(TEXT("PassengerOrbitArm"));
    OrbitArm->SetupAttachment(GetRootComponent());
    OrbitArm->TargetArmLength = 1600.0f;
    OrbitArm->SetRelativeLocation(FVector(0.0, 0.0, 300.0));
    OrbitArm->SetRelativeRotation(FRotator(-15.0, 0.0, 0.0));
    OrbitArm->bUsePawnControlRotation = false;
    OrbitArm->bDoCollisionTest = true;
    OrbitCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PassengerOrbitCamera"));
    OrbitCamera->SetupAttachment(OrbitArm, USpringArmComponent::SocketName);
    OrbitCamera->bUsePawnControlRotation = false;
    mSpringArmComponent = OrbitArm;
    mCameraComponent = OrbitCamera;

    static ConstructorHelpers::FClassFinder<AFGLocomotive> LocomotiveReference(
        TEXT("/Game/FactoryGame/Buildable/Vehicle/Train/Locomotive/BP_Locomotive"));
    LocomotiveCameraReference = LocomotiveReference.Class;
    if (LocomotiveCameraReference)
    {
        // GetComponents on a CDO misses Blueprint/SCS components. This engine
        // helper reads their actual templates, without spawning a train or
        // executing a locomotive's camera/control Blueprint on a shared actor.
        TArray<const USpringArmComponent*> NativeArms;
        AActor::GetActorClassDefaultComponents<USpringArmComponent>(LocomotiveCameraReference, NativeArms);
        const USpringArmComponent* NativeArm = NativeArms.Num() == 1 ? NativeArms[0] : nullptr;
        for (const USpringArmComponent* Item : NativeArms)
            if (Item->GetName().StartsWith(TEXT("SpringArm"))) { NativeArm = Item; break; }
        if (NativeArm)
            bLocomotiveFramingUsed = CastChecked<UTsabsPassengerSpringArm>(OrbitArm)->CopyLocomotiveFraming(NativeArm);
        TArray<const UCameraComponent*> NativeCameras;
        AActor::GetActorClassDefaultComponents<UCameraComponent>(LocomotiveCameraReference, NativeCameras);
        if (bLocomotiveFramingUsed && NativeCameras.Num() == 1 && !NativeCameras[0]->GetRelativeTransform().ContainsNaN())
            OrbitCamera->SetRelativeTransform(NativeCameras[0]->GetRelativeTransform());
    }

    static ConstructorHelpers::FObjectFinder<UInputAction> LookReference(
        TEXT("/Game/FactoryGame/Inputs/Player/Movement/IA_LookAxis.IA_LookAxis"));
    static ConstructorHelpers::FObjectFinder<UInputAction> LeaveReference(
        TEXT("/Game/FactoryGame/Inputs/Vehicles/Actions/IA_Vehicles_Leave.IA_Vehicles_Leave"));
    static ConstructorHelpers::FObjectFinder<UInputMappingContext> TrainReference(
        TEXT("/Game/FactoryGame/Inputs/Vehicles/Context/MC_Trains.MC_Trains"));
    LookAction = LookReference.Object;
    LeaveAction = LeaveReference.Object;
    mMappingContext = CreateDefaultSubobject<UInputMappingContext>(TEXT("PassengerLookLeaveMapping"));
    bool HasLook = false;
    bool HasLeave = false;
    if (TrainReference.Object)
    {
        for (const FEnhancedActionKeyMapping& Mapping : TrainReference.Object->GetMappings())
        {
            if (Mapping.Action && (Mapping.Action == LookAction || Mapping.Action == LeaveAction))
            {
                // Copy only the allowlisted mappings, preserving original modifiers/settings.
                mMappingContext->MapKey(Mapping.Action, Mapping.Key) = Mapping;
                HasLook |= Mapping.Action == LookAction;
                HasLeave |= Mapping.Action == LeaveAction;
            }
        }
    }
    // Editor placeholders can have no mappings. Conservative keyboard/mouse fallback.
    // Rebinding/gamepad behavior must be verified with real game assets and a player.
    if (LookAction && !HasLook) mMappingContext->MapKey(LookAction, EKeys::Mouse2D);
    if (LeaveAction && !HasLeave) mMappingContext->MapKey(LeaveAction, EKeys::E);
}

bool ATsabsPassengerAnchor::InitializePassengerAnchor(ATsabsPassengerRailcar* Vehicle, const FTransform& RelativeTransform)
{
    if (!HasAuthority() || IsTemplate() || !GetWorld() || !IsValid(Vehicle) || Vehicle->IsTemplate() ||
        Vehicle->GetWorld() != GetWorld() || !Vehicle->HasAuthority() || RelativeTransform.ContainsNaN() ||
        Vehicle->IsBoardingTransactionActive() ||
        GetDriver() || HasPendingDriver() || GetController() ||
        (GetOuterVehicle() && GetOuterVehicle() != Vehicle) ||
        (GetAttachParentActor() && GetAttachParentActor() != Vehicle)) return false;
    if (!AttachToActor(Vehicle, FAttachmentTransformRules::KeepRelativeTransform)) return false;
    SetActorRelativeTransform(RelativeTransform);
    mOuterVehicle = Vehicle; // Inherited replicated reference; do not replicate a parallel driver/outer state.
    SetOwner(Vehicle);
    ForceNetUpdate();
    return true;
}

bool ATsabsPassengerAnchor::ResetPassengerAnchor()
{
    if (!HasAuthority() || IsTemplate() || GetDriver() || HasPendingDriver() || GetController()) return false;
    const ATsabsPassengerRailcar* Vehicle = Cast<ATsabsPassengerRailcar>(mOuterVehicle);
    if (!IsValid(Vehicle) || Vehicle->GetWorld() != GetWorld() ||
        GetAttachParentActor() != Vehicle || Vehicle->IsBoardingTransactionActive()) return false;
    // This method never forcibly ejects, destroys or teleports an occupant.
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    mOuterVehicle = nullptr;
    SetOwner(nullptr);
    ForceNetUpdate();
    return true;
}

FBox ATsabsPassengerAnchor::GetComponentsBoundingBox(bool bNonColliding, bool bIncludeFromChildActors) const
{
    const ATsabsPassengerRailcar* Vehicle = Cast<ATsabsPassengerRailcar>(mOuterVehicle);
    if (IsValid(Vehicle) && Vehicle->GetWorld() == GetWorld() && GetAttachParentActor() == Vehicle)
    {
        // GetSafeExitLocation calls GetActorBounds(true) on THIS pawn. An invisible
        // seat has no primitives, so inherited bounds otherwise resolve to an empty
        // world-origin box. Reuse the live vehicle bounds without adding collision.
        const FBox ChassisBounds = Vehicle->GetComponentsBoundingBox(bNonColliding, bIncludeFromChildActors);
        if (ChassisBounds.IsValid) return ChassisBounds;
    }
    return Super::GetComponentsBoundingBox(bNonColliding, bIncludeFromChildActors);
}

ATsabsPassengerRailcar* ATsabsPassengerAnchor::GetPassengerRailcar() const
{
    ATsabsPassengerRailcar* Vehicle = Cast<ATsabsPassengerRailcar>(mOuterVehicle);
    if (IsTemplate() || IsActorBeingDestroyed() || !GetWorld() || !IsValid(Vehicle) ||
        Vehicle->IsTemplate() || Vehicle->IsActorBeingDestroyed() || Vehicle->GetWorld() != GetWorld() ||
        GetAttachParentActor() != Vehicle) return nullptr;
    return Vehicle; // Structural identity only, not a boarding grant or speed rule.
}

bool ATsabsPassengerAnchor::CanDriverEnter(AFGCharacterPlayer* Character)
{
    if (IsTemplate() || IsActorBeingDestroyed() || !IsValid(Character) || Character->IsTemplate() ||
        Character->IsActorBeingDestroyed() || Character->GetWorld() != GetWorld() ||
        (!HasAuthority() && !Character->IsLocallyControlled()) || (HasAuthority() && !Character->HasAuthority()) ||
        GetDriver() || HasPendingDriver() || GetController() || Character->IsDrivingVehicle()) return false;
    const ATsabsPassengerRailcar* Vehicle = GetPassengerRailcar();
    if (!IsValid(Vehicle) || !Vehicle->IsPassengerAnchor(this)) return false;
    const AFGPlayerController* PC = Cast<AFGPlayerController>(Character->GetController());
    // The native saved-entry API also handles a character with no controller.
    // If one exists, retain the exact world/pawn/authority identity checks.
    if (Character->GetController() && (!IsValid(PC) || PC->GetWorld() != GetWorld() ||
        PC->GetPawn() != Character || (HasAuthority() && !PC->HasAuthority()))) return false;
    return Super::CanDriverEnter(Character);
}

bool ATsabsPassengerAnchor::TryGetPassengerExitOffset(AFGCharacterPlayer* Character, FVector& OutOffset)
{
    ATsabsPassengerRailcar* Vehicle = Cast<ATsabsPassengerRailcar>(mOuterVehicle);
    if (IsTemplate() || !IsValid(Character) || Character->IsTemplate() || Character->GetWorld() != GetWorld() ||
        !IsValid(Vehicle) || !Vehicle->IsPassengerAnchor(this) || !GetPassengerRailcar()) return false;
    const USkeletalMeshComponent* Mesh = Vehicle->GetPassengerMesh();
    const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
    if (!IsValid(Mesh) || !IsValid(Capsule) || GetActorTransform().ContainsNaN()) return false;
    const FVector Scale = GetActorScale3D().GetAbs();
    const double MinScale = FMath::Min3(Scale.X, Scale.Y, Scale.Z);
    if (!FMath::IsFinite(MinScale) || MinScale <= 0.0) return false;
    const FTransform MeshInAnchor = Mesh->GetComponentTransform().GetRelativeTransform(GetActorTransform());
    const FBox LocalChassisBounds = Mesh->CalcBounds(MeshInAnchor).GetBox();
    const double Radius = Capsule->GetScaledCapsuleRadius() / MinScale;
    const double HalfHeight = Capsule->GetScaledCapsuleHalfHeight() / MinScale;
    FVector Candidate = FVector::ZeroVector;
    auto LogExitOutcome = [&](uint8 Flag, const TCHAR* Stage)
    {
        if ((ExitDiagnosticStates & Flag) != 0) return;
        ExitDiagnosticStates |= Flag;
        FVector BoundsOrigin, BoundsExtent;
        GetActorBounds(true, BoundsOrigin, BoundsExtent, false);
        const FVector YawDestination = GetActorLocation() + FRotator(0.0, GetActorRotation().Yaw, 0.0).RotateVector(Candidate);
        UE_LOG(LogTsabsPassengerWagon, Display,
            TEXT("TSABS_EXIT_DIAG stage=%s anchor=%s bounds_origin=%s bounds_extent=%s local_min=%s local_max=%s offset=%s yaw_destination=%s capsule_radius=%.3f capsule_half_height=%.3f"),
            Stage, *GetActorLocation().ToString(), *BoundsOrigin.ToString(), *BoundsExtent.ToString(),
            *LocalChassisBounds.Min.ToString(), *LocalChassisBounds.Max.ToString(), *Candidate.ToString(),
            *YawDestination.ToString(), Radius, HalfHeight);
    };
    if (!TsabsPassenger::TryMakeExitOffset(LocalChassisBounds, Radius, HalfHeight, Candidate))
    {
        LogExitOutcome(1, TEXT("candidate_rejected"));
        return false;
    }
    FVector Chosen;
    // The geometric candidates are never placements: each must pass the native
    // collision query. DriverLeave repeats native placement with the chosen offset.
    const bool bFound = TsabsPassenger::TryFindSafeExitOffset(LocalChassisBounds, Radius, HalfHeight,
        [&](const FVector& Attempt)
        {
            Candidate = Attempt;
            FVector NativeLocation;
            FRotator NativeRotation;
            return GetSafeExitLocation(Character, Attempt, NativeLocation, NativeRotation) &&
                !NativeLocation.ContainsNaN() && !NativeRotation.ContainsNaN();
        }, Chosen);
    if (!bFound)
    {
        LogExitOutcome(2, TEXT("native_rejected"));
        return false;
    }
    Candidate = Chosen;
    LogExitOutcome(4, TEXT("native_accepted"));
    OutOffset = Chosen;
    return true;
}

bool ATsabsPassengerAnchor::DriverEnter(AFGCharacterPlayer* Character)
{
    ATsabsPassengerRailcar* Vehicle = GetPassengerRailcar();
    if (!HasAuthority() || !IsValid(Vehicle) || !Vehicle->HasAuthority() ||
        !Vehicle->IsPassengerAnchor(this) || !CanDriverEnter(Character)) return false;
    // New interactions keep the exact Character+Anchor reservation held by the car.
    // Outside that transaction this remains the ordinary non-RPC native server API,
    // also called by the game's own saved-character lifecycle. No special restore
    // receipt, load hook, private saved-field reader or replicated occupant is added.
    if (Vehicle->IsBoardingTransactionActive() && !Vehicle->IsBoardingReservationValid(Character, this)) return false;
    // Native load must not require a fresh Use selection or an exit query in an
    // unready world. New Use preflights in the car; manual DriverLeave still runs
    // the native safe-placement query immediately before leaving.
    return Super::DriverEnter(Character);
}

bool ATsabsPassengerAnchor::CanLeaveVehicle(AFGCharacterPlayer* Character)
{
    if (IsTemplate() || !IsValid(Character) || Character->IsTemplate() ||
        Character != GetDriver() || Character->GetWorld() != GetWorld() ||
        Character->GetDrivenVehicle() != this || !GetPassengerRailcar()) return false;
    const AFGPlayerController* PassengerController = Cast<AFGPlayerController>(GetController());
    return IsValid(PassengerController) && PassengerController->GetPawn() == this &&
        (HasAuthority() || IsLocallyControlled()) && Super::CanLeaveVehicle(Character);
}

bool ATsabsPassengerAnchor::DriverLeave(bool bKeepDriving)
{
    if (!HasAuthority() || bKeepDriving || !CanLeaveVehicle(GetDriver())) return false;
    FVector ExitOffset;
    if (!TryGetPassengerExitOffset(GetDriver(), ExitOffset)) return false;
    mDriverExitOffset = ExitOffset;
    // Keep the SDK's native occupant replication, possession restoration and exit placement.
    return Super::DriverLeave(false);
}

void ATsabsPassengerAnchor::Server_DriverLeave_Implementation()
{
    // The possessed pawn owns this native RPC; DriverLeave rechecks identity,
    // native exit conditions and safe placement. No custom rejection channel.
    if (!HasAuthority() || IsTemplate()) return;
    DriverLeave(false);
}

void ATsabsPassengerAnchor::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
    const float PlayerFOV = OutResult.FOV;
    OrbitCamera->GetCameraView(DeltaTime, OutResult);
    // Match the native driveable-pawn policy: do not replace the player's FOV.
    OutResult.FOV = PlayerFOV;
    if (!IsTemplate() && IsLocallyControlled() && CameraDiagnosticSamples < 3)
    {
        ++CameraDiagnosticSamples;
        const APlayerController* PC = Cast<APlayerController>(GetController());
        UE_LOG(LogTsabsPassengerWagon, Display,
            TEXT("TSABS_CAMERA_DIAG sample=%u locomotive_framing=%d arm_length=%.3f actual_distance=%.3f arm_registered=%d arm_active=%d arm_tick=%d camera_registered=%d view_target_anchor=%d fov=%.3f"),
            CameraDiagnosticSamples, bLocomotiveFramingUsed, OrbitArm->TargetArmLength,
            FVector::Distance(OrbitCamera->GetComponentLocation(), OrbitArm->PreviousDesiredLoc),
            OrbitArm->IsRegistered(), OrbitArm->IsActive(), OrbitArm->IsComponentTickEnabled(),
            OrbitCamera->IsRegistered(), PC && PC->GetViewTarget() == this, PlayerFOV);
    }
}

void ATsabsPassengerAnchor::ApplyOrbitInput(FVector2D Delta)
{
    if (!FMath::IsFinite(Delta.X) || !FMath::IsFinite(Delta.Y)) return;
    FRotator Rotation = OrbitArm->GetRelativeRotation();
    Rotation.Yaw = FRotator::NormalizeAxis(Rotation.Yaw + Delta.X);
    Rotation.Pitch = FMath::Clamp(Rotation.Pitch + Delta.Y, -80.0, 80.0);
    OrbitArm->SetRelativeRotation(Rotation);
}

void ATsabsPassengerAnchor::SetupPlayerInputComponent(UInputComponent* InInputComponent)
{
    // Base setup still dispatches to our allowlisted AddInputBindings override.
    Super::SetupPlayerInputComponent(InInputComponent);
    AFGPlayerController* PC = Cast<AFGPlayerController>(GetController());
    if (IsTemplate() || !GetWorld() || !GetWorld()->IsGameWorld() || !IsValid(PC) ||
        !PC->IsLocalController() || PC->GetPawn() != this) return;
    AFGHUD* HUD = Cast<AFGHUD>(PC->GetHUD());
    if (!IsValid(HUD)) return;
    UTsabsPassengerHUD* Existing = Cast<UTsabsPassengerHUD>(HUD->GetPawnHUD());
    if (!IsValid(Existing) || Existing->GetPassengerAnchor() != this)
        HUD->AddPawnHUD(UTsabsPassengerHUD::StaticClass(), this);
    UTsabsPassengerHUD* Created = Cast<UTsabsPassengerHUD>(HUD->GetPawnHUD());
    if (IsValid(Created) && Created->GetPassengerAnchor() == this)
    {
        PassengerHUD = Created;
        Created->BeginCrosshairSuppression(); // AddPawnHUD has now assigned mPawnHUD.
    }
}

void ATsabsPassengerAnchor::RemovePassengerHUD()
{
    if (PassengerHUD.IsValid())
    {
        PassengerHUD->EndCrosshairSuppression();
        PassengerHUD->RemoveFromParent();
    }
    PassengerHUD.Reset();
}

void ATsabsPassengerAnchor::UnPossessed()
{
    RemovePassengerHUD();
    Super::UnPossessed();
}

void ATsabsPassengerAnchor::EndPlay(const EEndPlayReason::Type Reason)
{
    RemovePassengerHUD();
    Super::EndPlay(Reason);
}

void ATsabsPassengerAnchor::AddInputBindings(UInputComponent* InInputComponent)
{
    if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InInputComponent))
    {
        if (LookAction) Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATsabsPassengerAnchor::InputLook);
        if (LeaveAction) Input->BindAction(LeaveAction, ETriggerEvent::Started, this, &ATsabsPassengerAnchor::InputLeave);
    }
}

void ATsabsPassengerAnchor::InputLook(const FInputActionValue& Value)
{
    if (IsLocallyControlled()) ApplyOrbitInput(Value.Get<FVector2D>());
}

void ATsabsPassengerAnchor::InputLeave(const FInputActionValue& Value)
{
    // RPC is on this possessed pawn, never on the unowned railcar.
    if (IsLocallyControlled()) Server_DriverLeave();
}
