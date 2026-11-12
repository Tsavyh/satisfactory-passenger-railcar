#pragma once
// Standalone test boundary only: never used by the Unreal module.
struct UObject { bool valid = true; };
inline bool IsValid(const UObject* p) { return p && p->valid; }
