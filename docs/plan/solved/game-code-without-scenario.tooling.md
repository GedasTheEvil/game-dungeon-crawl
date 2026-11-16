# Game code does not call the scenario runner

Status: solved 2026-11-16. Was: draft 2026-11-16, size S, needs nothing first.

## Problem

The architecture review (2026-11-16, smell 1) found game code calling the test runner (`src/test/scenario.*`):
`state/game_state.cpp` picks the save paths with `Scenario::active() ? Scenario::outputDir()...`,
`state/settings.cpp` skips the settings file with `if (Scenario::active())`, and `graphics/draw.cpp` calls
`Scenario::onFrameRendered()`. state and graphics can't build without the test runner, and every new path has to know
about it.

## Plan

* `GameState::saveDir`: empty for `saves/` (`SaveSlots`), else the folder of the autosave, the quick save and the
  slots. The scenario runner sets it (lone run in `game.cpp`, each batch fork in `runForked`).
* `SettingsFile::SetEnabled(false)`: scenarios neither read nor write `saves/settings.ini`; `game.cpp` calls it.
* `setFrameHook()` in `graphics/draw.h`: called after the frame is drawn, before the swap; `game.cpp` registers
  `Scenario::onFrameRendered` for a scenario run.
* Only `game/game.cpp` (the composition root) includes `test/` headers.
