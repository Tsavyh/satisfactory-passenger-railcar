# Build notes — 0.1.17

This is the exact plugin-input snapshot recorded for the CL 517554 0.1.17 cook; see `SOURCE-MANIFEST.json`. The SDK, engine and game are not included. Project source code is MIT; game-derived content is not covered by that license.

The recorded build used Satisfactory Mod Loader 3.12.0, the CL 517554 starter project and Unreal Engine 5.6.1-CSS. Place this plugin's `Source`, `Content`, `Tests` and `TsabsPassengerWagon.uplugin` in a `Mods/TsabsPassengerWagon` directory within a matching development project, and build/package the `FactoryGameSteam` Win64 Shipping target with Alpakit. These are recorded environment and package inputs, not a guarantee of a bit-identical rebuild.

The matching prebuilt release is Windows/Steam only; current Linux/Proton compatibility and multiplayer behavior remain unverified. Some historical helper scripts and CI checks on the default branch target older releases; do not treat them as proof for this candidate.
