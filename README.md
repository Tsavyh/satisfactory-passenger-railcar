![Satisfactory Passenger Railcar — original railway artwork](assets/banner.png)

# Satisfactory Passenger Railcar

**Ride along. Let the locomotive do the driving.**  
An unofficial, open-source passenger wagon for **Satisfactory on Steam**, by **Tsabs**.

[![License: MIT](https://img.shields.io/badge/license-MIT-78a898?style=flat-square)](LICENSE)
[![Preview: 0.1.13](https://img.shields.io/badge/preview-0.1.13-f3a64b?style=flat-square)](https://github.com/Tsavyh/satisfactory-passenger-railcar/releases/tag/v0.1.13)

**[Download 0.1.13](https://github.com/Tsavyh/satisfactory-passenger-railcar/releases/download/v0.1.13/satisfactory-passenger-railcar-0.1.13-windows-steam.zip)** · [Español](README.es.md) · [Build from source](BUILDING.md) · [Changelog](CHANGELOG.md)

> **Experimental preview.** The passenger HUD and railway-menu placement have singleplayer user acceptance. **The changed exit hint and new HUD still need a remote-client check.** Multiplayer and Proton evidence from 0.1.8 is historical, not certification of 0.1.13. Back up saves and use a test world.

## A passenger wagon, not another locomotive

- **One invisible technical passenger place per wagon**, with an exterior orbit camera.
- **No driving controls, cargo inventory, walkable interior or visible seating.**
- Boarding and leaving require speed magnitude **strictly below 30 km/h**, forward or reverse. At exactly 30 km/h or above, access is blocked. Normal authority, occupancy and native safe-exit checks still apply; travel speed is not capped.
- Free construction and no progression unlock during this experimental phase. Final balance and extra capacity are undecided.

## New since the public 0.1.10 release

- **A quieter passenger view:** the crosshair is hidden locally only while riding as a passenger, without hiding unrelated players' UI.
- **A minimal native speed panel:** black background, dynamically updated speed magnitude and **km/h**. No MW readout, scale, visor or title.
- **Find it beside the other railway vehicles:** **Transport → Railway**, immediately after **Freight Car** (labels may be localized).
- Inherited lifecycle regression tests remain included. Local 0.1.11 and 0.1.12 development steps are combined into this 0.1.13 preview; they were not separate public releases.

The effective Leave-key hint remains visible only below 30 km/h. A rejected high-speed Leave attempt shows a **3-second** warning only to the affected passenger, then hides again while too fast; slowing clears it sooner. **Host and clients must all update to the same version.**

## Install the preview

**Target:** Satisfactory Steam **1.2.4.1 / CL 513885** with **SML 3.12.0**. Other game versions and storefronts are unverified.

1. Install SML separately with Satisfactory Mod Manager. If SML-only startup works, keep that setup.
2. Close the game. Download **the named Windows/Steam ZIP above**, not GitHub's automatic **Source code** archives.
3. Back up saves and move the old wagon folder **outside** `FactoryGame/Mods`. Copy `VAGON/TsabsPassengerWagon` from the ZIP into `FactoryGame/Mods`. Do not mix versions or replace SML.
4. First reach the menu only and check version **0.1.13**. Then use a test world, not a personal save. On Proton, keep existing Proton/launch settings unchanged for the startup check.

```text
FactoryGame/Mods/TsabsPassengerWagon/TsabsPassengerWagon.uplugin
```

The ZIP contains six runtime files, EN/ES instructions, MIT license, an optional generic **SML-only** profile and a member-hash manifest. The profile neither includes nor installs this wagon or SML binaries. Check the release's `SHA256SUMS.txt`. No SDK, Engine/game assets, private logs, saves, account data or PDB files are distributed.

**If startup fails:** preserve that attempt's log privately, close the game, and remove only the wagon to return to SML-only startup. Redact account identifiers, authentication/travel parameters and personal paths before sharing evidence. [Report a problem](https://github.com/Tsavyh/satisfactory-passenger-railcar/issues/new?template=bug_report.yml).

## Evidence, without shortcuts

| Layer | Verified scope / remaining limits |
| :--- | :--- |
| Windows / Steam | 0.1.13 real DX11 menu startup; native prerequisite loaded; zero streaming errors |
| Singleplayer user acceptance | 0.1.11 HUD functionality, 0.1.12 minimal-panel appearance and 0.1.13 railway-menu placement accepted |
| Automated checks | 39 editor cases: 36 without warnings + 3 with SDK warnings (134 warning entries); 7 package checks; exported MSVC policy tests: 26 precise + 26 fast; 2 native structural checks |
| Remote-client multiplayer | User-reported two-player approval belongs to 0.1.8; changed 0.1.10 hint and 0.1.11–0.1.13 UI remain pending a grouped remote check |
| Linux / Proton | Historical 0.1.8 startup evidence only; [contributor report and limits](docs/PROTON-STARTUP.md). No new Proton, GCC/Clang or hosted CI run claimed for 0.1.13 |
| Human-driver / autopilot travel | Separate acceptance tests still pending |
| Save/load, death/disconnection and native lifecycle | Editor regressions are not real-game scenario acceptance |
| Dedicated servers / other storefronts | No validated release provided |

## Own-source MIT, exact release inputs

The project's own source is **[MIT](LICENSE)**. The [v0.1.13 source snapshot](https://github.com/Tsavyh/satisfactory-passenger-railcar/tree/v0.1.13) and [`SOURCE-MANIFEST.json`](SOURCE-MANIFEST.json) identify the **58 reviewed plugin inputs** used for this binary, plus inherited public wrappers and documentation. Build paths/toolchains may prevent bit-identical rebuilds; source correspondence is not a reproducible-build claim.

- [Build instructions and test commands](BUILDING.md) · [Third-party boundaries](NOTICES.md)
- [Contributors](CONTRIBUTORS.md): includes [PR #1](https://github.com/Tsavyh/satisfactory-passenger-railcar/pull/1) by **MikelCalvo**, with Linux tooling and scoped Proton evidence; its original merge history is preserved.
- [Public identity and release checklist](docs/BRANDING.md)

`TsabsPassengerWagon` remains the technical plugin/module/folder name for compatibility. Obtain the SDK, Engine and game resources separately; MIT does not relicense them.

---
Unofficial community project. Cover artwork is original branding, **not a gameplay screenshot**.
