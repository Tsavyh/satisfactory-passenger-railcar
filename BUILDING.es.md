# Compilar y verificar

[English](BUILDING.md) · [Resumen del proyecto](README.es.md) · [Notas EN/ES](RELEASE-NOTES.md)

Este repositorio contiene **el plugin**, no un SDK completo de Satisfactory. El identificador interno sigue siendo `TsabsPassengerWagon`. Estos comandos son instrucciones para colaboradores: actualizar la documentación no los ejecuta ni acredita nuevos resultados CI.

## Entorno registrado para 0.1.15

| Componente | Entorno fijado |
| :--- | :--- |
| Juego | Satisfactory Steam 1.2.4.1, CL 513885 |
| Cargador | SML 3.12.0 |
| Starter Project | satisfactorymodding/SatisfactoryModLoader, commit `8eebc9e659f0b2ff4392d2f8cf6e2cff60c8ea49` |
| Engine | Unreal Engine 5.6.1-css-83 de Coffee Stain |
| Compilador | Visual Studio 2022, MSVC 14.38 |
| Windows SDK | 10.0.26100.0 |
| Wwise | 2025.1.6.9117; integración 2025.1.6.9117.4248 |

Obtén y prepara las dependencias por separado siguiendo la [documentación de Satisfactory Modding](https://docs.ficsit.app/). No se redistribuyen ni cambian de licencia aquí. No se ha validado la compilación del plugin Unreal en un host Linux; las pruebas standalone Linux no acreditan ese soporte.

## Identidad de las fuentes

El destino de publicación es **v0.1.15**, con **66 entradas exactas revisadas del plugin** registradas en [`SOURCE-MANIFEST.json`](SOURCE-MANIFEST.json). Las herramientas públicas y la documentación son distintas de esas entradas. La publicación final debe incluir tag, manifiesto de fuentes y artefacto de ejecución correspondientes; una etiqueta de versión no acredita procedencia por sí sola. Correspondencia de fuentes **no significa compilación reproducible bit a bit**.

El código propio es **[MIT](LICENSE)**. Se conservan [las PR #1–#5 de MikelCalvo y su historial original](CONTRIBUTORS.md). SDK, Engine y recursos del juego quedan fuera de la concesión MIT; consulta [NOTICES.md](NOTICES.md). No incluyas diagnósticos privados, identificadores de cuentas ni rutas personales en documentación o artefactos públicos.

## Colocar el plugin y empaquetar Unreal

En un Starter Project separado y preparado, coloca este repositorio en `Mods/TsabsPassengerWagon`, de modo que exista `Mods/TsabsPassengerWagon/TsabsPassengerWagon.uplugin`. No anides otra carpeta del mismo nombre ni reutilices Binaries/Intermediate de otra versión.

Abre Windows CMD en la raíz del Starter Project y define `UE_ROOT` con la ubicación real de tu Engine CSS:

```bat
set "UE_ROOT=C:\Ruta\Al\UnrealEngine"
call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" FactoryEditor Win64 Development "%CD%\FactoryGame.uproject" -WaitMutex -NoHotReloadFromIDE
call "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" -ScriptsForProject="%CD%\FactoryGame.uproject" PackagePlugin -project="%CD%\FactoryGame.uproject" -DLCName=TsabsPassengerWagon -Target=FactoryGameSteam -platform=Win64 -clientconfig=Shipping -build -installed -nocompileeditor -utf8output -verbose -timestamps -nop4 -UbtArgs="-MaxParallelActions=4 -NoUBA -NoXGE -NoFASTBuild"
```

Son los objetivos y argumentos del flujo registrado de compilación de release, con rutas parametrizadas. Compila el módulo Editor antes de usar `-nocompileeditor`. Revisa `Saved/ArchivedPlugins/TsabsPassengerWagon` antes de instalar. Mantén desactivado el despliegue automático. Estas instrucciones no autorizan instalar, arrancar el juego ni cambiar partidas/configuración.

## Generar y verificar el contenedor público

Tras compilar/cocinar, usa [el empaquetador](docs/RELEASE-PACKAGING.es.md) para crear ZIP y `SHA256SUMS.txt` en un **directorio nuevo fuera de los árboles de entrada y fuentes**:

```sh
python -B tools/package_release.py "/ruta/al/plugin-cocinado" --output "/ruta/al/paquete-nuevo"
python -B tools/verify_release.py satisfactory-passenger-railcar-0.1.15-windows-steam.zip --expected-version 0.1.15
```

El primer comando lee la **raíz del plugin cocinado**, no este checkout de fuentes. El ejemplo del verificador supone que el ZIP está en el directorio actual; en otro caso indica su ruta real de salida. Python 3.12 es la base CI; las herramientas también se han ejercitado en Windows con Python 3.14.7. Consulta [verificación de paquetes](docs/RELEASE-VERIFICATION.es.md) para SHA-256 externa opcional y restricciones de estructura.

El contenedor público 0.1.15 tiene **nueve miembros**: seis archivos de ejecución bajo `VAGON/TsabsPassengerWagon/`, más `LEEME.txt`, `LICENSE` y `MANIFEST.json`. **Sin PDB, perfil SML opcional, SDK, Engine/recursos del juego, logs privados ni guardados.** SML se instala aparte. El empaquetador no compila/cocina Unreal, instala o ejecuta payloads, ni publica la release. Verificar el paquete no certifica carga del juego, compatibilidad, ausencia de malware ni de datos privados.

## Pruebas sin Unreal

Desde una consola x64 Native Tools de MSVC, en la raíz del plugin:

```bat
call Tests\run-policy-tests.cmd
python -B -m unittest discover -s Tests -p test_native_prerequisite.py -v
```

El ejecutor compila y ejecuta 26 casos con `/fp:precise` y de nuevo con `/fp:fast`: frontera estricta de 30 km/h, reversa, NaN e infinitos. Los contratos Python verifican la dependencia explícita del módulo nativo y la referencia retenida a su clase.

En Linux, con Bash y GCC/Clang con C++20:

```sh
CXX=clang++ bash Tests/run-policy-tests.sh
CXX=g++ bash Tests/run-policy-tests.sh
CXX=clang++ python3 -B -m unittest discover -s Tests -p 'test_*.py' -v
```

El ejecutor Bash usa salidas temporales y los mismos casos en `-fno-fast-math` y `-ffast-math`. `CXX` selecciona un ejecutable de compilador (por defecto `c++`); usa el mismo en las pruebas Python del ejecutor. Las entradas IEEE-754 fijas evitan que Clang transforme la generación aritmética/de infinitos bajo fast-math. Son comprobaciones de políticas/ejecutor en el host, no una compilación Unreal.

Las suites portables de paquetes también pueden ejecutarse por separado, incluida la optimización:

```sh
python -B -m unittest discover -s Tests -p 'test_release_*.py' -v
python -B -O -m unittest discover -s Tests -p 'test_release_*.py' -v
```

Las fixtures sintéticas del verificador/empaquetador prueban las herramientas; no son binarios recompilados del mod.

## Qué puede comprobar GitHub automáticamente

Los workflows existentes están definidos para push, pull request y ejecución manual `workflow_dispatch`, sin juego/SDK propietario ni secretos del repositorio:

- [Standalone checks](.github/workflows/standalone-checks.yml): Ubuntu 24.04 con GCC y Clang, políticas precise/fast-math, suites Python descubiertas y ShellCheck; Windows Server 2022 con políticas MSVC precise/fast y contratos Python del prerrequisito nativo.
- [Release package verification](.github/workflows/package-verification.yml): suites de verificador y empaquetador en Ubuntu y Windows con Python 3.12, tanto normal como con `-O`.
- El job de referencia autentica el **ZIP público fijo 0.1.9**, reempaqueta sus seis payloads precompilados, verifica paquete/checksum generado y compara hashes de payloads. No compila 0.1.15 ni ejecuta el juego; un roundtrip correcto de 0.1.9 no certifica el binario 0.1.15.
- La nueva CI standalone de enrutamiento Windows está prevista para **176 casos: 150 de reenrailado + 23 de disponibilidad runtime + 3 de disponibilidad en rama Editor**. Una frontera padre simulada o una rama Editor **no es ejecución de Unreal Editor**, entrega RPC real, física, reenrailado nativo ni validación real del ciclo de vida de detours SML. Hay que comprobar aparte sus ejecuciones alojadas; aquí no se atribuye un nuevo éxito de routing CI.

Los workflows consultados usan permisos de repositorio de solo lectura, no conservan credenciales Git y fijan acciones a hashes de commit. Muestran versiones de herramientas y acotan los jobs; MSVC alojado no queda fijado al compilador de release. Una definición de workflow no demuestra que una ejecución concreta haya pasado. El estado CI debe apoyarse en la ejecución real y su revisión seleccionada, no en una afirmación de la IA.

La CI alojada sin SDK/juego con licencia **no compila Editor, cocina/empaqueta el plugin Unreal, arranca Satisfactory/Proton ni valida jugabilidad humana**. Estos workflows no publican ni despliegan. Mantén separadas las compilaciones propietarias y las pruebas humanas, sin prometer que GitHub las hará.

## Automatización Unreal y evidencia registrada 0.1.15

Con SDK preparado aparte y módulo Editor recién compilado:

```bat
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%CD%\FactoryGame.uproject" -unattended -NullRHI -NoSound -nosplash -ExecCmds="Automation RunTests Tsabs.Passenger" -TestExit="Automation Test Queue Empty" -ReportExportPath="%CD%\Saved\PassengerTestReport"
```

Comprueba los resultados hoja exportados y contadores failed/not-run, no solo la salida del proceso. La ejecución registrada 0.1.15 tiene **43 casos correctos: 40 éxitos ordinarios + 3 con avisos SDK, 134 entradas de aviso**. Compilación Editor y UAT/Shipping/cook terminaron con salida **0**; el paquete Shipping nuevo llegó al menú real Windows/Steam con **cero errores Streaming**. Son registros de preparación de release, no nuevas ejecuciones de esta actualización documental ni de CI alojada.

La cobertura heredada incluye forwarding/ocultación local de mira, composición del velocímetro mínimo nativo, propiedad/reconstrucción de widgets retenidos, categoría/orden de menú y regresiones de ciclo de vida. Las fixtures de reenrailado/disponibilidad deben conservar sus límites explícitos: sentinelas padre y stubs SDK no prueban comportamiento nativo runtime. Invocar hooks o `NativeTick` directamente no acredita reconstrucción Slate completa, posesión real, entrega RPC ni menú renderizado.

La aceptación humana es independiente: se aceptó el reenrailado real 0.1.14 y el bloque singleplayer 0.1.15 de nombre, descripción, coste (**5 armazones modulares normales + 10 tuberías de acero + 10 vigas de acero**), desbloqueo/ubicación como el vagón de carga nativo y construcción. La disponibilidad sigue **Tier 6 — Tecnología de tren monorraíl** (`MonorailTechnology`); el prototipo gratuito/sin desbloqueo ya no está vigente. Se mantienen una plaza y los controles de autoridad/descarrilamiento.

Se conservan la aceptación del HUD 0.1.13 y la **aprobación humana general SP/MP de 0.1.13**. Esta última es un informe del usuario, no una prueba independiente del agente ni certificación de plataformas, roles de jugadores o escenarios concretos. La excepción de reenrailado detectada después se corrigió y aceptó en 0.1.14. El informe Proton acotado del colaborador sigue siendo evidencia histórica **0.1.8**. No se acredita nueva aceptación 0.1.15 multijugador, Proton o servidor dedicado, viaje con conductor humano/piloto automático, guardado/carga real, muerte/desconexión ni restauración del pasajero guardado. Anfitrión y clientes deben usar la misma versión. El desarrollo adicional de jugabilidad, incluida 0.1.16, sigue pausado.
