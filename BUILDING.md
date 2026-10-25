# Build Satisfactory Passenger Railcar

[Español](BUILDING.es.md) · [Project overview](README.md)

This repository is the **plugin**, not a complete Satisfactory SDK checkout. Its internal name stays `TsabsPassengerWagon`.

## Verified toolchain for0.1.10

| Component | Pinned environment |
| :--- | :--- |
| Game | Satisfactory Steam 1.2.4.1, CL 513885 |
| Loader | SML 3.12.0 |
| Starter Project | satisfactorymodding/SatisfactoryModLoader, commit `8eebc9e659f0b2ff4392d2f8cf6e2cff60c8ea49` |
| Engine | Coffee Stain Unreal Engine 5.6.1-css-83 |
| Compiler | Visual Studio 2022, MSVC 14.38 |
| Windows SDK | 10.0.26100.0 |
| Wwise | 2025.1.6.9117; integration 2025.1.6.9117.4248 |

Obtain and configure dependencies separately using the [Satisfactory Modding documentation](https://docs.ficsit.app/). They are not redistributed or relicensed by this project. Building on a Linux host has not been validated.

## Layout and package

Place this repository in `Mods/TsabsPassengerWagon` inside a separately prepared Starter Project. The descriptor must be `Mods/TsabsPassengerWagon/TsabsPassengerWagon.uplugin`, not nested in an additional directory. Do not reuse another version's Binaries/Intermediate.

Open Windows CMD in the Starter Project root and set `UE_ROOT` to your installed CSS Engine:

```bat
set "UE_ROOT=C:\Path\To\UnrealEngine"
call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" FactoryEditor Win64 Development "%CD%\FactoryGame.uproject" -WaitMutex -NoHotReloadFromIDE
call "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" -ScriptsForProject="%CD%\FactoryGame.uproject" PackagePlugin -project="%CD%\FactoryGame.uproject" -DLCName=TsabsPassengerWagon -Target=FactoryGameSteam -platform=Win64 -clientconfig=Shipping -build -installed -nocompileeditor -utf8output -verbose -timestamps -nop4 -UbtArgs="-MaxParallelActions=4 -NoUBA -NoXGE -NoFASTBuild"
```

These targets and packaging arguments were exercised for the published build, with local paths substituted. Inspect `Saved/ArchivedPlugins/TsabsPassengerWagon` before installation. Leave automatic deployment disabled. Different paths/toolchains may not produce byte-identical binaries.

## Standalone checks

In an x64 MSVC Native Tools CMD, from the plugin root:

```bat
call Tests\run-policy-tests.cmd
python Tests\test_native_prerequisite.py
```

The policy script compiles/runs 26 checks with `/fp:precise` and again with `/fp:fast`, covering the strict 30 km/h boundary, reverse travel, NaN and infinity. The Python contract checks the native module dependency and retained class reference. Both were also executed from the exported source tree.

On Linux with Bash and a GCC/Clang C++20 compiler:

```sh
CXX=clang++ bash Tests/run-policy-tests.sh
CXX=g++ bash Tests/run-policy-tests.sh
python3 -B -m unittest discover -s Tests -p 'test_*.py' -v
```

The Bash runner uses temporary build outputs and runs the same 26 cases in
`-fno-fast-math` and `-ffast-math` modes. `CXX` selects one compiler executable
(default: `c++`). Run the Python checks with the same `CXX` to exercise that
compiler through the runner. The fast-math fixtures use fixed IEEE-754 input
bits rather than arithmetic or infinity-valued library calls that Clang may
transform under those options. These are host policy/runner checks, not a build
of the Unreal plugin.

## Unreal automation

From the Starter Project root:

```bat
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%CD%\FactoryGame.uproject" -unattended -NullRHI -NoSound -nosplash -ExecCmds="Automation RunTests Tsabs.Passenger" -TestExit="Automation Test Queue Empty" -ReportExportPath="%CD%\Saved\PassengerTestReport"
```

Inspect the exported report, not only the process exit code. The 0.1.8 code passed 27 cases: 24 without warnings plus 3 with SDK/UI warnings, no failures or skipped cases. Starter Project native stubs do not prove actual in-game behavior or replication.

The package also passed checks for the retained FactoryDedicatedClient DLL import. A real Windows DX11 startup reached the menu with zero streaming errors. The published 0.1.8 binary also passed a [Proton menu-startup check](docs/PROTON-STARTUP.md) on the affected Linux client; subsequent multiplayer acceptance was separately reported by two testers. The new 0.1.9 notice still needs an in-game remote-client check. No private logs are published.

## 0.1.9 local verification

The 27 editor cases (24 without warnings plus3 SDK/UI-warning cases),7 package checks, MSVC policy checks in both modes and a real Windows DX11 menu startup passed. The transient notice fixture covers idle cruising, rejected-attempt feedback,3-second expiry/retrigger and clearing when slowing. RPC flags are checked, not actual network delivery. GCC/Clang and Proton results in PR#1 are attributed to its contributor; no hosted CI was configured for that PR.

## 0.1.10 verification

27 editor cases passed (24 without warnings, 3 with SDK/UI warnings), seven package checks and real Windows DX11 menu startup passed. The tester accepted the three hint/feedback checks in singleplayer: hidden above 30 after a sustained interval, restored below 30, and brief rejected-exit warning without dismount. The fixture also covers strict float boundaries, reverse and non-finite speeds. Remote-client verification of this UI is still pending; do not infer replication from direct NativeTick tests. No new GCC/Clang/Proton run is claimed for this release.

## Development regression coverage (tests only)

The post-0.1.10 development tree passes 31 editor tests: 28 without warnings
and 3 with existing SDK/UI warnings, with no failures or skipped cases. Four
new cases cover scoped reservation cleanup after synchronous UObject
invalidation, engine destruction of an initialized empty fixture car, a
destroyed anchor reference failing closed, and repeated restoration hooks
without duplicate pilots. The existing effective-key fixture also checks
real UMG parent removal, unrelated UI preservation and a fresh HUD on reentry.

These checks do not simulate a real death/disconnection, serialize a saved
game, run the game's native possession pipeline, or deliver network RPCs.
An EditorPreview actor must be initialized before testing engine EndPlay
routing; correcting that fixture prerequisite was not a runtime bug fix.
Runtime code, the descriptor and published binary remain unchanged. For the
exact source of the published binary use tag `v0.1.10`, not the development
branch. No new release, cook, Proton or remote-client result is implied.
