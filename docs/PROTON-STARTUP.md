# Proton startup check: 0.1.8

This is a menu-startup result, not a claim of complete Proton or multiplayer
compatibility. The runtime fix was already part of the upstream 0.1.8 release;
no plugin runtime code was changed for this check.

## Environment

- Arch Linux x86_64, KDE Plasma 6.7.5 / Wayland, NVIDIA RTX 3070.
- Steam Satisfactory 1.2.4.1, CL 513885.
- Steam Proton 10.0-4, SML 3.12.0.
- Published `TsabsPassengerWagon-0.1.8-Steam-Proton-Publico.zip`.
- ZIP SHA-256:
  `1c0580e5d0a439c8974a1e7e72b94259761992f75b31fbc0e18b75b72007970f`.

The download was checked against GitHub's release asset digest and each installed
plugin file against the package manifest. The previous wagon was kept outside
`FactoryGame/Mods`. Proton, DLL overrides, game settings and existing launch
options were not changed for the check.

## Observed result

The affected machine previously stalled before the menu with 0.1.7 and logged
454 `LogStreaming: Error:` lines. Removing only that wagon restored SML-only
startup. With the official 0.1.8 package, the same machine reached the menu and
logged zero `LogStreaming: Error:` lines.

The 0.1.8 attempt included these checkpoints, in order:

```text
LogTsabsPassengerWagon: Display: TSABS_STARTUP_PREREQUISITE /Script/FactoryDedicatedClient.FGAbstractServerWidget
LogSatisfactoryModLoader: Display: Version check passed successfully! Game Changelist: 513885
LogSatisfactoryModLoader: Display: SML: 3.12.0
LogSatisfactoryModLoader: Display: TsabsPassengerWagon: 0.1.8
LogSatisfactoryModLoader: Display: Initialization finished!
LogLoad: Took 2.518971 seconds to LoadMap(/Game/FactoryGame/Map/MenuScenes/Map_Menu_1_02)
```

The main menu was also checked visually. No world was loaded during the recorded
check. Before/after hashes and timestamps of the 14 existing world `.sav` files
were unchanged; no new save files appeared. Steam updated an auxiliary
`steam_autocloud.vdf` timestamp without changing its content.

## Limits and next checks

- This was the published Windows/Steam binary under Proton, not a Linux-host
  build of the plugin. The full CSS Engine/MSVC toolchain was not available on
  this machine.
- Menu startup does not validate boarding, camera behavior, exit clearance,
  replication, human-driven travel, autopilot or a dedicated server.
- Existing nonfatal SDK/UI/online warnings remain; zero streaming errors does
  not mean that every log category is warning-free.
- Keep testing in a separate backed-up world. Do not use this startup result to
  claim that personal saves or multiplayer are safe.

Only the allowlisted startup checkpoints are reproduced here. Raw logs,
account identifiers, authentication/travel parameters and personal paths are not
part of this report.
