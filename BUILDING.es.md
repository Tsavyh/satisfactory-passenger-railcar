# Compilar y verificar

## Entorno probado

- Satisfactory Steam 1.2.4.1, CL 513885; SML 3.12.0.
- Starter Project de satisfactorymodding/SatisfactoryModLoader en el commit 8eebc9e659f0b2ff4392d2f8cf6e2cff60c8ea49.
- Unreal Engine 5.6.1-css-83 de Coffee Stain.
- Visual Studio 2022, MSVC 14.38 (v14.38-17.8), Windows SDK 10.0.26100.0.
- Wwise 2025.1.6.9117, integración 2025.1.6.9117.4248.

Obtén/prepara ese entorno mediante la documentación oficial de Satisfactory Modding (https://docs.ficsit.app/). Aquí solo se publica el plugin propio, no esas herramientas ni el SDK. No se ha probado este proceso de compilación en un host Linux.

## Colocar el plugin

En un Starter Project separado y preparado, coloca este repositorio en `Mods/TsabsPassengerWagon`, de modo que exista `Mods/TsabsPassengerWagon/TsabsPassengerWagon.uplugin`. No anides otra carpeta del mismo nombre. No copies Binaries/Intermediate de otra versión.

Para los comandos siguientes abre una consola Windows CMD en la raíz del Starter Project y define `UE_ROOT` con la ubicación real de tu Engine CSS. No cambies el perfil/configuración personal del juego para compilar.

```bat
set "UE_ROOT=C:\Ruta\Al\UnrealEngine"
call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" FactoryEditor Win64 Development "%CD%\FactoryGame.uproject" -WaitMutex -NoHotReloadFromIDE
call "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" -ScriptsForProject="%CD%\FactoryGame.uproject" PackagePlugin -project="%CD%\FactoryGame.uproject" -DLCName=TsabsPassengerWagon -Target=FactoryGameSteam -platform=Win64 -clientconfig=Shipping -build -installed -nocompileeditor -utf8output -verbose -timestamps -nop4 -UbtArgs="-MaxParallelActions=4 -NoUBA -NoXGE -NoFASTBuild"
```

Es el mismo objetivo/flujo UAT usado para el paquete publicado, con rutas parametrizadas. Alpakit también puede empaquetar el plugin para Windows/Steam. No actives despliegue automático al juego; revisa el ZIP generado en `Saved/ArchivedPlugins/TsabsPassengerWagon` antes de instalar. No se promete igualdad bit a bit entre compilaciones realizadas con toolchains/rutas diferentes.

## Verificación offline del ZIP

Con Python 3.12 (también probado en Windows con Python 3.14.7), comprueba un
contenedor público Windows Steam sin extraer, instalar ni ejecutar su contenido:

```sh
python -B tools/verify_release.py satisfactory-passenger-railcar-0.1.13-windows-steam.zip --expected-version 0.1.13
```

Consulta [verificación de paquetes](docs/RELEASE-VERIFICATION.es.md) para la suma
SHA-256 externa opcional, estructura exacta, pruebas y límites. No certifica
carga del juego, compatibilidad, ausencia de malware ni de datos privados.

## Pruebas sin Engine

Desde una consola x64 Native Tools de MSVC, en la raíz del plugin:

```bat
call Tests\run-policy-tests.cmd
python Tests\test_native_prerequisite.py
```

El primer script compila y ejecuta 26 casos en `/fp:precise` y en `/fp:fast`, incluidos frontera 30, reversa, NaN e infinitos. El segundo verifica la dependencia explícita y la referencia a la clase nativa que conserva el import de la DLL. No requieren SDK para esas dos comprobaciones.

En Linux, con Bash y un compilador GCC/Clang con C++20:

```sh
CXX=clang++ bash Tests/run-policy-tests.sh
CXX=g++ bash Tests/run-policy-tests.sh
python3 -B -m unittest discover -s Tests -p 'test_*.py' -v
```

El ejecutor Bash usa archivos temporales y ejecuta los mismos 26 casos en
`-fno-fast-math` y `-ffast-math`. `CXX` selecciona un ejecutable de compilador
(por defecto, `c++`); conserva ese mismo `CXX` en las pruebas Python si quieres
probar el ejecutor con ese compilador. Las entradas especiales usan bits
IEEE-754 fijos para que Clang no transforme su generación con fast-math.
Son pruebas de políticas y del ejecutor en el host, no una compilación del
plugin Unreal.

## Pruebas en editor y límites

```bat
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%CD%\FactoryGame.uproject" -unattended -NullRHI -NoSound -nosplash -ExecCmds="Automation RunTests Tsabs.Passenger" -TestExit="Automation Test Queue Empty" -ReportExportPath="%CD%\Saved\PassengerTestReport"
```

## Verificación 0.1.13 y cobertura heredada

39 casos editor correctos: **36 sin avisos y 3 con avisos SDK**, **134 entradas de aviso**, cero fallos y cero no ejecutados. Siete comprobaciones de aceptación del paquete correctas. Compilación editor y Shipping/cook terminaron con salida 0; el menú real Windows DX11 cargó el prerrequisito nativo con cero errores Streaming. Esta preparación repite únicamente pruebas independientes desde el export: **26 `/fp:precise` + 26 `/fp:fast` con MSVC 14.38** y **2 contratos Python del prerrequisito nativo**. No se atribuyen nuevas ejecuciones GCC/Clang, Unreal en host Linux, Proton ni CI alojada.

Se incluyen las regresiones de mira local y forwarding, composición del panel nativo, reconstrucción/propiedad de widgets retenidos, categoría/orden CDO del menú y ciclo de vida heredado posterior a 0.1.10: limpieza de reservas tras invalidación síncrona de UObject, destrucción por el motor de una fixture vacía inicializada, rechazo seguro con anclaje destruido, restauración idempotente, retirada de padre UMG y HUD nuevo al volver a entrar.

Invocar hooks o `NativeTick` en fixtures no demuestra reconstrucción Slate completa, posesión nativa real, entrega RPC ni menú renderizado. La aceptación humana es separada: función HUD 0.1.11, aspecto mínimo 0.1.12 y ubicación Transporte/Ferrocarril 0.1.13 aceptados en singleplayer. No acredita muerte/desconexión real, serialización de guardados, viaje con conductor humano/piloto automático ni nuevo ciclo de vida en cliente remoto.

El visto bueno humano multijugador de dos probadores y el informe Proton acotado del colaborador corresponden a 0.1.8. El indicador de salida 0.1.10 y la UI de pasajero 0.1.11–0.1.13 requieren una prueba remota agrupada. Anfitrión y clientes deben usar la misma versión. Se conservan los comandos del colaborador sin certificarlos de nuevo con otros compiladores. Este export no añade workflows.

[`SOURCE-MANIFEST.json`](SOURCE-MANIFEST.json) registra las 58 entradas exactas revisadas y el checksum del binario público reducido. Se conservan wrappers, MIT, límites de terceros, crédito de PR #1 e historial actual de main. No se promete recompilación idéntica bit a bit. Los diagnósticos privados quedan fuera del repositorio.
