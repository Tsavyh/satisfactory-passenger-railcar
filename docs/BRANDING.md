# Public identity and release checklist

## Stable identity

- **Public name:** Satisfactory Passenger Railcar
- **Repository:** `Tsavyh/satisfactory-passenger-railcar`
- **Author:** Tsabs
- **Own-code license:** MIT
- **Internal plugin/module/folder:** `TsabsPassengerWagon` — keep stable unless a separately planned migration is approved.
- **Canonical project URL:** https://github.com/Tsavyh/satisfactory-passenger-railcar

Use this identity consistently in the README, repository About section, release titles and future download labels. Preserve the author's credit without using an internal prototype name as the public headline.

## Presentation

- Primary README and release summary in clear English, with a prominent Spanish README link and equivalent installation/safety warnings.
- Dark graphite, warm amber, restrained typography and the original route-map artwork in `assets/`. No fabricated gameplay screenshots or third-party artwork.
- Put the installable download and current limitations before developer details. Use short sections, readable tables and correctly formatted Markdown lists.
- Use honest status labels: a passing editor test is not runtime compatibility; Windows startup is not Proton or multiplayer validation. Keep experimental releases marked as such.
- Do not silently increase capacity or imply an interior, cargo support, driving controls or dedicated-server support.

## Every future release

- [ ] Public name and canonical repository URL are consistent.
- [ ] English/Spanish download, requirements and install instructions match the release version.
- [ ] The default installable asset is easy to identify; Source code downloads are explained.
- [ ] Release title: `Satisfactory Passenger Railcar VERSION — short user-facing change`.
- [ ] Future asset pattern: `satisfactory-passenger-railcar-VERSION-windows-steam.zip`; keep `TsabsPassengerWagon` inside the package. Do not rename old frozen assets.
- [ ] Changes, real verification and remaining limitations are distinct. Never remove the Proton warning without a real Proton result.
- [ ] Own-source code, tests, build instructions and MIT license accompany the exact binary version/tag.
- [ ] No SDK/Engine/game assets, private diagnostics, saves, account identifiers or unnecessary debug symbols.
- [ ] Download CRC/hash, source/tag correspondence, links, README image rendering and release state are verified after upload.
- [ ] Preserve old tags, binaries and checksums. Presentation-only changes do not invent a new runtime version.

The 0.1.8 asset keeps its historical filename because it was already published and verified. Repository links may change after a rename, but published source snapshots and runtime artifacts remain frozen.
