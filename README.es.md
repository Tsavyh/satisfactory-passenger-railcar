![Satisfactory Passenger Railcar — portada original del proyecto](assets/banner.png)

# Satisfactory Passenger Railcar

**Viaja en el tren. Deja que la locomotora se encargue de conducir.**  
Mod no oficial de vagón de pasajeros para **Satisfactory en Steam**, creado por **Tsabs**, con código abierto bajo **MIT**.

**[Descargar 0.1.10](https://github.com/Tsavyh/satisfactory-passenger-railcar/releases/download/v0.1.10/satisfactory-passenger-railcar-0.1.10-windows-steam.zip)** · [English](README.md) · [Compilar el código](BUILDING.es.md) · [Comunicar un problema](https://github.com/Tsavyh/satisfactory-passenger-railcar/issues/new/choose)

> **Versión preliminar.** Los tres puntos del indicador/aviso fueron aceptados en **singleplayer con 0.1.10**. **Falta comprobar este cambio en cliente remoto.** La base 0.1.8 tenía visto bueno multijugador de dos probadores y arranque Proton comprobado. Haz backup y usa una partida de pruebas.

## Qué ofrece

- Un vagón enganchable al tren con **una plaza técnica invisible por vagón** en esta fase.
- **Cámara exterior orbital**, sin controles para conducir la locomotora.
- **Sin inventario de carga, interior transitable ni sillas visibles.**
- Subida y bajada con velocidad absoluta **estrictamente inferior a 30 km/h**, también marcha atrás, siempre que se cumplan los controles de ocupación, autoridad y salida segura. A 30 km/h o más se bloquea el acceso; no se limita la velocidad de circulación del tren.

Durante las pruebas no tiene coste de construcción ni requisito de desbloqueo. El equilibrio final y la ampliación de plazas están pendientes.

## Novedades de la 0.1.10

La indicación de la tecla para bajar aparece **solo por debajo de 30 km/h**, también marcha atrás. A 30 o más se oculta. Si intentas salir demasiado rápido, aparece el aviso durante **3 segundos**; después vuelve a ocultarse si sigue rápido. Al bajar de 30 reaparece la tecla efectiva. Los tres puntos están aceptados en singleplayer. **Anfitrión y clientes deben actualizar.**

La PR #1 aporta pruebas para Linux y un informe acotado de Proton sin cambiar la política de ejecución.

## Instalación

**Entorno objetivo:** Satisfactory Steam **1.2.4.1 / CL 513885** y **SML 3.12.0**. Otras versiones y tiendas no están verificadas.

1. Instala SML mediante Satisfactory Mod Manager. Si ya arranca SML solo, conserva esa configuración.
2. Cierra el juego y descarga **[el ZIP instalable](https://github.com/Tsavyh/satisfactory-passenger-railcar/releases/download/v0.1.10/satisfactory-passenger-railcar-0.1.10-windows-steam.zip)**. Los archivos **Source code** de GitHub no sirven para instalar directamente.
3. Conserva la carpeta anterior del vagón **fuera** de `FactoryGame/Mods` y copia `VAGON/TsabsPassengerWagon` del ZIP a `FactoryGame/Mods`. No mezcles versiones ni reemplaces la carpeta de SML.
4. **Primera prueba: llegar únicamente al menú** y comprobar que aparece la 0.1.10. No abras todavía una partida personal. En Proton, mantén la versión y las opciones de arranque actuales durante esta comprobación.

La ruta final debe ser:

```text
FactoryGame/Mods/TsabsPassengerWagon/TsabsPassengerWagon.uplugin
```

El ZIP incluye instrucciones, licencia MIT, un perfil opcional de SML solo y un manifiesto; no incluye SML, SDK ni datos personales. El checksum está en `SHA256SUMS.txt` de la release.

## Qué está probado y qué falta

| Entorno o función | Estado |
| :--- | :--- |
| Arranque Windows / Steam | Menú0.1.10 verificado con DX11 y tres puntos del indicador/aviso aceptados en singleplayer |
| Linux / Steam / Proton | La 0.1.8 llegó al menú en el cliente afectado con Arch Linux / Proton 10.0-4; [informe y límites](docs/PROTON-STARTUP.md) |
| Multijugador con cliente remoto | Visto bueno humano de dos probadores en 0.1.8; falta comprobar el nuevo aviso de 0.1.10 |
| Viaje con otro jugador conduciendo y con piloto automático | Son pruebas separadas todavía pendientes |
| Servidor dedicado y otras tiendas | Sin versión validada |

La corrección 0.1.8 eliminó una cascada de **454 errores de carga** en el caso reproducido en Windows. El cliente Proton afectado también llegó después al menú con cero errores de carga; posteriormente ambos probadores dieron el visto bueno multijugador. No es una certificación exhaustiva de todas las plataformas o escenarios. [Cambios de versión](CHANGELOG.md).

Si vuelve a fallar, conserva el log de ese intento antes de arrancar otra vez y retira solo el vagón para recuperar SML solo. No publiques identificadores de cuentas, parámetros de autenticación ni rutas personales. Puedes usar el [formulario de problemas](https://github.com/Tsavyh/satisfactory-passenger-railcar/issues/new?template=bug_report.yml).

## Código abierto

Fuentes propias bajo **[MIT](LICENSE)**, con [instrucciones de compilación](BUILDING.es.md), pruebas y [límites de recursos de terceros](NOTICES.md). El [tag v0.1.10](https://github.com/Tsavyh/satisfactory-passenger-railcar/tree/v0.1.10) conserva las fuentes del binario publicado. Mantiene el aviso temporal introducido en 0.1.9 y la [PR #1](https://github.com/Tsavyh/satisfactory-passenger-railcar/pull/1) de [MikelCalvo](https://github.com/MikelCalvo). [Colaboradores](CONTRIBUTORS.md).

El identificador interno **`TsabsPassengerWagon`** se mantiene por compatibilidad. El nombre público es **Satisfactory Passenger Railcar**. El SDK, Engine y recursos del juego se obtienen por separado.

---

Proyecto comunitario no oficial. La portada es una ilustración original de marca, **no una captura del juego**.
