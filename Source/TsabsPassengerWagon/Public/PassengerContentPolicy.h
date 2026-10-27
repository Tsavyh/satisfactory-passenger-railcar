#pragma once
#include "Module/ModModule.h"

#define TSABS_AVAILABILITY_POLICY_VERSION 1

namespace TsabsPassenger
{
enum class EPassengerAvailabilityAction { None, Grant, Revoke };

// bValidWorld includes the manager/world identity check at the routing boundary.
// Failed grant prerequisites deliberately remove only our old experimental grant.
inline EPassengerAvailabilityAction DecidePassengerAvailability(ELifecyclePhase Phase,
    bool bValidWorld, bool bValidOwner, bool bClient, bool bManagerAuthority,
    bool bRegistered, bool bReferenceResolved, bool bReconciling,
    bool bNativeAvailable, bool bOwnAvailable)
{
    if (Phase != ELifecyclePhase::POST_INITIALIZATION || !bValidWorld || !bValidOwner ||
        bClient || !bManagerAuthority || bReconciling)
        return EPassengerAvailabilityAction::None;
    const bool bDesired = bRegistered && bReferenceResolved && bNativeAvailable;
    if (bDesired == bOwnAvailable) return EPassengerAvailabilityAction::None;
    return bDesired ? EPassengerAvailabilityAction::Grant : EPassengerAvailabilityAction::Revoke;
}

// Shared production decisions, not a simulated RecipeManager.
inline bool ShouldRegisterPassengerRecipe(ELifecyclePhase Phase, bool bValidWorld, bool bValidOwner, bool bAlreadyAttempted)
{
    return Phase == ELifecyclePhase::INITIALIZATION && bValidWorld && bValidOwner && !bAlreadyAttempted;
}
}
