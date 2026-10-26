# Verificación offline del ZIP Windows Steam

[English](RELEASE-VERIFICATION.md) · [Compilación](../BUILDING.es.md)

Requiere **Python 3.12**, solo su biblioteca estándar. Ejecuta desde la raíz del
repositorio; no necesita SDK, compilador, juego, gestor de mods, red ni paquetes
Python. Lee el ZIP sin extraer, instalar ni ejecutar su contenido. No cambia
configuración, instalación ni partidas del juego.

```sh
python -B tools/verify_release.py satisfactory-passenger-railcar-0.1.13-windows-steam.zip --expected-version 0.1.13
```

Para contrastar además el ZIP completo con una suma obtenida **por separado de
una fuente fiable**, añade `--sha256` seguido de los 64 caracteres hexadecimales
en minúscula esperados. Para el contenedor público 0.1.13 observado:

```sh
python -B tools/verify_release.py satisfactory-passenger-railcar-0.1.13-windows-steam.zip --expected-version 0.1.13 --sha256 49f1785edb02b609b6e595087ed128bf1156905bf134ab72d65cc658bd2f8a73
```

Sirve también en Windows CMD seleccionando Python 3.12 como `python`. En Linux
puedes usar `python3.12`. Salida **0** indica que pasan todas las comprobaciones
implementadas; un rechazo devuelve **1** y argumentos incorrectos **2**, siempre
con mensaje fijo saneado, sin repetir rutas, nombres ni valores del ZIP.
`--help` devuelve 0 solo para mostrar ayuda, sin verificar un paquete. Las
protecciones siguen activas con Python `-O`.

## Contenedor aceptado

Solo se admite `satisfactory-passenger-railcar-VERSION-windows-steam.zip`, no el
nombre antiguo `TsabsPassengerWagon-…-Steam-Proton-Publico.zip`.
`--expected-version` es obligatorio: versión numérica `x.y.z`, sin prefijos,
sufijos, espacios ni ceros iniciales salvo un componente igual a cero. Debe
coincidir con `VersionName`/`SemVersion` del descriptor y `version` del manifiesto.
No se compara con el descriptor cambiante del repositorio.

Archivos regulares obligatorios y no vacíos:

- En raíz: `MANIFEST.json`, `LEEME.txt`, `LICENSE`.
- `VAGON/TsabsPassengerWagon/TsabsPassengerWagon.uplugin`.
- En `VAGON/TsabsPassengerWagon/Binaries/Win64/`:
  `FactoryGameSteam-TsabsPassengerWagon-Win64-Shipping.dll` y
  `FactoryGameSteam-Win64-Shipping.modules`.
- En `VAGON/TsabsPassengerWagon/Content/Paks/Windows/`:
  `TsabsPassengerWagonFactoryGame-Windows.pak`, `.ucas` y `.utoc`.

Se permite un `Solo-SML-x.y.z.smmprofile` opcional solo con la estructura conocida
exclusiva de SML: nombre de perfil `TsabsRailTest-SML312`, SML habilitado con
versión `=x.y.z` igual al nombre del archivo, destino requerido `Windows`,
lockfile con versión entera 1 y sin mods bloqueados, y metadatos que contienen
únicamente gameVersion entero 513885. Se rechazan campos, mods, ajustes o rutas
adicionales. Admitir otro esquema exige revisar y actualizar el verificador.

Se rechaza cualquier otro archivo, incluidos binarios del SDK/SML. Los
directorios explícitos son opcionales y solo pueden ser antecesores reales de
archivos permitidos. Solo se admite almacenamiento sin compresión o deflate.
Se rechazan rutas inseguras/no portables, duplicados exactos o sin distinguir
mayúsculas, enlaces simbólicos, tipos especiales y entradas cifradas. La política
es específica del host ZIP, no una validación general de conformidad: admite
DOS (0), Unix (3) y NTFS (10). DOS/Unix admiten un byte bajo compatible con DOS
(solo lectura/oculto/sistema/directorio/archivo/normal) y tipo Unix en la palabra
alta sin especificar, regular o directorio coherente, incluido el modo 0600 que
Python escribe con host DOS en Windows. Los bits 8–15 no se admiten en esos hosts.
El formato NTFS admitido usa únicamente atributos Windows nativos: los básicos
más temporal, comprimido y no indexado; rechaza todos los demás bits, incluida
una palabra alta no nula, sin interpretarla como modo POSIX. Atributos cero son
válidos. La marca directorio exige barra final; se admite barra con atributos
sin especificar. NORMAL debe ir solo y no puede marcar un directorio.
Reparse, dispositivo reservado, volumen, sparse/offline, EFS y otros formatos
no soportados se rechazan antes de abrir contenido. EFS es metadato del sistema
de archivos, **no una elusión del cifrado ZIP**; no cambia el rechazo separado
de las flags de cifrado ZIP. Véanse [PKWARE APPNOTE, secciones 4.4.2 y
4.4.15](https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT),
[atributos Microsoft](https://learn.microsoft.com/windows/win32/fileio/file-attribute-constants)
y [ZipInfo de Python](https://docs.python.org/3.14/library/zipfile.html#zipinfo-objects).
Un host desconocido o metadatos válidos pero fuera del subconjunto admitido se
rechazan; no se certifica que sean peligrosos para cualquier extractor.

Límites antes de descomprimir contenido: ZIP 128 MiB, archivo 128 MiB, total sin
comprimir declarado 256 MiB y 128 entradas. JSON tiene límite de 1 MiB; se
rechazan claves duplicadas, codificación inválida, constantes no estándar y
sintaxis incorrecta. Los hashes se calculan por bloques acotados y el parser PE
usa lecturas/mapeos RVA acotados sin cargar ni ejecutar una DLL. El parser PE
limita también a 512 MiB el trabajo estimado de descompresión, incluidas las
relecturas por desplazamientos hacia atrás; no es un plazo de tiempo real.

## Qué demuestra un resultado correcto y qué no

- El mapa del manifiesto cubre exactamente todos los archivos regulares salvo
  el propio manifiesto, con SHA-256 en minúscula que coinciden con sus bytes.
  **Es coherencia interna, no autenticidad independiente**: quien sustituya el
  contenido puede regenerar el manifiesto. La suma externa opcional vincula los
  bytes del ZIP solo a esa suma esperada obtenida de una fuente fiable aparte.
- El descriptor identifica el módulo runtime `TsabsPassengerWagon` y una
  dependencia SML habilitada y no opcional. `.modules` usa BuildId `SML` y
  resuelve únicamente ese módulo a la DLL Shipping incluida exacta.
- La DLL presenta cabeceras x64 PE32+ de DLL legibles y acotadas, y su tabla
  normal de imports conserva
  `FactoryGameSteam-FactoryDedicatedClient-Win64-Shipping.dll`. No basta una
  cadena fuera de la tabla. Se revisan límites de secciones/RVA, descriptores,
  nombres y arrays de thunks. No es un validador completo del formato PE ni del
  cargador Windows; no demuestra que el juego instalado proporcione la DLL
  correcta ni que llegue a cargarse.

El resultado es **solo verificación de paquete**: no certifica jugabilidad,
multijugador, compatibilidad con juego/Engine, arranque real, ausencia de malware
ni detección arbitraria de datos privados/DLP. La lista exacta de archivos no
analiza su contenido en busca de datos privados. Los booleanos históricos del
manifiesto no cuentan como pruebas runtime. Se calculan hashes del contenido
cocinado sin interpretarlo ni validarlo con Unreal. Los formatos futuros pueden
ser rechazados hasta una revisión y actualización explícita.

## Pruebas offline

```sh
python3.12 -B -m unittest discover -s Tests -p 'test_*.py' -v
```

En Windows selecciona solo la suite portable del verificador (las pruebas del
ejecutor Bash incluidas en el descubrimiento completo son exclusivas de Linux):

```bat
python -B -m unittest discover -s Tests -p test_release_verifier.py -v
```

Las pruebas generan fixtures
pequeñas de PE/imports/ZIP **sintéticas**, incluidas mutaciones adversarias; no
recompilan ni distribuyen un binario del mod. Las pruebas de políticas existentes
también requieren su entorno Bash/C++ documentado en Linux. El colaborador probó
el verificador original en Linux/Python 3.12 con el ZIP público 0.1.9. Esta
corrección de metadatos se probó en Windows 10/Python 3.14.7, normal y con `-O`:
25 pruebas correctas, conservando las 21 originales, y el ZIP público real 0.1.13
correcto con la suma anterior. No se atribuye una nueva ejecución Linux ni CI alojada.
