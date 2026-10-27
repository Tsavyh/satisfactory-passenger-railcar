# Satisfactory Passenger Railcar 0.1.15

[English](#english) · [Español](#español)

## English

**Experimental Windows/Steam preview · own source under MIT.** These notes are prepared for the 0.1.15 release; publication and verification of the hosted release are separate steps. Development of 0.1.16 and further gameplay changes remains paused.

### Changes since public 0.1.13

- Include the **0.1.14 native rerailing fix**, accepted by the user in game. Keep the derailment guard and authority checks; the fix is not permission to bypass native safety restrictions.
- Present the wagon as **Passenger Railcar / Vagón de pasajeros**, with the updated passenger description, no longer a free prototype.
- Match native **Freight Car** base construction cost: **5 ordinary Modular Frames, 10 Steel Pipes and 10 Steel Beams**. These are not Heavy Modular Frames, and the steel items are pipes **and beams**, not two sets of pipes.
- Follow native Freight Car progression: **Tier 6 — Monorail Train Technology** (`MonorailTechnology`), under **Transport → Railway**, immediately after Freight Car. Free/no-unlock prototype rules are obsolete.
- Retain **one invisible technical passenger place**, exterior orbit camera, local crosshair suppression, minimal native black speed panel with dynamic **km/h**, EN/ES UI and the strict **below 30 km/h** boarding/leaving threshold in either direction. No two-place capacity or new saved-passenger restoration feature.

### Evidence and limits

The user accepted the 0.1.15 **singleplayer** block of name, description, listed cost, availability beside Freight Car and construction; the rerailing fix was accepted in 0.1.14. The 0.1.13 passenger HUD acceptance is inherited.

Recorded release preparation: **43 successful Editor cases — 40 ordinary successes + 3 with SDK warnings, 134 warning entries**; Editor build and UAT/Shipping/cook exit **0**; fresh Shipping package and real Windows/Steam menu startup with **zero streaming errors**. These are recorded results, not new tests performed by editing these notes or by hosted CI.

The **general human SP/MP approval of 0.1.13** is retained: a user report, not independent agent gameplay or certification of named platforms, player roles or individual scenarios. Its later rerailing exception was fixed and accepted in 0.1.14. There is **no new 0.1.15 multiplayer, Proton or dedicated-server acceptance**. The scoped Proton startup report remains historical **0.1.8** evidence. Do not infer real-game save/load, death/disconnection, human-driver/autopilot travel or saved-passenger restoration acceptance. All participants must update to the same mod version.

GitHub can run standalone policy/contract tests, verifier/packager suites and an authentic **prebuilt 0.1.9** payload roundtrip. New Windows routing CI is intended to run **176 cases: 150 rerail + 23 runtime availability + 3 Editor-branch availability**, against explicit stand-ins, **not Unreal, real native RPC delivery, physics or actual rerailing**. Workflow definitions are not successful-run evidence; no new hosted routing result is claimed. Without the proprietary SDK/game, hosted CI does not replace Editor compilation/cook, game startup or human checks; it does not publish or deploy this mod.

### Install and source

- Target: **Satisfactory Steam 1.2.4.1 / CL 513885, SML 3.12.0**. Other versions/storefronts are unverified.
- Expected asset: [`satisfactory-passenger-railcar-0.1.15-windows-steam.zip`](https://github.com/Tsavyh/satisfactory-passenger-railcar/releases/download/v0.1.15/satisfactory-passenger-railcar-0.1.15-windows-steam.zip). Prepared-package record: **134182 bytes**, SHA-256 `5f43412cd2b7a96f3d8adbcfdc03af37764ced30da6fd3ff4a9d1cdf5f0df750`. Compare your download with `SHA256SUMS.txt` from the same release; this record is not a new verification of a hosted download. GitHub's automatic source archives are not installable builds.
- Close the game, back up saves and move the old wagon outside `FactoryGame/Mods`. Copy `VAGON/TsabsPassengerWagon` to `FactoryGame/Mods`, without extra nesting. Keep SML installed separately. Start at the menu, confirm **0.1.15**, then use a test world.
- Public ZIP: **nine members**, comprising six runtime payloads plus `LEEME.txt`, `LICENSE` and `MANIFEST.json`. **No PDB or optional SML profile**, SDK, Engine/game assets, private logs, saves or account data.
- Source target: **[v0.1.15](https://github.com/Tsavyh/satisfactory-passenger-railcar/tree/v0.1.15)** and [`SOURCE-MANIFEST.json`](SOURCE-MANIFEST.json), recording **66 exact reviewed plugin inputs**. Final publication must include the matching sources and manifest. MIT covers project-owned source only; see [NOTICES.md](NOTICES.md). No bit-reproducible Unreal build is claimed.

**Credits:** Tsabs; **MikelCalvo**, whose PRs **#1–#5**, original commits and merge history are retained, including Linux tooling, scoped Proton evidence, cross-platform CI, release verification and packaging. See [CONTRIBUTORS.md](CONTRIBUTORS.md).

## Español

**Versión experimental Windows/Steam · código propio MIT.** Notas preparadas para la release 0.1.15; publicar y verificar la release alojada son pasos separados. El desarrollo de 0.1.16 y los cambios adicionales de jugabilidad siguen pausados.

### Cambios desde la pública 0.1.13

- Incluye la **corrección de reenrailado nativo 0.1.14**, aceptada por el usuario en juego. Se conservan el guard de descarrilamiento y la autoridad; no permite saltarse restricciones de seguridad nativas.
- Nombre **Vagón de pasajeros / Passenger Railcar** y descripción de pasajero actualizada: ya no es un prototipo gratuito.
- Coste base como el **vagón de carga nativo**: **5 armazones modulares normales, 10 tuberías de acero y 10 vigas de acero**. No son armazones pesados ni dos lotes de tuberías.
- Progresión como el vagón de carga: **Tier 6 — Tecnología de tren monorraíl** (`MonorailTechnology`), en **Transporte → Ferrocarril**, inmediatamente después del vagón de carga. Las reglas antiguas gratuitas/sin desbloqueo ya no están vigentes.
- Se conserva **una plaza técnica invisible**, cámara exterior orbital, ocultación local de mira, velocímetro nativo mínimo negro con **km/h** dinámicos, interfaz EN/ES y acceso estrictamente **por debajo de 30 km/h** en ambos sentidos. No añade dos plazas ni una nueva función de restauración del pasajero guardado.

### Evidencia y límites

El usuario aceptó el bloque **singleplayer** 0.1.15 de nombre, descripción, coste indicado, disponibilidad junto al vagón de carga y construcción; el reenrailado se aceptó en 0.1.14. Se hereda la aceptación del HUD 0.1.13.

Preparación de release registrada: **43 casos Editor correctos — 40 éxitos ordinarios + 3 con avisos SDK, 134 entradas de aviso**; compilación Editor y UAT/Shipping/cook con salida **0**; paquete Shipping nuevo y menú real Windows/Steam con **cero errores Streaming**. Son resultados registrados, no nuevas pruebas por editar estas notas ni resultados de CI alojada.

Se conserva la **aprobación humana general SP/MP de 0.1.13**: informe del usuario, no prueba independiente del agente ni certificación de plataformas, roles de jugadores o escenarios concretos. Su posterior excepción de reenrailado se corrigió y aceptó en 0.1.14. **Sin nueva aceptación 0.1.15 de multijugador, Proton ni servidor dedicado.** El informe acotado de arranque Proton sigue siendo evidencia histórica **0.1.8**. No implica aceptación de guardado/carga real, muerte/desconexión, viaje con conductor humano/piloto automático ni restauración del pasajero guardado. Todos los participantes deben actualizar a la misma versión del mod.

GitHub puede ejecutar políticas/contratos standalone, suites de verificador/empaquetador y roundtrip de payloads **precompilados auténticos 0.1.9**. La nueva CI Windows de enrutamiento está prevista para **176 casos: 150 de reenrailado + 23 de disponibilidad runtime + 3 de disponibilidad en rama Editor**, contra sustitutos explícitos, **no Unreal, entrega RPC nativa, física ni reenrailado real**. Definir workflows no acredita ejecuciones correctas; no se atribuye un nuevo resultado alojado de routing. Sin SDK/juego propietario, la CI alojada no sustituye compilación Editor/cook, arranque ni pruebas humanas; tampoco publica ni despliega este mod.

### Instalación y fuentes

- Objetivo: **Satisfactory Steam 1.2.4.1 / CL 513885, SML 3.12.0**. Otras versiones/tiendas no verificadas.
- Artefacto previsto: [`satisfactory-passenger-railcar-0.1.15-windows-steam.zip`](https://github.com/Tsavyh/satisfactory-passenger-railcar/releases/download/v0.1.15/satisfactory-passenger-railcar-0.1.15-windows-steam.zip). Registro del paquete preparado: **134182 bytes**, SHA-256 `5f43412cd2b7a96f3d8adbcfdc03af37764ced30da6fd3ff4a9d1cdf5f0df750`. Contrasta tu descarga con `SHA256SUMS.txt` de esa misma release; este registro no es una nueva verificación de descarga alojada. Los archivos automáticos de fuentes de GitHub no son compilaciones instalables.
- Cierra el juego, guarda copia de las partidas y mueve el vagón anterior fuera de `FactoryGame/Mods`. Copia `VAGON/TsabsPassengerWagon` a `FactoryGame/Mods`, sin anidar otra carpeta. SML va instalado aparte. Llega primero al menú, confirma **0.1.15** y usa después una partida de pruebas.
- ZIP público: **nueve miembros**, seis payloads runtime más `LEEME.txt`, `LICENSE` y `MANIFEST.json`. **Sin PDB ni perfil SML opcional**, SDK, Engine/recursos del juego, logs privados, guardados ni datos de cuentas.
- Fuentes previstas: **[v0.1.15](https://github.com/Tsavyh/satisfactory-passenger-railcar/tree/v0.1.15)** y [`SOURCE-MANIFEST.json`](SOURCE-MANIFEST.json), con **66 entradas exactas revisadas del plugin**. La publicación final debe incluir fuentes y manifiesto correspondientes. MIT solo cubre el código propio; consulta [NOTICES.md](NOTICES.md). No se promete compilación Unreal idéntica bit a bit.

**Créditos:** Tsabs; **MikelCalvo**, con sus PR **#1–#5**, commits originales e historial de integración conservados: herramientas Linux, evidencia Proton acotada, CI multiplataforma, verificación y empaquetado. Consulta [CONTRIBUTORS.md](CONTRIBUTORS.md).
