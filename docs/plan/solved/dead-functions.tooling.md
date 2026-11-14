# Remove six functions nothing calls

Status: solved 2026-11-14. Was: draft 2026-11-13, size S, needs nothing first.

## Problem

`python3 -I tools/code_map.py unused` lists six functions that a header declares and nothing calls (not the game, the
tools or the unit tests); `grep -rnw NAME src tools tests` shows only the declaration and the definition:

* `Logger::setEnabled` (`src/core/logger.h:21`)
* `Timer::StartTime`, `Timer::SetStartTime` (`src/core/timer.h:25-26`)
* `PlayerStats::StaminaRatio` (`src/level/player_stats.h:93`, `player_stats.cpp:123`)
* `MotionFx::toonBlurFirst` (`src/render/motion_fx.h:30`, `motion_fx.cpp:184`)
* `GameState::LoadAutosave` (`src/state/game_state.h:122`, `game_state.cpp:309`)

## Plan

* Grep each name again first (word match, also `LevelEditor/`, `ViewModel/`, `Play/`, docs): a caller added since
  keeps it.
* Delete the declaration and the definition of each. Then what only they used: a member or a file-level variable
  that is now only written (`gToonBlurFirst` in `motion_fx.cpp`, the logger's enabled flag), a setter with no getter
  left, an include. Stop there: no wider clean-up.
* Docs that name one of them (`docs/development.md` "Where is X", `docs/glossary.md`): drop the name.
* `python3 -I tools/code_map.py unused` prints 0 candidates afterwards, or only ones this plan explains.
* `make check`.
