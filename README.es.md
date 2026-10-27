![Satisfactory Passenger Railcar — ilustración ferroviaria original](assets/banner.png)

# Satisfactory Passenger Railcar

**Viaja en el tren. Deja que la locomotora conduzca.**  
Vagón de pasajeros no oficial para **Satisfactory en Steam**, de **Tsabs**, con código propio abierto bajo **MIT**.

**[Descargar 0.1.15](https://github.com/Tsavyh/satisfactory-passenger-railcar/releases/download/v0.1.15/satisfactory-passenger-railcar-0.1.15-windows-steam.zip)** · [English](README.md) · [Compilar](BUILDING.es.md) · [Notas EN/ES](RELEASE-NOTES.md) · [Historial](CHANGELOG.md)

> **Versión experimental.** La aceptación humana singleplayer cubre el reenrailado 0.1.14 y el bloque 0.1.15 de nombre, descripción, coste, disponibilidad junto al vagón de carga y construcción. **No hay nueva aceptación 0.1.15 en cliente remoto, Proton ni servidor dedicado.** Haz copia de seguridad y utiliza una partida de pruebas. Los enlaces identifican los artefactos previstos de 0.1.15; la documentación no acredita por sí sola que ya estén publicados.

## Un vagón para viajar, no otra locomotora

- **Una plaza técnica invisible por vagón**, con cámara exterior orbital.
- **Sin conducción, inventario de carga, interior transitable ni sillas visibles.**
- Subida y bajada con velocidad absoluta **estrictamente inferior a 30 km/h**, también marcha atrás. A 30 km/h o más se bloquea el acceso. Se mantienen los controles normales de autoridad, ocupación y salida nativa segura; no se limita la velocidad de circulación.
- Busca **Vagón de pasajeros** en **Transporte → Ferrocarril**, inmediatamente después del **vagón de carga**; las etiquetas dependen del idioma.
- Coste base: **5 armazones modulares normales** (no pesados), **10 tuberías de acero** y **10 vigas de acero**. La progresión sigue al vagón de carga nativo: **Tier 6 — Tecnología de tren monorraíl** (`MonorailTechnology`). Las antiguas reglas de prototipo gratuito y sin desbloqueo ya no están vigentes.

## Novedades desde la versión pública 0.1.13

- **Reenrailado nativo recuperado en 0.1.14**, conservando el guard de descarrilamiento y la autoridad. El usuario aceptó su funcionamiento en el juego.
- **Presentación y equilibrio 0.1.15:** nombre y descripción de pasajero, coste base y progresión del vagón de carga nativo, y construcción junto a los demás vehículos ferroviarios. El usuario aceptó este bloque en singleplayer.
- Se conservan la ocultación local de la mira, el velocímetro nativo mínimo de fondo negro y la velocidad dinámica en **km/h** de 0.1.13, incluida la interfaz EN/ES. La capacidad sigue siendo **una plaza**; no se añaden dos plazas.
- Se conservan las **PR #1–#5 de MikelCalvo** y su historial original de integración: herramientas de política Linux, CI multiplataforma, verificación offline y empaquetado verificado. Consulta [colaboradores](CONTRIBUTORS.md).

La indicación de la tecla efectiva para bajar aparece solo por debajo de 30 km/h. Un intento rechazado a mayor velocidad muestra un aviso de **3 segundos** únicamente al pasajero afectado; después vuelve a ocultarse si sigue rápido. Al reducir la velocidad se limpia antes. **Anfitrión y clientes deben actualizar a la misma versión.**

## Instalación

**Entorno objetivo:** Satisfactory Steam **1.2.4.1 / CL 513885** y **SML 3.12.0**. Otras versiones y tiendas no están verificadas.

1. Instala SML por separado mediante Satisfactory Mod Manager. Si ya arranca SML solo, conserva esa configuración.
2. Cierra el juego y descarga **el ZIP Windows/Steam indicado arriba** cuando se publique, no los archivos automáticos **Source code** de GitHub.
3. Haz copia de seguridad de las partidas y mueve el vagón anterior **fuera** de `FactoryGame/Mods`. Copia `VAGON/TsabsPassengerWagon` del ZIP a `FactoryGame/Mods`. No mezcles versiones ni sustituyas SML.
4. Llega primero solo al menú y comprueba que figura **0.1.15**. Después usa una partida de pruebas, no una personal. La receta sigue el desbloqueo nativo del vagón de carga; no se concede como prototipo gratuito. Esta versión no certifica un nuevo arranque Proton.

```text
FactoryGame/Mods/TsabsPassengerWagon/TsabsPassengerWagon.uplugin
```

El ZIP público contiene **nueve miembros**: seis archivos de ejecución bajo `VAGON/TsabsPassengerWagon/`, más `LEEME.txt` (EN/ES), `LICENSE` y `MANIFEST.json`. **No incluye perfil SML opcional ni PDB.** SML se instala aparte. Verifica `SHA256SUMS.txt` de la release. No se distribuyen SDK, Engine, recursos del juego, logs privados, guardados ni datos de cuentas.

**Si falla el arranque:** conserva en privado el log de ese intento, cierra el juego y retira solo el vagón para volver a SML solo. Oculta identificadores, parámetros de autenticación/viaje y rutas personales antes de compartir evidencia. [Comunicar un problema](https://github.com/Tsavyh/satisfactory-passenger-railcar/issues/new?template=bug_report.yml).

## Qué está verificado y qué falta

| Capa | Alcance registrado / límites |
| :--- | :--- |
| Windows / Steam | Paquete Shipping 0.1.15 recién generado, menú real correcto y cero errores Streaming |
| Aceptación humana singleplayer | Reenrailado nativo 0.1.14; nombre, descripción, coste indicado, disponibilidad/menú junto al vagón de carga y construcción 0.1.15; HUD 0.1.13 heredado |
| Editor y empaquetado | 43 casos Editor correctos: 40 éxitos ordinarios + 3 con avisos SDK (134 entradas de aviso); compilación Editor y UAT/Shipping/cook con salida 0 |
| Automatización GitHub | Políticas portables, contratos Python, verificador y empaquetador; roundtrip del paquete precompilado auténtico 0.1.9. Tener workflows no demuestra una ejecución correcta para 0.1.15 |
| Fixtures de enrutamiento | Nueva cobertura CI Windows prevista: 176 casos standalone, 150 de reenrailado + 23 de disponibilidad runtime + 3 de rama Editor. Sin resultado alojado atribuido; no ejecutan Unreal, entrega RPC nativa, física ni reenrailado real |
| Informe humano multijugador | Se conserva la aprobación humana general SP/MP de 0.1.13, no una prueba independiente del agente ni certificación de plataformas, roles o escenarios concretos. Su excepción de reenrailado se corrigió y aceptó en 0.1.14. Sin nueva aceptación multijugador 0.1.15 |
| Linux / Proton | Solo evidencia histórica de arranque 0.1.8; [informe del colaborador y límites](docs/PROTON-STARTUP.md). Sin nuevo resultado Proton 0.1.15 |
| Viaje y ciclo de vida | Sin nueva aceptación de conductor humano/piloto automático, guardado/carga real, muerte/desconexión ni restauración del pasajero guardado |
| Servidor dedicado / otras tiendas | Sin versión validada |

GitHub puede ejecutar las comprobaciones que no necesitan el juego/SDK propietario. La CI alojada **no sustituye** compilación Editor/cook real, arranque del juego ni pruebas humanas. El desarrollo de 0.1.16 y los cambios adicionales de jugabilidad siguen pausados.

## Código propio MIT y entradas exactas

Código propio bajo **[MIT](LICENSE)**. El destino de publicación es el [snapshot v0.1.15](https://github.com/Tsavyh/satisfactory-passenger-railcar/tree/v0.1.15); [`SOURCE-MANIFEST.json`](SOURCE-MANIFEST.json) es el registro de las **66 entradas exactas revisadas del plugin**, distintas de wrappers públicos y documentación. La publicación final debe incorporar ese snapshot y manifiesto correspondientes. No se promete recompilación idéntica bit a bit con otras rutas o herramientas.

- [Compilación y pruebas](BUILDING.es.md) · [Límites de terceros](NOTICES.md)
- [Colaboradores](CONTRIBUTORS.md): **MikelCalvo**, PR #1–#5, herramientas Linux, evidencia Proton acotada, CI y herramientas de release; historial original conservado.
- [Identidad y checklist público](docs/BRANDING.md)

`TsabsPassengerWagon` sigue siendo el identificador técnico de plugin, módulo y carpeta. SDK, Engine y recursos del juego se obtienen aparte; MIT no cambia sus licencias.

---
Proyecto comunitario no oficial. La portada es una ilustración original de marca, **no una captura del juego**.
