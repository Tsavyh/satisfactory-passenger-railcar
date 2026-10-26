# Offline Windows Steam release verification

[Español](RELEASE-VERIFICATION.es.md) · [Build instructions](../BUILDING.md)

Requires **Python 3.12**, standard library only. Run from the repository root;
no SDK, compiler, game, mod manager, network access or Python packages are needed.
The verifier reads the ZIP in place: it never extracts, installs or executes its
contents and does not change game settings, installations or saves.

```sh
python -B tools/verify_release.py satisfactory-passenger-railcar-0.1.13-windows-steam.zip --expected-version 0.1.13
```

To additionally anchor the complete ZIP to a checksum obtained **separately from
a trusted source**, append `--sha256` and the expected 64 lowercase hexadecimal
characters. For the observed public 0.1.13 wrapper:

```sh
python -B tools/verify_release.py satisfactory-passenger-railcar-0.1.13-windows-steam.zip --expected-version 0.1.13 --sha256 49f1785edb02b609b6e595087ed128bf1156905bf134ab72d65cc658bd2f8a73
```

These commands also work in Windows CMD with Python 3.12 selected as `python`.
On Linux, use `python3.12` if needed. Exit **0** means every implemented package
check passed. Refusals return **1**, argument errors **2**, with a fixed sanitized
message; archive names, paths and parsed values are not echoed. `--help` returns
0 for usage only, without checking a package. Safety checks also remain active
with Python `-O`.

## Accepted wrapper

Only `satisfactory-passenger-railcar-VERSION-windows-steam.zip` is accepted;
legacy `TsabsPassengerWagon-…-Steam-Proton-Publico.zip` names are not supported.
`--expected-version` is required and must be numeric `x.y.z`, with no prefixes,
suffixes, whitespace or leading zeroes (except a component equal to zero).
It must equal both descriptor `VersionName`/`SemVersion` and manifest `version`.
The verifier does not compare against the repository's changing descriptor.

Required nonempty regular files:

- Root `MANIFEST.json`, `LEEME.txt`, `LICENSE`.
- `VAGON/TsabsPassengerWagon/TsabsPassengerWagon.uplugin`.
- Under `VAGON/TsabsPassengerWagon/Binaries/Win64/`:
  `FactoryGameSteam-TsabsPassengerWagon-Win64-Shipping.dll` and
  `FactoryGameSteam-Win64-Shipping.modules`.
- Under `VAGON/TsabsPassengerWagon/Content/Paks/Windows/`:
  `TsabsPassengerWagonFactoryGame-Windows.pak`, `.ucas` and `.utoc`.

One optional `Solo-SML-x.y.z.smmprofile` is allowed only in the observed SML-only
schema: profile name `TsabsRailTest-SML312`, enabled SML at `=x.y.z` matching its
filename, required target `Windows`, lockfile version integer 1 with no locked
mods, and metadata containing only gameVersion integer 513885. Extra fields,
mods, settings or paths are refused. Other profile schemas require a deliberate
verifier update, not a bypass.

All other files are refused, including SDK/SML binaries. Explicit directories
are optional and must be actual ancestors of the permitted files. Only stored
and deflated entries are supported. Unsafe/nonportable paths, exact or
case-folded duplicates, symlinks/special file types and encrypted entries are
refused. Metadata acceptance is deliberately host-specific, not a general ZIP
conformance claim: supported made-by hosts are DOS (0), Unix (3) and NTFS (10).
DOS/Unix accept a DOS-compatible low byte (readonly/hidden/system/directory/
archive/normal) and an upper-word Unix type that is unspecified, regular or a
matching directory, including Python's Windows DOS-host mode-0600 form. Bits
8–15 are unsupported for those hosts. The supported NTFS form uses native
Windows attributes only: those basic flags plus temporary, compressed and
not-content-indexed; all other bits, including a nonzero upper word, are refused
without interpreting it as POSIX mode. Zero attributes remain valid.
A directory flag requires a trailing slash; a slash with unspecified attributes
is supported. NORMAL must stand alone and cannot mark a directory. Reparse,
reserved device, volume, sparse/offline, EFS and other unsupported forms are
refused before opening payloads. EFS is filesystem metadata, **not a ZIP
encryption bypass**; the independent ZIP encryption-flag refusal is unchanged.
These rules follow the host-dependent fields in [PKWARE APPNOTE sections 4.4.2
and 4.4.15](https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT),
[Microsoft's attributes](https://learn.microsoft.com/windows/win32/fileio/file-attribute-constants)
and [Python's ZipInfo API](https://docs.python.org/3.14/library/zipfile.html#zipinfo-objects).
Unknown hosts or otherwise valid but unsupported metadata are refused, not
certified unsafe to extract by every possible extractor.

Limits before payload decompression: ZIP 128 MiB, member 128 MiB, total declared
uncompressed size 256 MiB, at most 128 entries. JSON is limited to 1 MiB; all JSON
objects reject duplicate keys, invalid encoding, nonstandard constants and
malformed syntax. Hashing streams bounded chunks, and PE parsing uses bounded
reads/RVA mappings rather than loading or executing a DLL. PE parsing also caps
estimated decompression work (including backward-seek revisits) at 512 MiB;
this is a finite work budget, not a wall-clock deadline.

## What a pass establishes — and does not

- The manifest mapping exactly covers every regular file except itself, with
  lowercase SHA-256 digests matching all payload bytes. **This is internal
  consistency, not independent authenticity**: someone replacing the payload
  can also regenerate its manifest. The optional external ZIP checksum anchors
  those complete ZIP bytes only to the separately trusted expected checksum.
- The descriptor identifies the `TsabsPassengerWagon` runtime module and an
  enabled, nonoptional SML dependency. The `.modules` file uses BuildId `SML`
  and maps only that module to the exact included Shipping DLL.
- The DLL has bounded, readable x64 PE32+ DLL headers and a normal import table
  retaining `FactoryGameSteam-FactoryDedicatedClient-Win64-Shipping.dll`.
  A matching string outside the import table is insufficient. The parser
  checks section/RVA bounds, import descriptors, names and thunk arrays. This
  is not a complete Windows loader/PE conformance validator, and does not prove
  that the installed game supplies the right DLL or that loading succeeds.

A pass is **package verification only**: not gameplay, multiplayer, engine/game
compatibility, successful game startup, malware scanning, arbitrary private-data
or DLP certification. The exact file allowlist is not a content privacy scanner.
Manifest historical test booleans are not accepted as runtime evidence. Cooked
payloads are hashed, not interpreted or validated by Unreal. Future wrapper
layouts may be refused until reviewed and explicitly supported.

## Offline tests

```sh
python3.12 -B -m unittest discover -s Tests -p 'test_*.py' -v
```

On Windows, select only the portable verifier suite (the Bash policy-runner
tests in full discovery are Linux-only):

```bat
python -B -m unittest discover -s Tests -p test_release_verifier.py -v
```

The verifier tests create small,
explicitly **synthetic** PE/import/ZIP fixtures, including adversarial mutations;
they do not rebuild or distribute a mod binary. Existing host-policy tests also
need their documented Bash/C++ environment on Linux. The contributor exercised
the original verifier on Linux/Python 3.12 with the public 0.1.9 wrapper. This
metadata correction was exercised on Windows 10/Python 3.14.7, normally and with
`-O`: all 25 verifier tests passed, preserving the original 21, and the real public
0.1.13 ZIP passed with the checksum above. No new Linux or hosted-CI run is claimed.
