#pragma once

#include "CoreMinimal.h"
#include "ReferenceSkeleton.h"

// Keep this conservative: only equivalent reference poses may share a leader.
inline bool PassengerChassisReferenceMatches(const FReferenceSkeleton& Native, const FReferenceSkeleton& Visual)
{
    if (Native.GetNum() != Visual.GetNum() || Native.GetNum() == 0) return false;
    for (int32 NativeIndex = 0; NativeIndex < Native.GetNum(); ++NativeIndex)
    {
        const int32 VisualIndex = Visual.FindBoneIndex(Native.GetBoneName(NativeIndex));
        if (VisualIndex == INDEX_NONE) return false;
        const int32 NativeParent = Native.GetParentIndex(NativeIndex);
        const int32 VisualParent = Visual.GetParentIndex(VisualIndex);
        if ((NativeParent == INDEX_NONE) != (VisualParent == INDEX_NONE)) return false;
        if (NativeParent != INDEX_NONE && Native.GetBoneName(NativeParent) != Visual.GetBoneName(VisualParent)) return false;
        if (!Native.GetRefBonePose()[NativeIndex].Equals(Visual.GetRefBonePose()[VisualIndex], 0.01f)) return false;
    }
    return true;
}
