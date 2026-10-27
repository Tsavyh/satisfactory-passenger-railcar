![Satisfactory Passenger Railcar — original railway artwork](assets/banner.png)

# Satisfactory Passenger Railcar

**Ride along. Let the locomotive do the driving.**  
An unofficial, open-source passenger wagon for **Satisfactory on Steam**, by **Tsabs**.

[![License: MIT](https://img.shields.io/badge/license-MIT-78a898?style=flat-square)](LICENSE)
[![Preview: 0.1.15](https://img.shields.io/badge/preview-0.1.15-f3a64b?style=flat-square)](https://github.com/Tsavyh/satisfactory-passenger-railcar/releases/tag/v0.1.15)

**[Download 0.1.15](https://github.com/Tsavyh/satisfactory-passenger-railcar/releases/download/v0.1.15/satisfactory-passenger-railcar-0.1.15-windows-steam.zip)** · [Español](README.es.md) · [Build from source](BUILDING.md) · [Release notes EN/ES](RELEASE-NOTES.md) · [Changelog](CHANGELOG.md)

> **Experimental preview.** Singleplayer user acceptance covers the 0.1.14 rerailing fix and the 0.1.15 name, description, cost, availability beside Freight Car and construction. There is **no new 0.1.15 remote-client, Proton or dedicated-server acceptance**. Back up saves and use a test world. Release links identify the intended 0.1.15 artifacts; documentation alone does not establish that publication has completed.

## A passenger wagon, not another locomotive

- **One invisible technical passenger place per wagon**, with an exterior orbit camera.
- **No driving controls, cargo inventory, walkable interior or visible seating.**
- Boarding and leaving require speed magnitude **strictly below 30 km/h**, forward or reverse. At exactly 30 km/h or above, access is blocked. Normal authority, occupancy and native safe-exit checks still apply; travel speed is not capped.
- Find **Passenger Railcar** (**Vagón de pasajeros** in Spanish) under **Transport → Railway**, immediately after **Freight Car**; labels may be localized.
- Base construction cost: **5 Modular Frames** (ordinary, not Heavy Modular Frames), **10 Steel Pipes** and **10 Steel Beams**. Progression follows the native Freight Car: **Tier 6 — Monorail Train Technology** (`MonorailTechnology`). The old free/no-unlock prototype rules no longer apply.

## New since the public 0.1.13 release

- **Native rerailing restored in 0.1.14**, with the derailment guard and authority checks retained. The user accepted the in-game fix.
- **0.1.15 presentation and balance:** passenger name/description, the native Freight Car's base cost and progression, and construction beside the other railway vehicles. This singleplayer block was accepted by the user.
- The local-only crosshair suppression, minimal native black speed panel and dynamic **km/h** display from 0.1.13 are retained, including EN/ES passenger UI. Capacity remains **one**; this is not a two-place release.
- Preserve **MikelCalvo's PRs #1–#5** and original integration history: Linux policy tooling, cross-platform CI, offline release verification and verified packaging. See [contributors](CONTRIBUTORS.md).

The effective Leave-key hint appears only below 30 km/h. A rejected high-speed Leave attempt shows a **3-second** warning only to the affected passenger, then hides again while too fast; slowing clears it sooner. **Host and clients must all update to the same version.**

## Install the preview

**Target:** Satisfactory Steam **1.2.4.1 / CL 513885** with **SML 3.12.0**. Other game versions and storefronts are unverified.

1. Install SML separately with Satisfactory Mod Manager. If SML-only startup works, keep that setup.
2. Close the game. Download **the named Windows/Steam ZIP above** once the release is published, not GitHub's automatic **Source code** archives.
3. Back up saves and move the old wagon folder **outside** `FactoryGame/Mods`. Copy `VAGON/TsabsPassengerWagon` from the ZIP into `FactoryGame/Mods`. Do not mix versions or replace SML.
4. First reach the menu only and check version **0.1.15**. Then use a test world, not a personal save. The recipe follows the native Freight Car unlock; it is not granted as a free prototype. No new Proton startup is certified by this release.

```text
FactoryGame/Mods/TsabsPassengerWagon/TsabsPassengerWagon.uplugin
```

The public ZIP has **nine members**: six runtime files under `VAGON/TsabsPassengerWagon/`, plus `LEEME.txt` (EN/ES), `LICENSE` and `MANIFEST.json`. **No optional SML profile or PDB is included.** SML is installed separately. Check the release's `SHA256SUMS.txt`. No SDK, Engine/game assets, private logs, saves or account data are distributed.

**If startup fails:** preserve that attempt's log privately, close the game, and remove only the wagon to return to SML-only startup. Redact account identifiers, authentication/travel parameters and personal paths before sharing evidence. [Report a problem](https://github.com/Tsavyh/satisfactory-passenger-railcar/issues/new?template=bug_report.yml).

## Evidence, without shortcuts

| Layer | Recorded scope / remaining limits |
| :--- | :--- |
| Windows / Steam | Fresh 0.1.15 Shipping package, successful real menu startup and zero streaming errors |
| Singleplayer user acceptance | 0.1.14 native rerailing; 0.1.15 name, description, listed cost, Freight Car availability/menu placement and construction; inherited 0.1.13 HUD acceptance |
| Editor and packaging | 43 successful Editor cases: 40 ordinary successes + 3 successes with SDK warnings (134 warning entries); Editor build and UAT/Shipping/cook exit 0 |
| GitHub automation | Portable policy, Python contract, verifier and packager checks; authentic prebuilt 0.1.9 package roundtrip. Workflow definitions are not proof of a successful run for 0.1.15 |
| Routing fixtures | New Windows CI coverage is intended to run 176 standalone cases: 150 rerail + 23 runtime availability + 3 Editor-branch availability. No hosted result is claimed; fixtures do not exercise Unreal, native RPC delivery, physics or actual in-game rerailing |
| Multiplayer user report | General SP/MP user approval is recorded for 0.1.13, not independent agent gameplay or certification of named platforms, roles or individual scenarios. Its rerailing exception was fixed and accepted in 0.1.14. No new 0.1.15 multiplayer acceptance |
| Linux / Proton | Historical 0.1.8 startup evidence only; [contributor report and limits](docs/PROTON-STARTUP.md). No new 0.1.15 Proton result |
| Travel and lifecycle | No new human-driver/autopilot, real-game save/load, death/disconnection or saved-passenger restoration acceptance |
| Dedicated servers / other storefronts | No validated release provided |

GitHub can run checks that do not need the proprietary game/SDK. Hosted CI does **not** replace the real Editor build/cook, game startup or human gameplay checks. Development of 0.1.16 and further gameplay changes is paused.

## Own-source MIT, exact release inputs

The project's own source is **[MIT](LICENSE)**. The release target is the [v0.1.15 source snapshot](https://github.com/Tsavyh/satisfactory-passenger-railcar/tree/v0.1.15); [`SOURCE-MANIFEST.json`](SOURCE-MANIFEST.json) is the release record for the **66 exact reviewed plugin inputs**, distinct from public wrappers and documentation. Final publication must include that matching source snapshot and manifest. Build paths/toolchains may prevent bit-identical rebuilds; source correspondence is not a reproducible-build claim.

- [Build instructions and test commands](BUILDING.md) · [Third-party boundaries](NOTICES.md)
- [Contributors](CONTRIBUTORS.md): **MikelCalvo**, PRs #1–#5, Linux tooling, scoped Proton evidence, CI and release tools; original merge history retained.
- [Public identity and release checklist](docs/BRANDING.md)

`TsabsPassengerWagon` remains the technical plugin/module/folder name for compatibility. Obtain the SDK, Engine and game resources separately; MIT does not relicense them.

---
Unofficial community project. Cover artwork is original branding, **not a gameplay screenshot**.
