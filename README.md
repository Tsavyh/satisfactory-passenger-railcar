![Satisfactory Passenger Railcar — original abstract railway cover](assets/banner.png)

# Satisfactory Passenger Railcar

**Ride along. Let the locomotive do the driving.**  
An unofficial, open-source passenger wagon mod for **Satisfactory on Steam**, by **Tsabs**.

[![License: MIT](https://img.shields.io/badge/license-MIT-78a898?style=flat-square)](LICENSE)
[![Preview: 0.1.10](https://img.shields.io/badge/preview-0.1.10-f3a64b?style=flat-square)](https://github.com/Tsavyh/satisfactory-passenger-railcar/releases/tag/v0.1.10)
![Status: Experimental](https://img.shields.io/badge/status-experimental-f3a64b?style=flat-square)

**[Download 0.1.10](https://github.com/Tsavyh/satisfactory-passenger-railcar/releases/download/v0.1.10/satisfactory-passenger-railcar-0.1.10-windows-steam.zip)** · [Español](README.es.md) · [Build from source](BUILDING.md) · [Report an issue](https://github.com/Tsavyh/satisfactory-passenger-railcar/issues/new/choose)

> **Preview release.** The tester accepted the three exit-hint/feedback checks in **singleplayer on 0.1.10**. **The changed hint still needs a remote-client check.** Baseline 0.1.8 had two-player approval and the affected Proton client passed startup. Back up saves and use a test world.

## What it does

- Adds a wagon that can be coupled to a train, with **one invisible passenger anchor per wagon** in this prototype.
- Provides an **exterior orbit camera** without locomotive driving controls.
- Has **no cargo inventory, walkable interior or visible seating**.
- Allows boarding and leaving only below **30 km/h in either direction**, subject to the normal authority, occupancy and safe-exit checks. At exactly 30 km/h or above, access is blocked. This does not cap the train's travel speed.

The prototype is available without a construction cost or progression unlock during testing. Final balance and additional passenger capacity are not decided yet.

## New in 0.1.10

The ordinary Leave-key instruction appears **only below 30 km/h** in either direction; at 30 or above it is hidden. A rejected Leave attempt shows the speed warning for **3 seconds**, then hides it again while still too fast. Slowing below 30 restores the effective-key instruction. The tester accepted these three checks in singleplayer. **Both host and clients must update.**

PR #1 adds Linux policy-test tooling and a scoped Proton report without changing runtime policy. [Changelog](CHANGELOG.md).

## Install the preview

**Target build:** Satisfactory Steam **1.2.4.1 / CL 513885** with **SML 3.12.0**. Other game builds and storefronts have not been verified.

1. Install SML with Satisfactory Mod Manager first. If SML-only startup already works, keep that setup.
2. Close the game and download **[the installable ZIP](https://github.com/Tsavyh/satisfactory-passenger-railcar/releases/download/v0.1.10/satisfactory-passenger-railcar-0.1.10-windows-steam.zip)**. GitHub's **Source code** downloads are for developers, not an installable mod.
3. Back up the previous wagon folder **outside** `FactoryGame/Mods`, then copy `VAGON/TsabsPassengerWagon` from the ZIP into `FactoryGame/Mods`. Keep SML separate and do not mix wagon versions.
4. **First run: reach the menu only.** Check that version **0.1.10** is listed before loading a test world. On Proton, keep the existing Proton version and launch options unchanged for this check.

Expected file location:

```text
FactoryGame/
└── Mods/
    ├── SML/
    └── TsabsPassengerWagon/
        └── TsabsPassengerWagon.uplugin
```

The ZIP includes an installation guide, MIT license, optional SML-only profile and integrity manifest. It does **not** include SML, the SDK or personal data. See the release's `SHA256SUMS.txt` to check the download.

<details>
<summary><strong>If startup fails</strong></summary>

Preserve the log from that attempt before starting the game again. Close the game and remove **only** the wagon folder to return to SML-only startup. Do not change several settings at once. Before sharing logs, redact account identifiers, authentication/travel URL parameters and personal paths. Use the [bug report form](https://github.com/Tsavyh/satisfactory-passenger-railcar/issues/new?template=bug_report.yml).
</details>

## Compatibility at a glance

| Area | Current evidence |
| :--- | :--- |
| Windows / Steam | 0.1.10 menu startup verified with DX11; three exit-hint/feedback checks accepted in singleplayer |
| Linux / Steam / Proton | 0.1.8 reached the menu on the affected Arch Linux / Proton 10.0-4 client; [startup report and limits](docs/PROTON-STARTUP.md) |
| Client/server multiplayer | 0.1.8 received two-player user-reported approval; new 0.1.10 notice pending focused remote check |
| Passenger travel with a human driver or autopilot | Separate acceptance tests still pending |
| Dedicated servers / other storefronts | No validated release provided |

**0.1.8 startup fix:** explicitly retains a required native game module. In the reproduced Windows DX11 case, streaming errors fell from **454 to 0**. The affected Proton client subsequently reached the menu with zero streaming errors too; subsequent two-player multiplayer approval was reported separately. This is not exhaustive platform/lifecycle certification. [Full changelog](CHANGELOG.md).

## Source, tests and contributions

The project's own code is licensed under **[MIT](LICENSE)**. The [v0.1.10 tag](https://github.com/Tsavyh/satisfactory-passenger-railcar/tree/v0.1.10) matches the published binary; [`SOURCE-MANIFEST.json`](SOURCE-MANIFEST.json) records the frozen release inputs. Includes the transient exit feedback introduced in 0.1.9 and [PR #1](https://github.com/Tsavyh/satisfactory-passenger-railcar/pull/1) by [MikelCalvo](https://github.com/MikelCalvo). See [contributors](CONTRIBUTORS.md).

- [Build instructions and test commands](BUILDING.md)
- [Third-party boundaries](NOTICES.md)
- [Public presentation and release checklist](docs/BRANDING.md)
- [Issues and feedback](https://github.com/Tsavyh/satisfactory-passenger-railcar/issues)

`TsabsPassengerWagon` remains the internal plugin identifier and folder name for compatibility. The public name is **Satisfactory Passenger Railcar**. The SDK, Engine and game assets are obtained separately, not redistributed here.

---

An unofficial community project for Satisfactory. Cover artwork is original project branding, **not a gameplay screenshot**.
