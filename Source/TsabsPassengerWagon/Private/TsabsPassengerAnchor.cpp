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
#include "FGRailroadVehicleMovementComponent.h"
#include "PassengerAccessPolicy.h"
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
        (GetOuterVehicle() && GetOuterVehicle() != Vehicle)) return false;
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
    if (IsValid(Vehicle) && Vehicle->IsBoardingTransactionActive()) return false;
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

bool ATsabsPassengerAnchor::IsWithinPassengerAccessSpeedLimit()
{
    ATsabsPassengerRailcar* Vehicle = Cast<ATsabsPassengerRailcar>(GetOuterVehicle());
    if (!IsValid(Vehicle) || Vehicle->IsTemplate() || Vehicle->GetWorld() != GetWorld() ||
        GetAttachParentActor() != Vehicle) return false;
    const UFGRailroadVehicleMovementComponent* Movement = Vehicle->GetRailroadVehicleMovementComponent();
    return IsValid(Movement) && TsabsPassenger::IsPassengerAccessSpeedAllowed(Movement->GetForwardSpeed());
}

bool ATsabsPassengerAnchor::CanDriverEnter(AFGCharacterPlayer* Character)
{
    if (IsTemplate() || !IsValid(Character) || Character->IsTemplate() ||
        Character->GetWorld() != GetWorld() || (!HasAuthority() && !Character->IsLocallyControlled()) ||
        GetDriver() || HasPendingDriver() || GetController() || Character->IsDrivingVehicle() ||
        !IsWithinPassengerAccessSpeedLimit()) return false;
    const AFGPlayerController* PassengerController = Cast<AFGPlayerController>(Character->GetController());
    return IsValid(PassengerController) && PassengerController->GetPawn() == Character &&
        Super::CanDriverEnter(Character);
}

bool ATsabsPassengerAnchor::TryGetPassengerExitOffset(AFGCharacterPlayer* Character, FVector& OutOffset)
{
    ATsabsPassengerRailcar* Vehicle = Cast<ATsabsPassengerRailcar>(mOuterVehicle);
    if (IsTemplate() || !IsValid(Character) || Character->IsTemplate() || Character->GetWorld() != GetWorld() ||
        !IsValid(Vehicle) || Vehicle->GetPilotAnchor() != this || !IsWithinPassengerAccessSpeedLimit()) return false;
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
    FVector NativeLocation;
    FRotator NativeRotation;
    // Native method is not virtual. Preserve its collision validation and
    // native placement; never teleport to the geometric candidate ourselves.
    if (!GetSafeExitLocation(Character, Candidate, NativeLocation, NativeRotation) ||
        NativeLocation.ContainsNaN() || NativeRotation.ContainsNaN())
    {
        LogExitOutcome(2, TEXT("native_rejected"));
        return false;
    }
    LogExitOutcome(4, TEXT("native_accepted"));
    OutOffset = Candidate;
    return true;
}

bool ATsabsPassengerAnchor::DriverEnter(AFGCharacterPlayer* Character)
{
    ATsabsPassengerRailcar* Vehicle = Cast<ATsabsPassengerRailcar>(GetOuterVehicle());
    // Only a current server use transaction for this exact character may mutate entry.
    if (!HasAuthority() || !IsValid(Vehicle) || Vehicle->GetPilotAnchor() != this || !Vehicle->IsBoardingReservationValid(Character) ||
        !CanDriverEnter(Character)) return false;
    FVector ExitOffset;
    if (!TryGetPassengerExitOffset(Character, ExitOffset)) return false;
    mDriverExitOffset = ExitOffset;
    return Super::DriverEnter(Character);
}

bool ATsabsPassengerAnchor::CanLeaveVehicle(AFGCharacterPlayer* Character)
{
    if (IsTemplate() || !IsValid(Character) || Character->IsTemplate() ||
        Character != GetDriver() || Character->GetWorld() != GetWorld() ||
        Character->GetDrivenVehicle() != this || !IsWithinPassengerAccessSpeedLimit()) return false;
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
    // Never trust an earlier client-side speed/occupancy check. Recheck now on server.
    if (!HasAuthority() || IsTemplate()) return;
    if (!IsWithinPassengerAccessSpeedLimit())
    {
        const APlayerController* PC = Cast<APlayerController>(GetController());
        if (IsValid(PC) && PC->GetPawn() == this) Client_NotifyExitSpeedRejected();
        return;
    }
    DriverLeave(false);
}

void ATsabsPassengerAnchor::Client_NotifyExitSpeedRejected_Implementation()
{
    const APlayerController* PC = Cast<APlayerController>(GetController());
    if (!IsValid(PC) || !PC->IsLocalController() || PC->GetPawn() != this) return;
    if (PassengerHUD.IsValid() && PassengerHUD->GetPassengerAnchor() == this)
        PassengerHUD->NotifyExitSpeedRejected();
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
