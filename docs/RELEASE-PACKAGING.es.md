# Generar un paquete verificado

[English](RELEASE-PACKAGING.md) · [Compilación](../BUILDING.es.md)

Requiere **Python 3.12**, solo biblioteca estándar. Compila/cocina primero el
plugin Windows Steam por separado. `PLUGIN_DIR` es su **raíz cocinada y
descomprimida**, con `TsabsPassengerWagon.uplugin`, `Binaries/Win64/` y
`Content/Paks/Windows/`; no es este repositorio fuente ni la carpeta superior.

Desde la raíz del repositorio, entrecomilla las rutas con espacios:

```sh
python -B tools/package_release.py "/ruta/al/plugin cocinado" --output "/ruta/a/paquete-nuevo"
python -B tools/package_release.py --help
```

En Windows CMD sustituye las rutas y selecciona Python 3.12 como `python`.
En Linux puede ser `python3.12`. Con la ruta absoluta al script también funciona
desde otro directorio. API Python: `build_release(plugin_dir, output)` de
`tools/package_release.py` devuelve el `pathlib.Path` del ZIP o lanza
`PackagingError` con mensaje fijo. Importar el módulo no genera un paquete.

## Entrada y salida

La versión se obtiene de `VersionName` y `SemVersion` coincidentes: `x.y.z`
numérica estricta sin ceros iniciales. No se modifica ninguna entrada. Solo se
leen estos seis archivos regulares, no vacíos, de la raíz cocinada:

- `TsabsPassengerWagon.uplugin`.
- `Binaries/Win64/FactoryGameSteam-TsabsPassengerWagon-Win64-Shipping.dll`.
- `Binaries/Win64/FactoryGameSteam-Win64-Shipping.modules`.
- `Content/Paks/Windows/TsabsPassengerWagonFactoryGame-Windows.pak`, `.ucas`, `.utoc`.

Se ignoran otros archivos: PDB, fuentes, logs y configuración no se buscan ni
copian. Las entradas seleccionadas y sus antecesores no pueden ser enlaces
simbólicos ni puntos de reanálisis/junctions de Windows. Se rechazan entradas
ausentes, vacías, especiales o demasiado grandes. Se incluye `LICENSE` del
repositorio, archivo regular pequeño, sin cambiar sus bytes.

`--output` debe ser un **directorio nuevo fuera de los árboles de entrada y
fuentes**, con padre real ya existente. Se rechaza sin cambiarlo cualquier
archivo, directorio (incluso vacío), enlace o enlace roto preexistente. No uses
componentes `..`; utiliza una ruta absoluta si hace falta. Solo se generan:

- `satisfactory-passenger-railcar-VERSION-windows-steam.zip`.
- `SHA256SUMS.txt`: SHA-256 en minúsculas, dos espacios, nombre del ZIP y salto de línea.

Dentro del ZIP hay `LEEME.txt`, `LICENSE`, `MANIFEST.json` en la raíz y los seis
archivos del plugin, idénticos, en `VAGON/TsabsPassengerWagon/`. Las instrucciones
bilingües usan los requisitos del descriptor; el manifiesto calcula hashes de
todo salvo sí mismo. No añade SML, perfiles personales, afirmaciones históricas
de pruebas, procedencia de fuentes/tags ni publicación automática.

El ZIP temporal terminado y su SHA-256 deben superar el **[verificador real
existente](RELEASE-VERIFICATION.es.md)** antes de crear la salida. Código 0:
paquete comprobado; fallo: 1; argumentos incorrectos: 2, con mensajes fijos
sin rutas/datos privados. `--help` devuelve 0 sin empaquetar. Las protecciones
siguen activas con `-O`.

## Límites y comprobaciones

ZIP/archivo: 128 MiB; total descomprimido: 256 MiB; JSON: 1 MiB; licencia:
1 MiB. Copia y hashes por bloques limitados; detecta cambios ordinarios de la
entrada durante el proceso: mantenla estable. No protege frente a un proceso
privilegiado que sustituya archivos. Entradas idénticas dan bytes ZIP idénticos
con la **misma versión de Python/zlib** (orden, fecha, permisos y JSON fijos);
no se promete igualdad de compresión entre toolchains.

La preparación temporal se hace bajo el padre de salida. La publicación crea
archivos exclusivamente y limpia solo lo propio tras fallos ordinarios de
copia/escritura. **No es atómica frente a interrupciones o cortes eléctricos**:
puede quedar una salida incompleta. Su presencia no prueba éxito; comprueba
el código de salida y verifica ZIP/checksum por separado. Nunca sustituye
publicaciones anteriores. No instala SML, toca juego/partidas, ejecuta binarios,
compila Unreal ni demuestra arranque, juego o compatibilidad multijugador.

Pruebas portables del empaquetador (entradas sintéticas, no un mod recompilado):

```sh
python -B -m unittest discover -s Tests -p test_release_packager.py -v
python -B -O -m unittest discover -s Tests -p test_release_packager.py -v
```

El [workflow de paquetes](../.github/workflows/package-verification.yml) ejecuta
estas pruebas junto al verificador en Ubuntu 24.04 y Windows Server 2022, con
Python 3.12 y optimización 0 y 1. El job de referencia autentica y verifica primero
el ZIP público fijo 0.1.9, y vuelve a empaquetar solo sus seis archivos del plugin
ya compilados. Comprueba el ZIP/checksum generado y los hashes idénticos de esos
seis archivos. Los paquetes temporales se eliminan, no se suben ni publican.
Esto prueba empaquetado, no compilación Unreal ni arranque del juego.
