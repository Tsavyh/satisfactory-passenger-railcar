#include "TsabsPassengerRailcar.h"
#include "TsabsPassengerRailMovement.h"
#include "TsabsPassengerWagonLog.h"
#include "FGFreightWagon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Hologram/FGRailroadVehicleHologram.h"
#include "UObject/ConstructorHelpers.h"
#include "TsabsPassengerAnchor.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "FGCharacterPlayer.h"
#include "FGPlayerController.h"
#include "FGHealthComponent.h"
#include "PassengerAccessPolicy.h"
#include "FGConstructDisqualifier.h"

ATsabsPassengerRailcar::ATsabsPassengerRailcar()
{
    PassengerMovement = CreateDefaultSubobject<UTsabsPassengerRailMovement>(TEXT("MovementComp"));
    PassengerMovement->SetUpdatedComponent(mMesh);
    mHologramClass = AFGRailroadVehicleHologram::StaticClass();
    bReplicates = true;
    bAlwaysRelevant = true;

    // The class path resolves to the REAL base-game asset in a running game.
    // Editor placeholder values are only useful for structural tests.
    static ConstructorHelpers::FClassFinder<AFGFreightWagon> FreightClass(
        TEXT("/Game/FactoryGame/Buildable/Vehicle/Train/Wagon/BP_FreightWagon"));
    FreightSetupReference = FreightClass.Class;
    if (FreightSetupReference)
    {
        const AFGFreightWagon* Source = GetDefault<AFGFreightWagon>(FreightSetupReference);
        const UFGRailroadVehicleMovementComponent* SourceMovement = Source->FindComponentByClass<UFGRailroadVehicleMovementComponent>();
        USkeletalMeshComponent* SourceMesh = Source->FindComponentByClass<USkeletalMeshComponent>();
        mLength = Source->GetLength();
        PassengerMovement->CopyEditableSetupFrom(SourceMovement);
        if (mMesh && SourceMesh)
        {
            mMesh->SetSkeletalMeshAsset(SourceMesh->GetSkeletalMeshAsset());
            mMesh->SetRelativeTransform(SourceMesh->GetRelativeTransform());
            mMesh->SetCollisionProfileName(SourceMesh->GetCollisionProfileName());
            mMesh->SetAnimInstanceClass(SourceMesh->GetAnimClass());
        }
    }
    else
    {
        UE_LOG(LogTsabsPassengerWagon, Error, TEXT("Base freight class reference could not be resolved."));
    }
}

UPawnMovementComponent* ATsabsPassengerRailcar::GetMovementComponent() const
{
    return PassengerMovement;
}

UFGRailroadVehicleMovementComponent* ATsabsPassengerRailcar::GetRailroadVehicleMovementComponent() const
{
    return PassengerMovement;
}

USkeletalMeshComponent* ATsabsPassengerRailcar::GetPassengerMesh() const
{
    return mMesh;
}

bool ATsabsPassengerRailcar::CanDriverEnter(AFGCharacterPlayer* Character)
{
    return false;
}

bool ATsabsPassengerRailcar::DriverEnter(AFGCharacterPlayer* Character)
{
    return false;
}

void ATsabsPassengerRailcar::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATsabsPassengerRailcar, PilotAnchor);
}

void ATsabsPassengerRailcar::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority()) EnsurePilotAnchor();
}

bool ATsabsPassengerRailcar::EnsurePilotAnchor()
{
    if (!HasAuthority() || IsTemplate() || !GetWorld() || IsActorBeingDestroyed() || BoardingReservation.IsReserved()) return false;
    if (PilotAnchor)
    {
        if (!IsValid(PilotAnchor) || PilotAnchor->GetWorld() != GetWorld()) return false;
        if (PilotAnchor->GetOuterVehicle() == this && PilotAnchor->GetAttachParentActor() == this) return true;
        return PilotAnchor->InitializePassengerAnchor(this, FTransform::Identity);
    }
    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ATsabsPassengerAnchor* NewAnchor = GetWorld()->SpawnActor<ATsabsPassengerAnchor>(ATsabsPassengerAnchor::StaticClass(), GetActorTransform(), Params);
    if (!IsValid(NewAnchor)) return false;
    if (!NewAnchor->InitializePassengerAnchor(this, FTransform::Identity))
    {
        // Newly spawned and empty only. Never destroy an existing occupied anchor.
        NewAnchor->Destroy();
        return false;
    }
    PilotAnchor = NewAnchor;
    ForceNetUpdate();
    return true;
}

bool ATsabsPassengerRailcar::IsBoardingCharacterValid(AFGCharacterPlayer* Character) const
{
    if (IsTemplate() || IsActorBeingDestroyed() || !IsValid(Character) || Character->IsTemplate() ||
        Character->IsActorBeingDestroyed() || !GetWorld() || Character->GetWorld() != GetWorld() ||
        (!HasAuthority() && !Character->IsLocallyControlled()) || Character->IsDrivingVehicle()) return false;
    const AFGPlayerController* PC = Cast<AFGPlayerController>(Character->GetController());
    const UFGHealthComponent* Health = Character->GetHealthComponent();
    return IsValid(PC) && PC->GetWorld() == GetWorld() && PC->GetPawn() == Character &&
        (!HasAuthority() || (Character->HasAuthority() && PC->HasAuthority())) &&
        IsValid(Health) && !Health->IsDead();
}

bool ATsabsPassengerRailcar::CanReachBoardingBody(AFGCharacterPlayer* Character) const
{
    // The standard player Tick selects this actor BEFORE UpdateUseState. The
    // server computes its own non-replicated selection, and Server_OnUse carries
    // no actor/hit supplied by a client. Reuse that policy (range, camera, Interact
    // channel, overlaps and priorities), rather than adding a second geometry gate.
    return IsBoardingCharacterValid(Character) && Character->GetBestUsableActor() == this;
}

ETsabsBoardingFeedback ATsabsPassengerRailcar::GetBoardingFeedback(AFGCharacterPlayer* Character) const
{
    if (!IsBoardingCharacterValid(Character)) return ETsabsBoardingFeedback::InvalidCharacter;
    if (!IsValid(PassengerMovement) || !TsabsPassenger::IsPassengerAccessSpeedAllowed(PassengerMovement->GetForwardSpeed()))
        return ETsabsBoardingFeedback::Moving;
    if (!IsValid(PilotAnchor) || PilotAnchor->IsActorBeingDestroyed() || PilotAnchor->GetOuterVehicle() != this ||
        PilotAnchor->GetWorld() != GetWorld() || PilotAnchor->GetAttachParentActor() != this)
        return ETsabsBoardingFeedback::Unavailable;
    if (BoardingReservation.IsReserved() || PilotAnchor->GetDriver() || PilotAnchor->HasPendingDriver() || PilotAnchor->GetController())
        return ETsabsBoardingFeedback::Busy;
    if (!CanReachBoardingBody(Character)) return ETsabsBoardingFeedback::OutOfReach;
    if (!PilotAnchor->CanDriverEnter(Character)) return ETsabsBoardingFeedback::Unavailable;
    FVector ExitOffset;
    if (!PilotAnchor->TryGetPassengerExitOffset(Character, ExitOffset)) return ETsabsBoardingFeedback::ExitBlocked;
    return ETsabsBoardingFeedback::Ready;
}

bool ATsabsPassengerRailcar::IsBoardingReservationValid(AFGCharacterPlayer* Character) const
{
    return HasAuthority() && BoardingReservation.IsHeldFor(Character) &&
        IsValid(PilotAnchor) && PilotAnchor->HasAuthority() && PilotAnchor->GetOuterVehicle() == this &&
        PilotAnchor->GetAttachParentActor() == this && PilotAnchor->IsWithinPassengerAccessSpeedLimit() &&
        CanReachBoardingBody(Character);
}

bool ATsabsPassengerRailcar::TryBoardPilot(AFGCharacterPlayer* Character)
{
    if (!HasAuthority() || GetBoardingFeedback(Character) != ETsabsBoardingFeedback::Ready) return false;
    // Server game-thread transaction, held across native entry/re-entrant callbacks.
    FScopedPassengerBoardingReservation Reservation(BoardingReservation, Character);
    if (!Reservation.IsAcquired() || !IsBoardingReservationValid(Character)) return false;
    return PilotAnchor->DriverEnter(Character); // Never DriverEnter on the railcar.
}

void ATsabsPassengerRailcar::OnUse_Implementation(AFGCharacterPlayer* Character, const FUseState& State)
{
    if (IsDerailed())
    {
        Super::OnUse_Implementation(Character, State);
        return;
    }
    // Called on client and server by the standard player-owned use route. No railcar RPC.
    // Deliberately ignore every client-supplied state/hit/availability field.
    if (HasAuthority()) TryBoardPilot(Character);
}

bool ATsabsPassengerRailcar::IsUseable_Implementation() const
{
    return !IsTemplate() && !IsActorBeingDestroyed(); // Keep blocked-reason feedback visible.
}

void ATsabsPassengerRailcar::UpdateUseState_Implementation(AFGCharacterPlayer* Character, const FVector& AtLocation,
    UPrimitiveComponent* ComponentHit, FUseState& OutState)
{
    if (IsDerailed())
    {
        Super::UpdateUseState_Implementation(Character, AtLocation, ComponentHit, OutState);
        return;
    }
    OutState.SetUseState(GetBoardingFeedback(Character) == ETsabsBoardingFeedback::Ready ?
        UFGUseState_Valid::StaticClass() : UTsabsPassengerUseState_Blocked::StaticClass());
}

FText ATsabsPassengerRailcar::FormatBoardingPrompt(const FText& UseKey)
{
    return FText::Format(NSLOCTEXT("TsabsPassenger", "BoardWithKey",
        "Pulsa <Key>{0}</> para montar como pasajero"), UseKey);
}

FText ATsabsPassengerRailcar::GetLookAtDecription_Implementation(AFGCharacterPlayer* Character, const FUseState& State) const
{
    if (IsDerailed())
    {
        return Super::GetLookAtDecription_Implementation(Character, State);
    }
    switch (GetBoardingFeedback(Character))
    {
    case ETsabsBoardingFeedback::Ready:
    {
        AFGPlayerController* PC = IsValid(Character) ? Cast<AFGPlayerController>(Character->GetController()) : nullptr;
        return FormatBoardingPrompt(PC ? PC->GetKeyNameForUseAction() : FText::GetEmpty());
    }
    case ETsabsBoardingFeedback::Moving:
        return FText::Format(NSLOCTEXT("TsabsPassenger", "BoardSpeedLimit", "El tren debe ir a menos de {0} km/h para subir"),
            FText::AsNumber(TsabsPassenger::PassengerAccessSpeedLimitKmh));
    case ETsabsBoardingFeedback::Busy:
        return NSLOCTEXT("TsabsPassenger", "Busy", "Plaza ocupada o entrada en curso");
    case ETsabsBoardingFeedback::OutOfReach:
        return NSLOCTEXT("TsabsPassenger", "Reach", "Acércate al vagón y míralo sin obstáculos");
    case ETsabsBoardingFeedback::ExitBlocked:
        return NSLOCTEXT("TsabsPassenger", "ExitBlocked", "No se ha encontrado una salida libre junto al vagón");
    default:
        return NSLOCTEXT("TsabsPassenger", "Unavailable", "Plaza de pasajero no disponible");
    }
}

bool ATsabsPassengerRailcar::HasProtectedPilot() const
{
    // A broken/missing reference is not permission to dismantle an unknown occupant.
    return BoardingReservation.IsReserved() || !IsValid(PilotAnchor) || PilotAnchor->GetWorld() != GetWorld() ||
        PilotAnchor->GetOuterVehicle() != this || PilotAnchor->GetAttachParentActor() != this ||
        PilotAnchor->GetDriver() || PilotAnchor->HasPendingDriver() || PilotAnchor->GetController();
}

bool ATsabsPassengerRailcar::CanDismantle_Implementation() const
{
    return !IsTemplate() && !HasProtectedPilot() && Super::CanDismantle_Implementation();
}

void ATsabsPassengerRailcar::GetDismantleDisqualifiers_Implementation(
    TArray<TSubclassOf<UFGConstructDisqualifier>>& OutReasons, const TArray<AActor*>& SelectedActors) const
{
    Super::GetDismantleDisqualifiers_Implementation(OutReasons, SelectedActors);
    if (HasProtectedPilot()) OutReasons.AddUnique(UFGDDVehicleHasDriver::StaticClass());
}

void ATsabsPassengerRailcar::Dismantle_Implementation()
{
    // Revalidate on authority; do not trust an earlier UI query/disqualifier list.
    if (!HasAuthority() || !CanDismantle_Implementation()) return;
    Super::Dismantle_Implementation();
}

bool ATsabsPassengerRailcar::ReleaseEmptyPilot()
{
    if (!HasAuthority() || IsTemplate() || HasProtectedPilot()) return false;
    if (!PilotAnchor->ResetPassengerAnchor()) return false;
    ATsabsPassengerAnchor* Empty = PilotAnchor;
    PilotAnchor = nullptr;
    Empty->Destroy();
    ForceNetUpdate();
    return true;
}

void ATsabsPassengerRailcar::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (HasAuthority() && IsValid(PilotAnchor) && !ReleaseEmptyPilot())
    {
        // No forced ejection or occupied-anchor destruction policy is invented here.
        UE_LOG(LogTsabsPassengerWagon, Warning,
            TEXT("Passenger pilot retained during EndPlay: occupied, pending, reserved or inconsistent; no forced exit."));
    }
    Super::EndPlay(EndPlayReason);
}

void ATsabsPassengerRailcar::PostLoadGame_Implementation(int32 SaveVersion, int32 GameVersion)
{
    Super::PostLoadGame_Implementation(SaveVersion, GameVersion);
    // PilotAnchor and native mOuterVehicle are SaveGame refs. Owner dependencies
    // are handled by the save system; do not add a railcar<->anchor dependency cycle.
    if (HasAuthority()) EnsurePilotAnchor();
}
