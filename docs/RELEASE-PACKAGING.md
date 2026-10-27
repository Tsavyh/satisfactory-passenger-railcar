# Generate a verified release package

[Español](RELEASE-PACKAGING.es.md) · [Build instructions](../BUILDING.md)

Requires **Python 3.12**, standard library only. First build/cook the Windows
Steam plugin separately. `PLUGIN_DIR` is its **unpacked cooked root**, containing
`TsabsPassengerWagon.uplugin`, `Binaries/Win64/` and `Content/Paks/Windows/`;
it is not this source checkout or the parent of the plugin directory.

From the repository root (quote paths containing spaces):

```sh
python -B tools/package_release.py "/path/to/cooked TsabsPassengerWagon" --output "/path/to/railcar-release-new"
python -B tools/package_release.py --help
```

On Windows CMD, use your actual cooked plugin path and Python 3.12 as `python`.
On Linux, use `python3.12` if needed. An absolute script path also works from an
unrelated working directory. Python API: `build_release(plugin_dir, output)` in
`tools/package_release.py` returns the generated ZIP `pathlib.Path`, or raises a
fixed-message `PackagingError`. Importing the module does not build anything.

## Input and output

The version comes from matching descriptor `VersionName` and `SemVersion`,
strict numeric `x.y.z` with no leading zeroes. Inputs are never modified. Only
these six nonempty regular files are read from the cooked root:

- `TsabsPassengerWagon.uplugin`.
- `Binaries/Win64/FactoryGameSteam-TsabsPassengerWagon-Win64-Shipping.dll`.
- `Binaries/Win64/FactoryGameSteam-Win64-Shipping.modules`.
- `Content/Paks/Windows/TsabsPassengerWagonFactoryGame-Windows.pak`, `.ucas`, `.utoc`.

Unrelated files, including PDBs, source, logs and settings, are ignored, not
searched or copied. Selected files and their ancestors must not be symlinks or
Windows reparse points/junctions. Missing, empty, special or oversized inputs
are refused. The repository's small regular `LICENSE` is included byte-for-byte.

`--output` must name a **new directory outside the input and source trees**.
Its parent must already be a real directory. Existing files, directories
(including empty ones), symlinks and dangling links are refused unchanged.
Do not use paths containing `..`; choose an absolute output path if needed.
The directory receives exactly:

- `satisfactory-passenger-railcar-VERSION-windows-steam.zip`.
- `SHA256SUMS.txt`: lowercase SHA-256, two spaces, ZIP basename and newline.

The ZIP contains root `LEEME.txt`, `LICENSE`, `MANIFEST.json` and those six
byte-identical plugin files under `VAGON/TsabsPassengerWagon/`. The generated
bilingual instructions use descriptor requirements; the manifest hashes every
member except itself. No SML binary, personal profile, historical test claims,
source/tag provenance or automatic release/publication is added.

The completed staged ZIP and its computed SHA-256 must pass the **existing
[real verifier](RELEASE-VERIFICATION.md)** before output is created. Exit 0
reports package-only success; failures return 1, argument errors 2, with fixed
sanitized messages. `--help` returns 0 without building. Guards remain active
under `-O`.

## Limits and checks

ZIP/member limits are 128 MiB, aggregate uncompressed members 256 MiB, JSON
1 MiB; the license is also capped at 1 MiB. Copying/hashing uses bounded chunks
and detects ordinary cooperative source changes; keep the cooked input stable.
This is not protection against a privileged process replacing files mid-run.
Identical inputs produce identical ZIP bytes on the **same Python/zlib
toolchain** (sorted names, fixed timestamps/modes, deterministic JSON); compressed
byte equivalence across toolchains is not promised.

Staging is temporary under the output parent. Publication uses exclusive
creation and cleans only owned output after ordinary write/copy failures.
It is **not crash/power-loss atomic**: an interrupted run can leave incomplete
output. Never treat its presence alone as success; inspect the command exit
and independently verify the ZIP/checksum. Old publication directories are
never replaced. Nothing installs SML, touches game files/saves, runs payloads,
compiles Unreal, or proves startup, gameplay or multiplayer compatibility.

Portable packager tests (synthetic inputs, not a rebuilt mod):

```sh
python -B -m unittest discover -s Tests -p test_release_packager.py -v
python -B -O -m unittest discover -s Tests -p test_release_packager.py -v
```

The [package workflow](../.github/workflows/package-verification.yml) runs these
alongside the verifier suite on Ubuntu 24.04 and Windows Server 2022, with Python
3.12 at optimization levels 0 and 1. The reference-package job first authenticates
and verifies the fixed public 0.1.9 ZIP, then rewraps only its six prebuilt plugin
payloads with this tool. It verifies the generated ZIP/checksum and confirms that
all six payload hashes are unchanged. Temporary packages are removed, not uploaded
or published. This exercises packaging, not an Unreal build or game startup.
