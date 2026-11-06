# Source directories follow the library layers

Status: solved 2026-11-06. Was: draft 2026-11-06, size L, needs nothing first.

## Problem

The `src/` directories are by topic (`world`, `entities`, `ui`, ...). The build layers base <- level <- sim <- render
<- game ([development.md](../../development.md#libraries)) cut across them. Most of this mismatch is in `src/world` (108 files):

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

* Proposed layout (the implementer can adjust the names; check [glossary.md](../../glossary.md) first):
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

## Done (2026-11-06)

Open points decided: the full split; relative includes kept (`"../level/level.h"`), rewritten by a script from each
file's old and new place, so `check_layers.sh` and `tidy.sh` resolve them as before.

* `src/core/` base (timer, logger, paths, worker_pool.h); `src/level/`, `src/sim/`, `src/render/` one per library;
  the game: `src/game/` (`game.cpp` with `main`, `sound`), `src/graphics/` (the game's drawing: `dungeon_render*.cpp`,
  `character_model`, `monster_draw`, `player_view`, `item`, `trap`, `draw`, `fire`, `particles_draw`, `frame_rate`),
  `ui/`, `state/`, `input/`, `test/` for the rest. `src/world/` and `src/entities/` are gone.
* Moved by the layer the makefile lists gave each file: `gameplay_config.h` to `level/`, `render_config.h` to `sim/`,
  `ui_draw` to `render/`, the pure layouts (`ui_layout`, `inventory_layout`, ...), `bindings`, `input.h`,
  `mouse_look.h`, `camera_look.h`, `settings_ini`, `save_slots`, `scenario_script`, `hud_icon.h`, `pointer.h` to
  `level/`.
* Makefile: `src/<library>/*.cpp` and `*.h` wildcards; `SIM_FILES` gone (`check_sim.sh` gets the sim's files).
  `.clang-tidy` `HeaderFilterRegex` names the new directories.
* Paths in docs, tools, content comments and open drafts rewritten (solved and won't-do plans keep the old ones).
* One `git mv` commit, then the fixes; both in `.git-blame-ignore-revs`.
