![Satisfactory Passenger Railcar — ilustración ferroviaria original](assets/banner.png)

# Satisfactory Passenger Railcar

**Viaja en el tren. Deja que la locomotora conduzca.**  
Vagón de pasajeros no oficial para **Satisfactory en Steam**, de **Tsabs**, con código propio abierto bajo **MIT**.

**[Descargar 0.1.13](https://github.com/Tsavyh/satisfactory-passenger-railcar/releases/download/v0.1.13/satisfactory-passenger-railcar-0.1.13-windows-steam.zip)** · [English](README.md) · [Compilar](BUILDING.es.md) · [Historial](CHANGELOG.md)

> **Versión experimental.** El HUD de pasajero y la posición en el menú ferroviario tienen aceptación humana en singleplayer. **Falta comprobar el indicador de salida y el nuevo HUD en cliente remoto.** La evidencia multijugador y Proton de 0.1.8 es histórica: no certifica la 0.1.13. Haz copia de seguridad y utiliza una partida de pruebas.

## Un vagón para viajar, no otra locomotora

- **Una plaza técnica invisible por vagón**, con cámara exterior orbital.
- **Sin conducción, inventario de carga, interior transitable ni sillas visibles.**
- Subida y bajada con velocidad absoluta **estrictamente inferior a 30 km/h**, también marcha atrás. A 30 km/h o más se bloquea el acceso. Se mantienen los controles normales de autoridad, ocupación y salida nativa segura; no se limita la velocidad de circulación.
- Durante esta fase es gratuito y no requiere desbloqueo. El equilibrio final y las plazas adicionales están pendientes.

## Novedades desde la versión pública 0.1.10

- **Vista despejada:** la mira se oculta localmente solo mientras ocupas la plaza de pasajero, sin ocultar la interfaz de otros jugadores.
- **Velocímetro nativo mínimo:** fondo negro, magnitud de velocidad actualizada dinámicamente y **km/h**. Sin MW, escala, visor ni título.
- **En su sección ferroviaria:** **Transporte → Ferrocarril**, inmediatamente después del **vagón de carga** (las etiquetas dependen del idioma).
- Se conservan las pruebas de regresión del ciclo de vida. Los pasos locales 0.1.11 y 0.1.12 se integran en esta 0.1.13; no fueron publicaciones independientes.

La indicación de la tecla efectiva para bajar sigue apareciendo solo por debajo de 30 km/h. Un intento rechazado a mayor velocidad muestra un aviso de **3 segundos** únicamente al pasajero afectado; después vuelve a ocultarse si sigue rápido. Al reducir la velocidad se limpia antes. **Anfitrión y clientes deben actualizar a la misma versión.**

## Instalación

**Entorno objetivo:** Satisfactory Steam **1.2.4.1 / CL 513885** y **SML 3.12.0**. Otras versiones y tiendas no están verificadas.

1. Instala SML por separado mediante Satisfactory Mod Manager. Si ya arranca SML solo, conserva esa configuración.
2. Cierra el juego y descarga **el ZIP Windows/Steam indicado arriba**, no los archivos automáticos **Source code** de GitHub.
3. Haz copia de seguridad de las partidas y mueve el vagón anterior **fuera** de `FactoryGame/Mods`. Copia `VAGON/TsabsPassengerWagon` del ZIP a `FactoryGame/Mods`. No mezcles versiones ni sustituyas SML.
4. Llega primero solo al menú y comprueba que figura **0.1.13**. Después usa una partida de pruebas, no una personal. En Proton, mantén la versión y las opciones de arranque existentes para comprobar el inicio.

```text
FactoryGame/Mods/TsabsPassengerWagon/TsabsPassengerWagon.uplugin
```

El ZIP contiene seis archivos de ejecución, instrucciones EN/ES, licencia MIT, perfil genérico opcional de **SML solo** y manifiesto con hashes. El perfil no incluye ni instala este vagón ni los binarios de SML. Verifica `SHA256SUMS.txt` de la release. No se distribuyen SDK, Engine, recursos del juego, logs privados, guardados, cuentas ni archivos PDB.

**Si falla el arranque:** conserva en privado el log de ese intento, cierra el juego y retira solo el vagón para volver a SML solo. Oculta identificadores, parámetros de autenticación/viaje y rutas personales antes de compartir evidencia. [Comunicar un problema](https://github.com/Tsavyh/satisfactory-passenger-railcar/issues/new?template=bug_report.yml).

## Qué está verificado y qué falta

| Capa | Alcance real / límites |
| :--- | :--- |
| Windows / Steam | Menú real 0.1.13 con DX11, prerrequisito nativo cargado y cero errores Streaming |
| Aceptación humana singleplayer | Función del HUD 0.1.11, aspecto mínimo 0.1.12 y ubicación ferroviaria 0.1.13 aceptados |
| Pruebas automáticas | 39 casos editor: 36 sin avisos + 3 con avisos SDK (134 entradas); 7 checks de paquete; MSVC desde el export: 26 precise + 26 fast; 2 contratos estructurales nativos |
| Multijugador remoto | El visto bueno de dos probadores corresponde a 0.1.8; el indicador 0.1.10 y la UI 0.1.11–0.1.13 siguen pendientes de una comprobación remota agrupada |
| Linux / Proton | Solo evidencia histórica de arranque 0.1.8; [informe del colaborador y límites](docs/PROTON-STARTUP.md). Sin nueva ejecución Proton, GCC/Clang ni CI alojada para 0.1.13 |
| Viaje con conductor humano / piloto automático | Aceptaciones separadas pendientes |
| Guardado/carga, muerte/desconexión y ciclo de vida nativo | Las regresiones editor no equivalen a aceptación de esos escenarios en el juego |
| Servidor dedicado / otras tiendas | Sin versión validada |

## Código propio MIT y entradas exactas

Código propio bajo **[MIT](LICENSE)**. El [snapshot v0.1.13](https://github.com/Tsavyh/satisfactory-passenger-railcar/tree/v0.1.13) y [`SOURCE-MANIFEST.json`](SOURCE-MANIFEST.json) identifican las **58 entradas revisadas del plugin** usadas para este binario, además de wrappers públicos y documentación heredados. No se promete recompilación idéntica bit a bit con otras rutas o herramientas.

- [Compilación y pruebas](BUILDING.es.md) · [Límites de terceros](NOTICES.md)
- [Colaboradores](CONTRIBUTORS.md): se conserva la [PR #1](https://github.com/Tsavyh/satisfactory-passenger-railcar/pull/1) de **MikelCalvo**, sus herramientas Linux, evidencia Proton acotada e historial original de integración.
- [Identidad y checklist público](docs/BRANDING.md)

`TsabsPassengerWagon` sigue siendo el identificador técnico de plugin, módulo y carpeta. SDK, Engine y recursos del juego se obtienen aparte; MIT no cambia sus licencias.

---
Proyecto comunitario no oficial. La portada es una ilustración original de marca, **no una captura del juego**.
