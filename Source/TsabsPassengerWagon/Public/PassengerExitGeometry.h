#pragma once
#include "CoreMinimal.h"
namespace TsabsPassenger
{
/** Provisional LOCAL lateral candidate only. Never a substitute for native safe-exit validation. */
inline bool TryMakeExitOffset(const FBox& BodyBounds, double CapsuleRadius, double CapsuleHalfHeight, FVector& OutOffset)
{
    if (!BodyBounds.IsValid || BodyBounds.Min.ContainsNaN() || BodyBounds.Max.ContainsNaN() ||
        !FMath::IsFinite(CapsuleRadius) || !FMath::IsFinite(CapsuleHalfHeight) ||
        CapsuleRadius <= 0.0 || CapsuleHalfHeight < CapsuleRadius) return false;
    const FVector Size = BodyBounds.GetSize();
    if (Size.ContainsNaN() || Size.X <= 0.0 || Size.Y <= 0.0 || Size.Z <= 0.0) return false;
    // GetSafeExitLocation's downward fallback sweeps TO this target and
    // accepts a non-initial contact. This is a ground-probe endpoint, not
    // the final capsule centre: adding half-height can leave the entire
    // sweep above a clear floor when the chassis is elevated over its pivot.
    const FVector Candidate(BodyBounds.GetCenter().X, BodyBounds.Max.Y + 2.0 * CapsuleRadius,
        FMath::Min(0.0, BodyBounds.Min.Z));
    if (Candidate.ContainsNaN()) return false;
    OutOffset = Candidate;
    return true;
}

// Each candidate must be accepted by the caller's native placement query.
template <typename FValidate>
inline bool TryFindSafeExitOffset(const FBox& LocalBounds, double CapsuleRadius,
                                  double CapsuleHalfHeight, FValidate&& Validate,
                                  FVector& OutOffset)
{
    FVector Candidate;
    if (!TryMakeExitOffset(LocalBounds, CapsuleRadius, CapsuleHalfHeight, Candidate)) return false;
    // Keep the doorway-centred probe first; nearby longitudinal probes can
    // avoid a local obstruction. Each still goes through native validation.
    const double Step = FMath::Min(100.0, LocalBounds.GetExtent().X / 3.0);
    if (!FMath::IsFinite(Step) || Step <= 0.0) return false;
    const double Offsets[] = {0.0, Step, -Step, 2.0 * Step, -2.0 * Step};
    const double CenterX = Candidate.X;
    for (double Offset : Offsets)
    {
        Candidate.X = CenterX + Offset;
        if (Validate(Candidate))
        {
            OutOffset = Candidate;
            return true;
        }
    }
    return false;
}
}
