# Changelog

[Release notes EN/ES](RELEASE-NOTES.md) · [English](README.md) · [Español](README.es.md)

## 0.1.15 — Passenger identity, native cost and progression / Identidad, coste y progresión nativos

- Name and description identify the passenger wagon, including **Vagón de pasajeros** in Spanish; the old “free prototype” presentation is retired. / Nombre y descripción de vagón de pasajeros; se retira la presentación de «prototipo gratuito».
- Base cost: **5 ordinary Modular Frames + 10 Steel Pipes + 10 Steel Beams**, matching native Freight Car. Availability follows its **Tier 6 — Monorail Train Technology** (`MonorailTechnology`) unlock and menu placement in Transport → Railway. / Coste base: **5 armazones modulares normales + 10 tuberías de acero + 10 vigas de acero**, como el vagón de carga nativo; mismo desbloqueo Tier 6 y ubicación Transporte → Ferrocarril. Earlier free/no-unlock rules are historical, not current. / Las antiguas reglas gratuitas y sin desbloqueo son históricas, no actuales.
- User-reported singleplayer acceptance covers name, description, listed cost, availability beside Freight Car and construction. Gameplay development, including 0.1.16, remains paused. / El usuario aceptó en singleplayer nombre, descripción, coste indicado, disponibilidad junto al vagón de carga y construcción. El desarrollo de jugabilidad, incluida 0.1.16, sigue pausado.
- Retain the accepted 0.1.14 native rerailing fix, derailment guard and authority checks. Capacity stays at **one invisible technical place**, with exterior orbit camera and strict below-30 km/h access in either direction. The 0.1.13 local crosshair/minimal speed HUD and EN/ES presentation are inherited. / Se conservan reenrailado 0.1.14, guard de descarrilamiento, autoridad, una plaza técnica invisible, cámara orbital, acceso estrictamente por debajo de 30 km/h y HUD EN/ES heredado.
- Recorded verification: **43 successful Editor cases — 40 ordinary + 3 with SDK warnings, 134 warning entries**; Editor build and UAT/Shipping/cook exit 0; fresh Shipping package and real Windows/Steam menu startup with zero streaming errors. / Verificación registrada: **43 casos Editor correctos — 40 ordinarios + 3 con avisos SDK, 134 entradas**; compilación Editor y UAT/Shipping/cook con salida 0, paquete Shipping nuevo y menú real Windows/Steam sin errores Streaming.
- Release target: own source under **MIT**, tag **v0.1.15**, **66 exact reviewed plugin inputs** in `SOURCE-MANIFEST.json`; public runtime ZIP has **nine members** (six runtime payloads + `LEEME.txt`, `LICENSE`, `MANIFEST.json`), **no PDB and no optional SML profile**. Source correspondence is not a bit-reproducible build claim; publication remains a separate step. / Destino de release: fuentes propias MIT, tag v0.1.15, 66 entradas exactas; ZIP público de nueve miembros, sin PDB ni perfil SML opcional. No se promete recompilación idéntica y publicar es un paso aparte.
- Preserve **MikelCalvo's PRs #1–#5**, original commits, merge history and branding. Portable policy/Python checks, release verification and packaging/0.1.9 payload roundtrip belong in GitHub CI; workflow definitions are not new execution results. Routing fixtures are not Unreal or native RPC/gameplay tests. / Se conservan PR #1–#5 de MikelCalvo, commits, historial y marca. Las comprobaciones portables, verificación y empaquetado/roundtrip 0.1.9 se automatizan en GitHub; definir workflows no acredita ejecuciones nuevas. Las fixtures de enrutamiento no son pruebas de Unreal ni RPC/jugabilidad nativos.
- Retain general human SP/MP approval of 0.1.13: a user report, not independent agent gameplay or certification of named platforms, player roles or individual scenarios. Its rerailing exception was fixed and accepted in 0.1.14. No new 0.1.15 multiplayer, Proton, dedicated-server or real-game saved-passenger restoration acceptance. Host and clients must use the same version. / Se conserva la aprobación humana general SP/MP de 0.1.13: informe del usuario, no prueba independiente del agente ni certificación de plataformas, roles o escenarios concretos. Su excepción de reenrailado se corrigió y aceptó en 0.1.14. Sin nueva aceptación 0.1.15 de multijugador, Proton, dedicado ni restauración real del pasajero guardado; anfitrión y clientes deben usar la misma versión.

## 0.1.14 — Native rerailing / Reenrailado nativo

- Restore delegation to native rerailing while retaining the derailment guard and authority checks. The user accepted the in-game result. This local development step is included in 0.1.15, not represented here as a separate public release. / Se recupera la delegación al reenrailado nativo conservando el guard de descarrilamiento y la autoridad. El usuario aceptó el resultado en juego. Este paso local se integra en 0.1.15, sin presentarlo como publicación independiente.

## 0.1.13 — A quieter passenger view, in the railway menu

- Hide the crosshair locally only for the current passenger; preserve forwarding and unrelated UI.
- Show a minimal native black panel with dynamic speed magnitude and km/h only: no MW, scale, visor or title.
- Place the wagon under Transport → Railway, immediately after Freight Car; keep free/unlocked experimental construction.
- Combine the local-only 0.1.11 functional HUD and 0.1.12 visual refinement into this public preview. One invisible technical place, orbit camera, strict below-30 km/h access in either direction and normal guards remain unchanged.
- Include inherited lifecycle regressions and new HUD/composition/menu coverage. 39 editor cases passed (36 without warnings + 3 with SDK warnings; 134 warning entries), seven package checks, Editor and Shipping/cook exit 0, and real Windows DX11 menu startup with zero streaming errors.
- Exported MSVC 14.38 tests: 26 precise + 26 fast; two native prerequisite contracts. Singleplayer user acceptance covers HUD function, minimal appearance and menu placement, not new remote-client delivery.
- The initial release record retained historical two-player/Proton evidence from 0.1.8 and left a grouped remote UI check pending. A subsequent general human SP/MP approval of 0.1.13 supersedes that general multiplayer-pending status, without specifying platforms, roles or individual scenarios. The later rerailing exception was fixed and accepted in 0.1.14. No new GCC/Clang/Proton/hosted CI result is asserted by this historical entry.
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
