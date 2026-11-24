# Source files by topic: sub-directories inside the libraries, misplaced files moved

Status: draft 2026-11-24, size L, needs nothing first.

## Problem

The directory is the library ([development.md](../development.md#libraries)): `src/level/` is `liblevel.a`, the one
GL-free library under sim and render that the unit tests, the editor, the level tools and the balance sim link. So
every GL-free piece that a test or a tool needs landed in `src/level/`, whatever its topic: 139 files, of which about
half are not about levels. A reader looking for the scroll bar opens `src/ui/` and finds nothing; `level/scroll_bar.h`,
`level/pointer.h`, `level/hud_icon.h`, `level/input.h` (a name `input/input.h` also has) look misplaced.

A plain move to `src/ui/` breaks the build: `src/ui/` is in the game only, and the editor (`scroll_bar`, `ui_layout`),
render (`ui_layout`, `settings_ini`, `inscription_layout`) and the unit tests read these headers.

The other directories are flat too: `sim/` has 77 files (`dungeon_*`, `monster_*`, the model files), `graphics/` 35
(`dungeon_render_*`), `ui/` 54 (menu, journal, HUD parts), and a few game files sit in the wrong game directory
(`graphics/draw.*` composes the whole frame from `state/` and `ui/`, the cause of the `graphics <-> ui` and
`graphics <-> state` include cycles in `tools/code_map.py graph`).

## Decision

Keep one library per top directory (`make layers` keeps reading the layer from the path), and allow one level of
topic sub-directories inside any `src/<dir>/`: `src/level/ui/scroll_bar.h` is liblevel, topic UI. The sub-directory
names repeat the game's directories (`ui`, `input`) so a reader finds the GL-free half next to where they look.

Rejected (user 2026-11-24: `level/ui/scroll_bar.h` is fine): a new library directory for the GL-free UI and input
logic (`src/front/` or similar). It needs its own layer slot between level and sim (sim includes `status_list.h`,
`save_slots.h`), a new `.a`, link-line changes in five programs, and gains little over a sub-directory.

## Plan

Sub-directory names (user 2026-11-24): the implementer decides them (`tuning/` or `numbers/`, `scenario_script` in
`files/` or a `test/`, ...); the ones below are suggestions.

### 1. Build and checks read sub-directories

* `makefile`: `SOURCES`, `*_LIB_SOURCES`, `*_LIB_HEADERS`, `FORMAT_FILES` also take `src/*/*/*.cpp|h`
  (`src/level/*/*.cpp` etc.). Object paths keep the tree (`build/src/level/ui/scroll_bar.o`): check the mkdir rule.
* `tools/check_layers.sh`, `tools/check_sim.sh`, `tools/tidy.sh`, `tools/code_map.py`, `tools/gl_legacy.py`,
  `tools/docs_check.py`, `tools/where.py`, `.clang-tidy` / `.clang-format` header filters, the unit-test shard script:
  each globs `src/*/`; make them see `src/*/*/`. `code_map.py graph` groups by the top directory (the library), with
  an option for sub-directories.
* Includes stay relative to the including file (`"../../level/level.h"` from `src/level/ui/`).
* Rule in [development.md](../development.md#libraries): the top directory is the library, a sub-directory is a topic
  inside it; at most one level deep.

### 2. `src/level/` by topic (the main part)

Stays at the top of `src/level/` (the level and its cells): `level`, `tile_defs`, `campaign`, `containers`, `decor`,
`decor_scatter`, `inscriptions`, `shrines`, `sun_beam`, `terrain`, `zone_look`, `zone_music`, `movement`,
`climb_path`, `view_window`, `loot`.

| New place | Files | Why |
|---|---|---|
| `level/check/` | `level_check`, `level_check_dressing`, `level_check_internal`, `level_check_mechanisms`, `level_walker`, `path_script` | the checker, 6 files |
| `level/gen/` | `level_gen`, `level_gen_content`, `level_gen_dressing`, `level_gen_gates`, `level_gen_layout`, `level_builder` | the generator, 6 files |
| `level/rules/` | `items`, `item_bag`, `monster_kinds`, `damage`, `curses`, `poison`, `bleeding`, `tier_drain`, `vitals`, `head_bump`, `trap_hurt`, `spike_cycle`, `skills`, `archer_skills`, `player_stats`, `player_actions`, `progression`, `offering_gift`, `quick_potion`, `judgment`, `run_stats`, `interaction`, `world_events`, `journal`, `journal_ids` | game rules and data rows, not the level |
| `level/tuning/` | every `gameplay_*.h` (13) | the numbers by topic (keep the file names) |
| `level/ui/` | `ui_layout`, `inventory_layout`, `skills_layout`, `inscription_layout`, `scroll_bar`, `pointer`, `page_turn`, `map_marks`, `message_log`, `status_list`, `hud_icon`, `controls_help`, `credits_text`, `journal_text` | the screens' GL-free layouts and rules; only each other include them inside liblevel |
| `level/input/` | `bindings`, `input` renamed `keys` (clash with `input/input.h`), `mouse_look`, `camera_look` | key codes, bindings, look maths |
| `level/files/` | `settings_ini`, `save_slots`, `leaderboard`, `riddle_file`, `texts`, `names`, `scenario_script` | files on disk and their parsers |
| `core/` | `rng`, `rgb` | generic, include nothing of liblevel; base library |

Checked 2026-11-24: none of the `level/ui/` and `level/input/` files is included by a level/rules file, except
`ui_layout.h` including `rules/interaction.h` (fine: same library). Re-check with `grep` before moving; a file whose
users disagree with its row stays where its users are and the commit says so.

### 3. `src/sim/`, `src/graphics/`, `src/ui/`, `src/render/` sub-directories

* `sim/dungeon/`: `dungeon`, `dungeon_*` (29), `sim_links`, `mechanisms`, `hazards`, `projectiles`, `cut_scene`,
  `gate_travel`, `stargate_dial`. `sim/monster/`: `monster`, `monster_*` (9). `sim/model/`: `md3_mesh`, `model_info`,
  `figures`, `render_config`. Top: `player`, `player_control`, `particles`.
* `graphics/dungeon/`: `dungeon_view`, `dungeon_render*` (10), `world_draw`.
* `ui/menu/`: `menu*` (7), `confirm_box`, `records_view`, `win_entry`. `ui/journal/`: `journal_*` (3), `page_curl`.
  `ui/hud/`: `player_hud`, `player_hud_view`, `boss_bar`, `level_gem`, `status_box`, `zone_banner`,
  `interact_prompt`. `ui/map/`: `map_view`, `map_zoom`.
* `render/gl/`: `gl_context`, `gl_includes`, `gl_state`, `batch`, `default_shaders`, `mat4`, `matrix_stack`,
  `wide_lines`, `shader`.

### 4. Single misplaced files (game side)

* `graphics/draw.*` → `game/draw.*`: it composes the frame from `state/` and `ui/` (HUD parts); only `game/` and
  `test/` include it. Ends the `graphics <-> ui` and `graphics <-> state` cycles (re-run `code_map.py graph`).
* `graphics/frame_rate.*` → `game/`: the Frame rate option drives `game/window.h`'s loop.
* `graphics/loading_text.*` → `ui/`: text layout of the loading screen.
* `graphics/monster_draw.cpp` includes `ui/player_hud.h` for `PlayerHud::drawStatusCard`: move that function to a
  `ui/hud/status_card.*` both include, or leave and note it.
* `sim/cut_scene.*` and `sim/dungeon_cutscene.cpp`: two names for one topic; put them side by side
  (`sim/dungeon/cut_scene*`), merge if small.
* `sim/dungeon_cheats.*` (test-only: scenarios, unit tests): name says so; leave in `sim/dungeon/`, or `sim/test/`.
* `state/gameplay_commands.h` is included only by `input/`; `input <-> state` cycle. Look, move if it is input's.
* `render/brightness.*`: used only by `graphics/` and `state/`, no tool. May move to `graphics/` (game only); low
  value, implementer's choice.
* `sim/render_config.h`: a sim header named "render"; consider `world_units.h`. Optional.

### 5. Docs and references

* `docs/development.md` (Project layout, Libraries table), `docs/where-is-x.md` rows (every moved path; `where.py`
  must still find them), `docs/glossary.md` rows with paths, `docs/ui.md`, `docs/testing.md`, other docs that name
  `src/...` paths (`grep -rn "src/level/" docs tools tests AGENTS.md`). Solved plans are history: no edits.
* Code comments that name a path (`ui/inventory.h draws it`): update the moved ones (`grep` each old path).
* `git mv` every file (history follows). One commit per step (1, 2, 3, 4) so a broken step is easy to find; each
  passes `make check` and `make layers`.

## Done when

* `make check`, `make tidy-all`, `make test` (scenarios) pass; the editor, model viewer, level tools, balance sim
  build.
* `src/level/` top holds about 16 files; `tools/code_map.py headers` lists fewer "only other directories" headers.
* `tools/code_map.py graph`: no `graphics <-> ui`, `graphics <-> state` cycle.
* `grep -rn` of every old path finds only solved plans.
