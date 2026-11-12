#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cxx=${CXX:-c++}
build_dir=$(mktemp -d "${TMPDIR:-/tmp}/passenger-policy.XXXXXXXX")
trap 'rm -rf -- "$build_dir"' EXIT
for mode in precise fast; do
    flags=(-fno-fast-math)
    if [[ $mode == fast ]]; then flags=(-ffast-math); fi
    printf 'Passenger reservation: %s (%s)\n' "$mode" "$cxx"
    "$cxx" -std=c++20 -O2 -Wall -Wextra -Werror "${flags[@]}" \
        -I"$root/Tests" -I"$root/Source/TsabsPassengerWagon/Public" \
        "$root/Tests/StandalonePassengerPolicyTests.cpp" -o "$build_dir/passenger-policy-$mode"
    "$build_dir/passenger-policy-$mode"
done
