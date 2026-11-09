#include "TsabsPassengerRailcar.h"
#include "TsabsPassengerRailMovement.h"
#include "TsabsPassengerWagonLog.h"
#include "PassengerChassisCompatibility.h"
#include "PassengerChassisSlotMapping.h"
#include "FGFreightWagon.h"
#include "FGBlueprintFunctionLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Hologram/FGRailroadVehicleHologram.h"
#include "UObject/ConstructorHelpers.h"
#include "TsabsPassengerAnchor.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "FGCharacterPlayer.h"
#include "FGPlayerController.h"
#include "FGHealthComponent.h"
#include "FGConstructDisqualifier.h"

ATsabsPassengerRailcar::ATsabsPassengerRailcar()
{
    PassengerMovement = CreateDefaultSubobject<UTsabsPassengerRailMovement>(TEXT("MovementComp"));
    PassengerMovement->SetUpdatedComponent(mMesh);

    // PRIVATE visual experiment: the derived game geometry is not cleared for distribution.
    PassengerBodyVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PassengerBodyVisual"));
    PassengerBodyVisual->SetupAttachment(mMesh, FName(TEXT("body")));
    PassengerBodyVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PassengerBodyVisual->SetCanEverAffectNavigation(false);
    // Query-only body proxy: blocks player sweeps and permits game targeting,
    // while never taking over the native skeletal train's physics simulation.
    PassengerBodyCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("PassengerBodyCollision"));
    PassengerBodyCollision->SetupAttachment(PassengerBodyVisual);
    PassengerBodyCollision->SetBoxExtent(FVector(635.0, 200.0, 220.0));
    PassengerBodyCollision->SetRelativeLocation(FVector(0.0, 0.0, 480.0));
    PassengerBodyCollision->SetCollisionObjectType(ECC_WorldDynamic);
    PassengerBodyCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PassengerBodyCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    PassengerBodyCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    PassengerBodyCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    PassengerBodyCollision->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Block); // BuildGun
    PassengerBodyCollision->SetCollisionResponseToChannel(ECC_GameTraceChannel13, ECR_Overlap); // Interact
    PassengerBodyCollision->SetCanEverAffectNavigation(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyAsset(
        TEXT("/TsabsPassengerWagon/VisualPrivate/Study30/PassengerBody30_PRIVATE.PassengerBody30_PRIVATE"));
    if (BodyAsset.Succeeded())
    {
        PassengerBodyVisual->SetStaticMesh(BodyAsset.Object);
    }
    else
    {
        UE_LOG(LogTsabsPassengerWagon, Error, TEXT("Private passenger visual body prototype not found."));
    }
    // Keep the original skeletal component for movement, physics and collision.
    PassengerChassisVisual = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PassengerChassisVisual"));
    PassengerChassisVisual->SetupAttachment(mMesh);
    PassengerChassisVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PassengerChassisVisual->SetCanEverAffectNavigation(false);
    PassengerChassisVisual->SetVisibility(false);
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> ChassisAsset(
        TEXT("/TsabsPassengerWagon/VisualPrivate/Study26/Skeletal/PassengerChassis26_PRIVATE.PassengerChassis26_PRIVATE"));
    if (ChassisAsset.Succeeded())
        PassengerChassisVisual->SetSkeletalMeshAsset(ChassisAsset.Object);
    else
        UE_LOG(LogTsabsPassengerWagon, Error, TEXT("Private passenger visual chassis prototype not found."));

    // Holograms can copy the template before BeginPlay applies native alignment.
    // Use the visual chassis reference here; BeginPlay still recalculates against
    // the actual native skeleton and preserves its existing compatibility checks.
    if (const USkeletalMesh* PreviewMesh = PassengerChassisVisual->GetSkeletalMeshAsset())
    {
        const FReferenceSkeleton& PreviewRef = PreviewMesh->GetRefSkeleton();
        const int32 PreviewBody = PreviewRef.FindBoneIndex(TEXT("body"));
        if (PreviewBody != INDEX_NONE)
        {
            FTransform BoneToComponent = FTransform::Identity;
            for (int32 Index = PreviewBody; Index != INDEX_NONE; Index = PreviewRef.GetParentIndex(Index))
                BoneToComponent = BoneToComponent * PreviewRef.GetRefBonePose()[Index];
            PassengerBodyVisual->SetRelativeTransform(BoneToComponent.Inverse());
        }
    }
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

void ATsabsPassengerRailcar::OnCustomizationDataApplied(const FFactoryCustomizationData& CustomizationData)
{
    Super::OnCustomizationDataApplied(CustomizationData);
    if (PassengerBodyVisual)
        UFGBlueprintFunctionLibrary::ApplyCustomizationPrimitiveDataToComponent(PassengerBodyVisual, CustomizationData);
    if (PassengerChassisVisual)
        UFGBlueprintFunctionLibrary::ApplyCustomizationPrimitiveDataToComponent(PassengerChassisVisual, CustomizationData);
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
    DOREPLIFETIME(ATsabsPassengerRailcar, SecondPassengerAnchor);
}

void ATsabsPassengerRailcar::BeginPlay()
{
    Super::BeginPlay();
    // The body mesh is authored in chassis-component coordinates, not bone coordinates.
    // Align against the reference pose so it follows the body bone without a fixed offset.
    if (PassengerBodyVisual)
    {
        PassengerBodyVisual->SetVisibility(false);
        const USkeletalMesh* Chassis = mMesh ? mMesh->GetSkeletalMeshAsset() : nullptr;
        if (Chassis && PassengerBodyVisual->GetStaticMesh())
        {
            const FReferenceSkeleton& Reference = Chassis->GetRefSkeleton();
            const int32 BodyBone = Reference.FindBoneIndex(TEXT("body"));
            if (BodyBone != INDEX_NONE)
            {
                FTransform BoneToComponent = FTransform::Identity;
                for (int32 Index = BodyBone; Index != INDEX_NONE; Index = Reference.GetParentIndex(Index))
                    BoneToComponent = BoneToComponent * Reference.GetRefBonePose()[Index];
                if (!BoneToComponent.ContainsNaN() && BoneToComponent.GetScale3D().GetAbsMin() > SMALL_NUMBER)
                {
                    PassengerBodyVisual->SetRelativeTransform(BoneToComponent.Inverse());
                    PassengerBodyVisual->SetVisibility(true);
                }
            }
        }
    }
    if (PassengerBodyCollision)
        PassengerBodyCollision->SetCollisionEnabled(PassengerBodyVisual && PassengerBodyVisual->IsVisible()
            ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
    // Keep native motion/physics as the leader; swap rendering only if the full
    // reference skeleton agrees. A mismatch leaves the native chassis visible.
    if (mMesh && PassengerChassisVisual && mMesh->GetSkeletalMeshAsset() && PassengerChassisVisual->GetSkeletalMeshAsset())
    {
        const FReferenceSkeleton& NativeRef = mMesh->GetSkeletalMeshAsset()->GetRefSkeleton();
        const FReferenceSkeleton& VisualRef = PassengerChassisVisual->GetSkeletalMeshAsset()->GetRefSkeleton();
        bool bCompatible = PassengerChassisReferenceMatches(NativeRef, VisualRef);
        TArray<UMaterialInterface*> NativeMaterials;
        if (bCompatible)
        {
            const TArray<FName> Slots = PassengerChassisVisual->GetMaterialSlotNames();
            bCompatible = !Slots.IsEmpty();
            for (const FName Slot : Slots)
            {
                const FName NativeSlot = PassengerChassisNativeSlot(FName(*mMesh->GetSkeletalMeshAsset()->GetPathName()), Slot);
                const int32 SourceIndex = mMesh->GetMaterialIndex(NativeSlot);
                if (SourceIndex == INDEX_NONE)
                {
                    bCompatible = false;
                    break;
                }
                // Null is a valid native material slot; preserve it exactly.
                NativeMaterials.Add(mMesh->GetMaterial(SourceIndex));
            }
        }
        if (bCompatible)
        {
            for (int32 Index = 0; Index < NativeMaterials.Num(); ++Index)
                PassengerChassisVisual->SetMaterial(Index, NativeMaterials[Index]);
            PassengerChassisVisual->SetLeaderPoseComponent(mMesh);
            if (PassengerChassisVisual->LeaderPoseComponent.Get() == mMesh)
            {
                mMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
                mMesh->SetVisibility(false, false);
                PassengerChassisVisual->SetVisibility(true, false);
            }
        }
    }
    // Diagnostic only: do not change compatibility, rendering or gameplay.
    // The shipped game's native skeleton/material slots are not SDK placeholders.
    if (!PassengerChassisVisual || !PassengerChassisVisual->IsVisible())
    {
        FString Reason(TEXT("unknown"));
        FString Detail;
        const USkeletalMesh* NativeAsset = mMesh ? mMesh->GetSkeletalMeshAsset() : nullptr;
        const USkeletalMesh* VisualAsset = PassengerChassisVisual ? PassengerChassisVisual->GetSkeletalMeshAsset() : nullptr;
        if (!NativeAsset) Reason = TEXT("native_mesh_missing");
        else if (!VisualAsset) Reason = TEXT("visual_mesh_missing");
        else
        {
            const FReferenceSkeleton& Native = NativeAsset->GetRefSkeleton();
            const FReferenceSkeleton& Visual = VisualAsset->GetRefSkeleton();
            if (Native.GetNum() != Visual.GetNum() || Native.GetNum() == 0)
            {
                Reason = TEXT("bone_count_mismatch");
                Detail = FString::Printf(TEXT("native_count=%d visual_count=%d"), Native.GetNum(), Visual.GetNum());
            }
            else
            {
                for (int32 Index = 0; Index < Native.GetNum(); ++Index)
                {
                    const int32 VisualIndex = Visual.FindBoneIndex(Native.GetBoneName(Index));
                    if (VisualIndex == INDEX_NONE)
                    {
                        Reason = TEXT("bone_name_mismatch");
                        Detail = FString::Printf(TEXT("native_index=%d missing_visual_bone=%s"), Index,
                            *Native.GetBoneName(Index).ToString());
                        break;
                    }
                    const int32 NativeParent = Native.GetParentIndex(Index);
                    const int32 VisualParent = Visual.GetParentIndex(VisualIndex);
                    if ((NativeParent == INDEX_NONE) != (VisualParent == INDEX_NONE)
                        || (NativeParent != INDEX_NONE && Native.GetBoneName(NativeParent) != Visual.GetBoneName(VisualParent)))
                    {
                        Reason = TEXT("bone_parent_mismatch");
                        Detail = FString::Printf(TEXT("native_index=%d visual_index=%d bone=%s"), Index, VisualIndex,
                            *Native.GetBoneName(Index).ToString());
                        break;
                    }
                    if (!Native.GetRefBonePose()[Index].Equals(Visual.GetRefBonePose()[VisualIndex], 0.01f))
                    {
                        Reason = TEXT("bone_pose_mismatch");
                        Detail = FString::Printf(TEXT("native_index=%d visual_index=%d bone=%s native=%s visual=%s"), Index, VisualIndex,
                            *Native.GetBoneName(Index).ToString(), *Native.GetRefBonePose()[Index].ToHumanReadableString(),
                            *Visual.GetRefBonePose()[VisualIndex].ToHumanReadableString());
                        break;
                    }
                }
                if (Reason == TEXT("unknown"))
                {
                    const TArray<FName> Slots = PassengerChassisVisual->GetMaterialSlotNames();
                    if (Slots.IsEmpty()) Reason = TEXT("visual_material_slots_empty");
                    for (const FName Slot : Slots)
                    {
                        const FName ResolvedSlot = PassengerChassisNativeSlot(FName(*NativeAsset->GetPathName()), Slot);
                        if (mMesh->GetMaterialIndex(ResolvedSlot) == INDEX_NONE)
                        {
                            Reason = TEXT("native_material_slot_missing");
                            TArray<FString> MaterialDetails;
                            for (const FName NativeSlot : mMesh->GetMaterialSlotNames())
                            {
                                const int32 NativeIndex = mMesh->GetMaterialIndex(NativeSlot);
                                const UMaterialInterface* Material = mMesh->GetMaterial(NativeIndex);
                                MaterialDetails.Add(FString::Printf(TEXT("%s:%s"), *NativeSlot.ToString(),
                                    Material ? *Material->GetPathName() : TEXT("null")));
                            }
                            Detail = FString::Printf(TEXT("slot=%s resolved_slot=%s native_asset=%s native_materials=[%s]"), *Slot.ToString(),
                                *ResolvedSlot.ToString(), *NativeAsset->GetPathName(), *FString::Join(MaterialDetails, TEXT(",")));
                            break;
                        }
                    }
                    if (Reason == TEXT("unknown")) Reason = TEXT("leader_pose_not_set");
                }
            }
        }
        UE_LOG(LogTsabsPassengerWagon, Display, TEXT("TSABS_CHASSIS_DETAIL reason=%s %s"), *Reason, *Detail);
    }
    UE_LOG(LogTsabsPassengerWagon, Display,
        TEXT("TSABS_VISUAL_DIAG body_visible=%d body_collision=%d clipped_chassis_visible=%d native_chassis_visible=%d leader_matches=%d"),
        PassengerBodyVisual && PassengerBodyVisual->IsVisible(),
        PassengerBodyCollision && PassengerBodyCollision->GetCollisionEnabled() != ECollisionEnabled::NoCollision,
        PassengerChassisVisual && PassengerChassisVisual->IsVisible(), mMesh && mMesh->IsVisible(),
        PassengerChassisVisual && PassengerChassisVisual->LeaderPoseComponent.Get() == mMesh);
    if (HasAuthority()) EnsurePassengerAnchors();
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

bool ATsabsPassengerRailcar::EnsurePassengerAnchors()
{
    if (!HasAuthority() || IsTemplate() || !GetWorld() || IsActorBeingDestroyed() ||
        BoardingReservation.IsReserved() || (SecondPassengerAnchor && SecondPassengerAnchor == PilotAnchor)) return false;
    if (!EnsurePilotAnchor()) return false;
    if (SecondPassengerAnchor)
    {
        if (!IsValid(SecondPassengerAnchor) || SecondPassengerAnchor->IsActorBeingDestroyed() ||
            SecondPassengerAnchor->GetWorld() != GetWorld()) return false;
        if (SecondPassengerAnchor->GetOuterVehicle() == this && SecondPassengerAnchor->GetAttachParentActor() == this) return true;
        return SecondPassengerAnchor->InitializePassengerAnchor(this, FTransform::Identity);
    }
    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ATsabsPassengerAnchor* NewAnchor = GetWorld()->SpawnActor<ATsabsPassengerAnchor>(
        ATsabsPassengerAnchor::StaticClass(), GetActorTransform(), Params);
    if (!IsValid(NewAnchor)) return false;
    if (!NewAnchor->InitializePassengerAnchor(this, FTransform::Identity))
    {
        NewAnchor->Destroy(); // Only this new empty actor, never an existing saved identity.
        return false;
    }
    SecondPassengerAnchor = NewAnchor;
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

bool ATsabsPassengerRailcar::IsPassengerAnchor(const ATsabsPassengerAnchor* Anchor) const
{
    return IsValid(Anchor) && PilotAnchor != SecondPassengerAnchor &&
        (Anchor == PilotAnchor || Anchor == SecondPassengerAnchor) && Anchor->GetPassengerRailcar() == this;
}

ATsabsPassengerAnchor* ATsabsPassengerRailcar::FindAvailablePassengerAnchor() const
{
    if (IsTemplate() || IsActorBeingDestroyed() || BoardingReservation.IsReserved() ||
        !IsPassengerAnchor(PilotAnchor) || (SecondPassengerAnchor && !IsPassengerAnchor(SecondPassengerAnchor))) return nullptr;
    for (ATsabsPassengerAnchor* Anchor : {PilotAnchor.Get(), SecondPassengerAnchor.Get()})
        if (Anchor && !Anchor->GetDriver() && !Anchor->HasPendingDriver() && !Anchor->GetController()) return Anchor;
    return nullptr;
}

ETsabsBoardingFeedback ATsabsPassengerRailcar::GetBoardingFeedback(AFGCharacterPlayer* Character) const
{
    if (!IsBoardingCharacterValid(Character)) return ETsabsBoardingFeedback::InvalidCharacter;
    if (!IsPassengerAnchor(PilotAnchor) || (SecondPassengerAnchor && !IsPassengerAnchor(SecondPassengerAnchor)))
        return ETsabsBoardingFeedback::Unavailable;
    if (BoardingReservation.IsReserved()) return ETsabsBoardingFeedback::Busy;
    ATsabsPassengerAnchor* Anchor = FindAvailablePassengerAnchor();
    if (!Anchor) return ETsabsBoardingFeedback::Busy;
    if (!CanReachBoardingBody(Character)) return ETsabsBoardingFeedback::OutOfReach;
    if (!Anchor->CanDriverEnter(Character)) return ETsabsBoardingFeedback::Unavailable;
    FVector ExitOffset;
    if (!Anchor->TryGetPassengerExitOffset(Character, ExitOffset)) return ETsabsBoardingFeedback::ExitBlocked;
    return ETsabsBoardingFeedback::Ready;
}

bool ATsabsPassengerRailcar::IsBoardingReservationValid(AFGCharacterPlayer* Character) const
{
    return IsBoardingReservationValid(Character, BoardingAnchor.Get());
}

bool ATsabsPassengerRailcar::IsBoardingReservationValid(AFGCharacterPlayer* Character, const ATsabsPassengerAnchor* Anchor) const
{
    return HasAuthority() && BoardingReservation.IsHeldFor(Character) && IsPassengerAnchor(Anchor) &&
        BoardingAnchor.Get() == Anchor && Anchor->HasAuthority() && CanReachBoardingBody(Character);
}

bool ATsabsPassengerRailcar::TryBoardPilot(AFGCharacterPlayer* Character)
{
    if (!HasAuthority() || GetBoardingFeedback(Character) != ETsabsBoardingFeedback::Ready) return false;
    ATsabsPassengerAnchor* Anchor = FindAvailablePassengerAnchor();
    if (!Anchor) return false;
    // One synchronous vehicle transaction; exact character AND selected stable place.
    FScopedPassengerBoardingReservation Reservation(BoardingReservation, Character);
    TGuardValue<TWeakObjectPtr<ATsabsPassengerAnchor>> Selected(BoardingAnchor, TWeakObjectPtr<ATsabsPassengerAnchor>(Anchor));
    if (!Reservation.IsAcquired() || !IsBoardingReservationValid(Character, Anchor)) return false;
    return Anchor->DriverEnter(Character); // Never give control of the train itself.
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
    // A missing original identity or a broken referenced place is never a vacancy.
    const auto IsProtected = [this](ATsabsPassengerAnchor* Anchor)
    {
        return !IsValid(Anchor) || Anchor->IsActorBeingDestroyed() || Anchor->GetWorld() != GetWorld() ||
            Anchor->GetOuterVehicle() != this || Anchor->GetAttachParentActor() != this ||
            Anchor->GetDriver() || Anchor->HasPendingDriver() || Anchor->GetController();
    };
    // A null second slot is legitimate before pair creation / on a legacy one-place car.
    return BoardingReservation.IsReserved() || IsProtected(PilotAnchor) ||
        (SecondPassengerAnchor && (SecondPassengerAnchor == PilotAnchor || IsProtected(SecondPassengerAnchor)));
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
    ATsabsPassengerAnchor* Empty = PilotAnchor;
    ATsabsPassengerAnchor* Second = SecondPassengerAnchor;
    // Refuse the entire cleanup if either place is occupied/pending/inconsistent.
    if (Second && !Second->ResetPassengerAnchor()) return false;
    if (!Empty->ResetPassengerAnchor()) return false;
    PilotAnchor = nullptr;
    SecondPassengerAnchor = nullptr;
    Empty->Destroy();
    if (Second) Second->Destroy();
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
    if (HasAuthority()) EnsurePassengerAnchors();
}
