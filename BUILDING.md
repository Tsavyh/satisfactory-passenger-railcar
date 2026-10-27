# Build Satisfactory Passenger Railcar

[Español](BUILDING.es.md) · [Project overview](README.md) · [Release notes EN/ES](RELEASE-NOTES.md)

This repository is the **plugin**, not a complete Satisfactory SDK checkout. Its internal name stays `TsabsPassengerWagon`. The commands below are contributor instructions; updating this documentation does not run them or certify a new CI result.

## Recorded release toolchain for 0.1.15

| Component | Pinned environment |
| :--- | :--- |
| Game | Satisfactory Steam 1.2.4.1, CL 513885 |
| Loader | SML 3.12.0 |
| Starter Project | satisfactorymodding/SatisfactoryModLoader, commit `8eebc9e659f0b2ff4392d2f8cf6e2cff60c8ea49` |
| Engine | Coffee Stain Unreal Engine 5.6.1-css-83 |
| Compiler | Visual Studio 2022, MSVC 14.38 |
| Windows SDK | 10.0.26100.0 |
| Wwise | 2025.1.6.9117; integration 2025.1.6.9117.4248 |

Obtain and configure dependencies separately using the [Satisfactory Modding documentation](https://docs.ficsit.app/). They are not redistributed or relicensed by this project. Building the Unreal plugin on a Linux host has not been validated; Linux standalone tests do not establish that support.

## Source identity

The release target is **v0.1.15**, with **66 exact reviewed plugin inputs** recorded in [`SOURCE-MANIFEST.json`](SOURCE-MANIFEST.json). Public tools and documentation are separate from that plugin input set. Final publication must contain the matching tag, source manifest and runtime artifact; a version label alone is not provenance. Source correspondence is **not a bit-reproducible build claim**.

The project's own code is **[MIT](LICENSE)**. Preserve [MikelCalvo's PRs #1–#5 and original history](CONTRIBUTORS.md). SDK, Engine and game resources remain outside the MIT grant; see [NOTICES.md](NOTICES.md). Keep private diagnostics, account identifiers and personal paths outside public documentation and artifacts.

## Layout and Unreal package

Place this repository in `Mods/TsabsPassengerWagon` inside a separately prepared Starter Project. The descriptor must be `Mods/TsabsPassengerWagon/TsabsPassengerWagon.uplugin`, not nested in an additional directory. Do not reuse another version's Binaries/Intermediate.

Open Windows CMD in the Starter Project root and set `UE_ROOT` to your installed CSS Engine:

```bat
set "UE_ROOT=C:\Path\To\UnrealEngine"
call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" FactoryEditor Win64 Development "%CD%\FactoryGame.uproject" -WaitMutex -NoHotReloadFromIDE
call "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" -ScriptsForProject="%CD%\FactoryGame.uproject" PackagePlugin -project="%CD%\FactoryGame.uproject" -DLCName=TsabsPassengerWagon -Target=FactoryGameSteam -platform=Win64 -clientconfig=Shipping -build -installed -nocompileeditor -utf8output -verbose -timestamps -nop4 -UbtArgs="-MaxParallelActions=4 -NoUBA -NoXGE -NoFASTBuild"
```

These targets and packaging arguments belong to the recorded release-build workflow, with local paths substituted. Build the Editor module before using `-nocompileeditor`. Inspect `Saved/ArchivedPlugins/TsabsPassengerWagon` before installation. Leave automatic deployment disabled. Do not treat these instructions as permission to install, launch the game or change saves/configuration.

## Generate and verify the public wrapper

After building/cooking, use [release packaging](docs/RELEASE-PACKAGING.md) to create a ZIP and `SHA256SUMS.txt` in a **new directory outside both input and source trees**:

```sh
python -B tools/package_release.py "/path/to/cooked-plugin" --output "/path/to/new-release"
python -B tools/verify_release.py satisfactory-passenger-railcar-0.1.15-windows-steam.zip --expected-version 0.1.15
```

The first command reads the **cooked plugin root**, not this source checkout. The verifier example assumes the named ZIP is in the current directory; otherwise pass its actual output path. Python 3.12 is the CI baseline; the tools have also been exercised on Windows with Python 3.14.7. See [release verification](docs/RELEASE-VERIFICATION.md) for optional external SHA-256 and layout constraints.

The public 0.1.15 wrapper contains **nine members**: six runtime files under `VAGON/TsabsPassengerWagon/`, plus `LEEME.txt`, `LICENSE` and `MANIFEST.json`. **No PDB, optional SML profile, SDK, Engine/game assets, private logs or saves.** SML must be installed separately. The packager does not compile/cook Unreal, install or execute payloads, or publish a release. Package verification is not game-load, compatibility, malware or private-data certification.

## Standalone checks without Unreal

In an x64 MSVC Native Tools CMD, from the plugin root:

```bat
call Tests\run-policy-tests.cmd
python -B -m unittest discover -s Tests -p test_native_prerequisite.py -v
```

The policy runner executes 26 cases with `/fp:precise` and again with `/fp:fast`: strict 30 km/h boundary, reverse travel, NaN and infinity. The Python contracts check the explicit native module dependency and retained class reference.

On Linux with Bash and a GCC/Clang C++20 compiler:

```sh
CXX=clang++ bash Tests/run-policy-tests.sh
CXX=g++ bash Tests/run-policy-tests.sh
CXX=clang++ python3 -B -m unittest discover -s Tests -p 'test_*.py' -v
```

The Bash runner uses temporary build outputs and the same cases in `-fno-fast-math` and `-ffast-math` modes. `CXX` selects one compiler executable (default `c++`); keep it consistent for Python runner tests. Fixed IEEE-754 fixture bits avoid arithmetic/infinity generation that Clang can transform under fast-math. These are host policy/runner checks, not an Unreal build.

Portable package suites can also be run independently, including with optimization enabled:

```sh
python -B -m unittest discover -s Tests -p 'test_release_*.py' -v
python -B -O -m unittest discover -s Tests -p 'test_release_*.py' -v
```

Synthetic verifier/packager fixtures test the tools; they are not rebuilt mod binaries.

## What GitHub can check automatically

The existing workflows are designed for push, pull request and manual `workflow_dispatch` events, without the proprietary game/SDK or repository secrets:

- [Standalone checks](.github/workflows/standalone-checks.yml): Ubuntu 24.04 with GCC and Clang, precise/fast-math policy runs, discovered Python suites and ShellCheck; Windows Server 2022 with MSVC precise/fast policy runs and native-prerequisite Python contracts.
- [Release package verification](.github/workflows/package-verification.yml): verifier and packager suites on Ubuntu and Windows with Python 3.12, both normally and with `-O`.
- The reference-package job authenticates the **fixed public 0.1.9 ZIP**, rewraps its six prebuilt payloads, verifies the generated package/checksum and compares payload hashes. It neither builds 0.1.15 nor runs a game; a successful 0.1.9 roundtrip cannot certify the 0.1.15 binary.
- New standalone Windows routing CI is intended to run **176 cases: 150 rerail + 23 runtime availability + 3 Editor-branch availability**. A simulated parent boundary or Editor branch is **not Unreal Editor execution**, actual native RPC delivery, physics, native rerailing or real SML detour lifecycle validation. Their hosted run status must be checked separately; no new routing-CI success is claimed here.

The checked-in workflows use read-only repository permissions, do not persist Git credentials and pin actions to commit SHAs. They print tool versions and use bounded jobs; hosted MSVC is not pinned to the release-build compiler. Workflow configuration is not evidence that a particular run passed. Use the actual run and its selected revision to establish CI status; do not replace automated checks with an AI assertion of success.

Hosted CI without the licensed SDK/game **does not compile Editor, cook/package the Unreal plugin, launch Satisfactory/Proton or validate human gameplay**. These workflows do not publish or deploy. Keep proprietary-toolchain builds and human checks separate rather than promising GitHub can perform them.

## Unreal automation and recorded 0.1.15 evidence

With the separately prepared SDK and freshly built Editor module:

```bat
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%CD%\FactoryGame.uproject" -unattended -NullRHI -NoSound -nosplash -ExecCmds="Automation RunTests Tsabs.Passenger" -TestExit="Automation Test Queue Empty" -ReportExportPath="%CD%\Saved\PassengerTestReport"
```

Check the exported leaf results and failed/not-run counters, not only the process exit. The recorded 0.1.15 run has **43 successful cases: 40 ordinary successes + 3 with SDK warnings, 134 warning entries**. Editor build and UAT/Shipping/cook exited **0**; the fresh Shipping package reached the real Windows/Steam menu with **zero streaming errors**. These are release-preparation records, not new executions performed by this documentation update or by hosted CI.

Inherited coverage includes local crosshair forwarding/suppression, minimal native speed-panel composition, retained-widget ownership/reconstruction, menu category/order and lifecycle regressions. Rerail routing and availability fixtures must retain their boundary labels: parent sentinels and SDK stubs do not prove native runtime behavior. Direct hooks or `NativeTick` calls are not complete Slate reconstruction, real possession, RPC delivery or rendered-menu proof.

Human acceptance is separately scoped: the 0.1.14 in-game rerailing fix was accepted; the 0.1.15 singleplayer name, description, cost (**5 ordinary Modular Frames + 10 Steel Pipes + 10 Steel Beams**), native Freight Car unlock/placement and construction were accepted. Availability follows **Tier 6 — Monorail Train Technology** (`MonorailTechnology`); free/no-unlock prototype rules are obsolete. One passenger place and the authority/derailment guards remain unchanged.

The 0.1.13 HUD acceptance and **general human SP/MP approval of 0.1.13** are retained. The latter is a user report, not independent agent gameplay or certification of named platforms, player roles or individual scenarios. The subsequently identified rerailing exception was fixed and accepted in 0.1.14. The contributor's scoped Proton startup report remains historical **0.1.8** evidence. No new 0.1.15 multiplayer, Proton or dedicated-server acceptance, human-driver/autopilot travel, real-game save/load, death/disconnection or saved-passenger restoration is implied. Host and clients must use the same version. Further gameplay development, including 0.1.16, remains paused.
