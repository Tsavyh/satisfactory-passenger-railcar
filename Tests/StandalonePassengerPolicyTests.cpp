// Exercises the actual production reservation header with a minimal UObject boundary.
// Does not model network replication, native possession or Unreal GC.
#include "PassengerBoardingReservation.h"
#include <iostream>
#include <type_traits>

int main() {
    int checks = 0, failures = 0;
    auto check = [&](bool ok, const char* label) { ++checks; if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; } };
    UObject a, b, invalid; invalid.valid = false;
    FPassengerBoardingReservation r;
    check(!r.IsReserved(), "initially free");
    check(!r.IsHeldFor(&a), "not held initially");
    check(!r.TryAcquire(nullptr), "reject null");
    check(!r.TryAcquire(&invalid), "reject invalid");
    check(!r.IsReserved(), "invalid does not reserve");
    check(r.TryAcquire(&a), "acquire first");
    check(r.IsReserved(), "reserved");
    check(r.IsHeldFor(&a), "held for first");
    check(!r.IsHeldFor(&b), "not held for second");
    check(!r.TryAcquire(&a), "same cannot reacquire");
    check(!r.TryAcquire(&b), "other cannot acquire");
    check(!r.Release(&b), "other cannot release");
    check(r.IsHeldFor(&a), "identity survives failed release");
    a.valid = false;
    check(!r.IsHeldFor(&a), "destroyed identity is not authorized");
    check(r.Release(&a), "destroyed identity can release reservation");
    check(!r.IsReserved(), "free after release");
    a.valid = true;
    check(r.TryAcquire(&b), "second can acquire");
    check(r.Release(&b), "second releases");
    check(!r.Release(&b), "double release fails");
    {
        FScopedPassengerBoardingReservation s(r, &a);
        check(s.IsAcquired(), "scope acquires");
        check(r.IsHeldFor(&a), "scope holds identity");
        FScopedPassengerBoardingReservation nested(r, &b);
        check(!nested.IsAcquired(), "nested scope rejected");
    }
    check(!r.IsReserved(), "scope releases");
    {
        FScopedPassengerBoardingReservation s(r, nullptr);
        check(!s.IsAcquired(), "invalid scope rejected");
    }
    check(!r.IsReserved(), "invalid scope cannot reserve");
    check(!std::is_copy_constructible_v<FScopedPassengerBoardingReservation>, "scope is not copyable");
    if (failures) return 1;
    std::cout << "PASSENGER_RESERVATION_POLICY_OK: " << checks << " checks passed\n";
    return 0;
}
