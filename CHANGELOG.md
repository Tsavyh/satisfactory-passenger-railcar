# Changelog

## 0.1.13 — A quieter passenger view, in the railway menu

- Hide the crosshair locally only for the current passenger; preserve forwarding and unrelated UI.
- Show a minimal native black panel with dynamic speed magnitude and km/h only: no MW, scale, visor or title.
- Place the wagon under Transport → Railway, immediately after Freight Car; keep free/unlocked experimental construction.
- Combine the local-only 0.1.11 functional HUD and 0.1.12 visual refinement into this public preview. One invisible technical place, orbit camera, strict below-30 km/h access in either direction and normal guards remain unchanged.
- Include inherited lifecycle regressions and new HUD/composition/menu coverage. 39 editor cases passed (36 without warnings + 3 with SDK warnings; 134 warning entries), seven package checks, Editor and Shipping/cook exit 0, and real Windows DX11 menu startup with zero streaming errors.
- Exported MSVC 14.38 tests: 26 precise + 26 fast; two native prerequisite contracts. Singleplayer user acceptance covers HUD function, minimal appearance and menu placement, not new remote-client delivery.
- Historical two-player/Proton evidence remains scoped to 0.1.8. The changed 0.1.10 hint and new HUD need a grouped remote check; human-driver/autopilot travel and real-game lifecycle scenarios remain pending. No new GCC/Clang/Proton/hosted CI claim.
- Preserve current main history, PR #1 by MikelCalvo, MIT own-source license and original branding. Host and clients must all update to the same version.

## 0.1.10 — The exit hint, only when speed allows

- Show the effective Leave-key instruction only below 30 km/h in magnitude. Hide it at 30 or above in both directions; do not hardcode E.
- Keep the3-second, owning-passenger rejection warning after a rejected Leave attempt. Return to hidden while still too fast; restore the ordinary hint below 30. No change to authority, occupancy, native safe exit, speed policy, camera or driving controls.
- Collapse only the owned interaction widget so the parent HUD can keep ticking. Extend boundary/reverse/non-finite/visibility regression checks.
- 27 editor cases passed (24 without warnings+3 with SDK/UI warnings), seven package checks, real Windows DX11 menu startup and independent static review passed. The tester accepted all three focused visual/feedback checks in **singleplayer**. Remote-client verification of this new UI remains pending.
- Preserve inherited PR#1 byMikelCalvo, MIT own-source code and the existing public branding. Both host and clients must update.

## 0.1.9 — Feedback when you ask to leave

- Replace the permanent speed restriction warning with a **3-second, owning-passenger notice after a server-rejected Leave attempt**. Clear sooner when speed becomes permitted; no automatic exit, relaxed speed limit or driving-control change.
- Preserve authority, occupancy, safe-exit geometry, camera, effective bindings and the native startup prerequisite.
- Merge [PR #1](https://github.com/Tsavyh/satisfactory-passenger-railcar/pull/1) by **MikelCalvo**: Linux Bash policy runner, failure/cleanup tests, IEEE-754 fixture inputs for Clang fast-math, English/Spanish developer guidance and a scoped Proton startup report.
- Record the subsequent **user-reported two-player multiplayer approval of0.1.8** separately from that menu-only report. The new0.1.9 notice has editor/package/startup verification, not yet a real remote-client acceptance.
- Keep public branding and own-source **MIT** code. Both host and clients should update to0.1.9.

## 0.1.8 — Startup fix preview

- Explicitly retains the native `FactoryDedicatedClient` prerequisite through the `FGAbstractServerWidget` class reference.
- In a reproduced Windows DX11 startup, streaming errors fell from **454 to 0** and SML/the menu initialized.
- Leaves camera, HUD, effective bindings, authority/occupancy/safe-exit guards and the strict **below 30 km/h** access policy unchanged.
- First release with the project's complete own-source code, tests and build instructions under **MIT**.
- **Proton:** candidate fix; the affected client's startup block is not yet verified as resolved. Multiplayer remains unverified.

### Presentation refresh (no new binary)

The public project is now **Satisfactory Passenger Railcar**, with English/Spanish documentation and original railway-map cover artwork. The internal plugin identifier, release tags, code and binary assets are unchanged.

## 0.1.7 — Historical preview

- Deferred local HUD asset loading to avoid loading broad UI/Niagara assets from the native HUD constructor.
- Introduced the current below-30-km/h access policy.
- A startup block was reported on Proton; keep that compatibility warning when referring to this historical version.
