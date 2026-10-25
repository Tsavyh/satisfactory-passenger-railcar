# Build Satisfactory Passenger Railcar

[Español](BUILDING.es.md) · [Project overview](README.md)

This repository is the **plugin**, not a complete Satisfactory SDK checkout. Its internal name stays `TsabsPassengerWagon`.

## Verified toolchain for 0.1.13

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

```bat
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%CD%\FactoryGame.uproject" -unattended -NullRHI -NoSound -nosplash -ExecCmds="Automation RunTests Tsabs.Passenger" -TestExit="Automation Test Queue Empty" -ReportExportPath="%CD%\Saved\PassengerTestReport"
```

## 0.1.13 verification and inherited coverage

39 editor cases passed: **36 without warnings and 3 with SDK warnings**, with **134 warning entries**, zero failures and zero not-run cases. Seven package acceptance checks passed. Editor build and Shipping/cook both exited 0; a real Windows DX11 menu startup loaded the native prerequisite with zero streaming errors. This preparation reruns only standalone checks from the exported tree: **26 `/fp:precise` + 26 `/fp:fast` with MSVC 14.38**, and **2 Python native prerequisite contracts**. No new GCC/Clang, Linux-host Unreal build, Proton or hosted-CI result is claimed.

The source includes local crosshair forwarding/suppression, native speed-panel composition, retained-widget reconstruction/ownership checks, menu CDO category/order checks and the inherited post-0.1.10 lifecycle regressions. Those regressions cover scoped reservation cleanup after synchronous UObject invalidation, engine destruction of an initialized empty fixture car, a destroyed anchor failing closed, idempotent restoration hooks, UMG parent removal and fresh-HUD reentry.

Direct fixture hooks or `NativeTick` calls are not complete Slate reconstruction, native gameplay possession, RPC delivery or rendered-menu proof. Human acceptance is separate: 0.1.11 HUD functionality, 0.1.12 minimal-panel appearance and 0.1.13 Transport/Railway placement were accepted in singleplayer. No real death/disconnection, save serialization, human-driver/autopilot travel or new remote-client lifecycle acceptance is implied.

Historical two-player user approval and the contributor's scoped Proton startup report belong to 0.1.8. The changed 0.1.10 exit hint and 0.1.11–0.1.13 passenger UI still need a grouped remote-client check. Both host and clients must use the same version. Contributor commands above are retained, not newly certified on other compilers. No workflow is added by this export.

[`SOURCE-MANIFEST.json`](SOURCE-MANIFEST.json) records all 58 exact reviewed plugin inputs and the reduced public binary checksum. Public wrappers, MIT/third-party boundaries, PR #1 credit and current main history are preserved. This is source correspondence, not a bit-reproducible build claim. Keep private diagnostics outside the repository.
