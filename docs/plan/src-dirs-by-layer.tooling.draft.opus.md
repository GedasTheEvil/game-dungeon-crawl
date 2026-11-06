# Source directories follow the library layers

Status: draft 2026-11-06, size L, needs nothing first.

## Problem

The `src/` directories are by topic (`world`, `entities`, `ui`, ...). The build layers base <- level <- sim <- render
<- game ([development.md](../development.md#libraries)) cut across them. Most of this mismatch is in `src/world` (108 files):

* level library: 28 `world/` files, plus 6 from `ui/` (`*_layout`, `status_list`, `message_log`, `page_turn`), 2 from
  `state/` (`settings_ini`, `save_slots`), `input/bindings`, `entities/player_stats`, `test/scenario_script`.
* sim library: ~29 `world/dungeon_*.cpp` plus 15 `entities/` files.
* game (no library): the 11 `world/dungeon_render*.cpp` and `entities/` `character_model`, `item`, `trap`,
  `monster_draw`, `player_view`.

Costs:

* `LEVEL_LIB_SOURCES` and `*_HEADERS` in the makefile are long hand-kept lists. Every new level-side file must be
  added by hand, or it silently builds into the game only.
* A file's directory does not tell its layer. `world/dungeon_sun.cpp` (sim, no GL) and `world/dungeon_render_sun.cpp`
  (game, GL) sit side by side. `make layers` catches a wrong include only after it is written.
* `src/world` is too large to scan. Agents work from the "Where is X" table instead.

## Plan

Move the files so that each layer has its own directory. Then the makefile picks up each library with a wildcard, and
`check_layers.sh` reads the layer from the path.

* Proposed layout (the implementer can adjust the names; check [glossary.md](../glossary.md) first):
  * `src/core/` base (as now) plus the game's `main` only if that stays clear, otherwise `src/game/`.
  * `src/level/`: today's level library (rows, level format, checker, generator, campaign, items, journal data,
    the pure layouts from `ui/`, bindings, settings_ini, save_slots, player_stats, scenario_script).
  * `src/sim/`: `dungeon.h`, `dungeon_*.cpp` without render, `sim_links.h`, `dungeon_rules.h`, the sim's `entities/`
    files (monster*, player, figures, particles, md3_mesh, model_info, trap_hurt.h).
  * `src/render/`: today's render library (`graphics/` part, `ui_draw`).
  * Game: `src/game/` or keep `ui/`, `state/`, `input/`, `test/` and `graphics/` for what remains, plus the
    `dungeon_render*.cpp` files (e.g. `src/graphics/dungeon/`) and the drawing half of `entities/`.
* Keep the file names (`dungeon_render_sun.cpp` stays as it is), so that grep and the plans' prose still find them.
* Do the moves in one `git mv` commit with no content changes, then the include and makefile fixes. Add both commits to
  `.git-blame-ignore-revs`.
* Makefile: replace the lists with `$(wildcard src/level/*.cpp)` and so on. Drop `*_LIB_HEADERS` where a
  directory covers them. `check_layers.sh` and `check_sim.sh` get their file sets from the directories.
  `.clang-tidy` `HeaderFilterRegex` gets the new directory names.
* Docs: development.md (Project layout, Libraries, Code structure, the paths in Where is X), ui.md, testing.md, levels.md,
  AGENTS.md (`src/world/monster_kinds.cpp` in Boss abilities), tools/editor/readme.md. Solved plans keep the old
  paths (history). `make check` must pass and `grep -rn 'src/world/' docs/*.md AGENTS.md` must find no stale paths.

## Open points

* Full split, or only the cheap part: move `dungeon_render*.cpp` out of `world/` and the sim files into `src/sim/`,
  and leave the level library as a list. The cheap part is about half the gain at about a quarter of the churn.
* Include style: the code uses relative includes (`"../world/level.h"`). After the moves, either fix them with a
  script or switch to `-Isrc` with `"level/level.h"`. Pick one and use it everywhere.
* Running worktree tasks conflict with the move. Do it when no other task is open.
