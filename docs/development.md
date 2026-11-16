# Development

Terms used in the game, the code and the docs: [glossary.md](glossary.md). Git workflow: [version-control.md](version-control.md). Hanging shells and waits: [stuck-processes.md](stuck-processes.md).

## Requirements

* Make
* GCC
* OpenGL
* GLUT
* SDL, SDL Mixer (the game's audio; the tools do not need them)
* Python 3 (`tools/plan.py`, run by `make check`; the asset scripts)
* clang-format, clang-tidy (`make format`, `make tidy`)

PNG loading uses the bundled single-header [stb_image](https://github.com/nothings/stb) (`external/stb/stb_image.h`), no extra library needed.

## Build and run

Data paths are relative to the repo root (`MEDIA_DIR`, `CONTENT_DIR` in `src/core/paths.h`). The game, editor and
model viewer move there themselves (the parent of `bin/`), so `bin/game` also starts from elsewhere; the level tools
read their file arguments from the current directory.

* `make` builds the game and every tool, in parallel (as many jobs as the load gate's cap; `make -j1` builds
  serially). Game: `./Play` (or `bin/game` from the repo root).
* ccache is used in front of the compiler when installed (`apt install ccache`; plan
  [check-build-header-rebuild](plan/solved/check-build-header-rebuild.tooling.md)): a rebuild after a branch
  switch, a merge or a reverted header edit hits the cache. `make NO_CCACHE=1` opts out; no ccache, no error. `ccache -s` shows the hit rate.
* `make release` (plan [release-zip](plan/solved/release-zip.tooling.md)): `release-<version>.zip` in the repo
  root (version: `git describe --always --dirty`; a dirty tree is refused unless `make release ARGS=--allow-dirty`).
  Holds `Play`, `bin/game` (the game only, built portably with `-march=x86-64-v2` in `build/release`, apart from the
  dev build), `media/`, `content/` and an empty `saves/`. Unzip and run `./Play` on a system with SDL, GL and GLUT
  installed (no dev tools needed). The music fallback `soundtrack.ogg` ships if it is present locally.
* `bin/game --level N|PATH|gen:SEED:D`: play a level by hand, menu skipped. `bin/game --play SCENARIO.txt`: play a
  scenario's setup (level, gear) by hand. `--no-sound` mutes either. See [testing.md](testing.md#play-by-hand).
* Level editor: `make editor`, `make run-editor [LEVEL=content/levels/lvl3]` (opens that file). See [tools/editor/readme.md](../tools/editor/readme.md).
* Model viewer: `make model-viewer`, `make run-model-viewer ARGS="media/models/monsters/anubis.md3"`.
* Level tools: `make level-tools`, then `bin/levelcheck content/levels/lvl*` (validate, rank by difficulty) and
  `bin/levelgen --seed 1 --difficulty 5 OUT` (random level), `bin/levelconvert FILE...` (old level files to the current
  format), `tools/level/shape.py LEVEL 12,33=step 17-18,21=low3` (set or clear cell shapes by mask or the editor's
  brush names, the drawing's shaped glyph kept in step; plan subcell-campaign). See [levels.md](levels.md).
* Balance sim (plan [monster-balance](plan/solved/monster-balance.md)): `bin/balance fight "giant rat" 30
  [--armor A] [--might M] [--hp P] [--difficulty D]` (one monster against a player of that level: tank in melee, kite
  with the bow, run past; HP lost as a share of max HP, seconds to the kill, potions drunk), `bin/balance table [--gear
  gen|play]` (every zone's monsters and boss against its player), `bin/balance curve` (the target campaign generated:
  XP and gear per level), `bin/balance today` (the same for `content/levels/`), `bin/balance tune [gen]` (each monster's
  HP and damage for its class's target in its first zone; play gear unless `gen`), `bin/balance home "giant rat" [--monster-hp HP]
  [--monster-damage D] [--minions N]` (one monster against the player of its first zone, with stats to try); any of
  them with `--amulet ID` (its place among the amulets) wears that amulet in every fight. The fights run in the unit
  tests'
  sim (`tests/unit/sim_world.h`) on an arena hall; `BALANCE_TRACE=1` prints each fight once a second.

### C++23

The build uses C++23 (`-std=c++23` in `CXXFLAGS` and `TIDY_CPPFLAGS`, `makefile`). In new code:

* An enum as an index: `std::to_underlying(e)`, never `static_cast<size_t>(e)`.
* `string::contains` (and `std::ranges::contains`, `std::ranges::any_of`) for a membership test, never `find(...) !=
  end()`.
* `std::ranges::fold_left(v, 0, std::plus{})` for a sum, not `std::accumulate` (in `.cpp` files: `<ranges>` views cost
  build time in a header).
* `std::unreachable()` after a switch in which every enumerator returns, only when the value cannot come unchecked
  from a file, a save or a scenario script (those keep a real fallback: reaching `std::unreachable()` is undefined
  behaviour).
* `std::views::enumerate` only when the loop reads the element and its index and the signed index needs no new cast
  (not when `i` is written, steps by more than 1, is compared with a `size()` or stored as a `size_t`); not in the
  per-frame paths (`src/graphics/`, `src/render/`, the simulation step). Plan cpp23-idioms found too few loops that fit
  to convert the old ones.
* Tools (`tools/`) print with `std::println` / `std::print` (`stderr` first for errors), not `printf`: `{}` needs no
  length modifier and no `.c_str()`; `%5d` is `{:5}`, `%-12s` is `{:<12}`, `%5s` is `{:>5}` (a string aligns left by
  default, a number right). `snprintf` into a buffer stays. The game (`src/`) stays on `printf`: `<print>` made the
  one measured file (`tools/balance/balance.cpp`) 1.7x slower to compile (plan cpp23-print).

## Project layout

* `src/` - one directory per library, then the game ([Libraries](#libraries), plan src-dirs-by-layer): `core` (base),
  `level`, `sim`, `render`; the game's own `game` (`main`, sound), `graphics` (the game's drawing), `ui`, `input`,
  `state`, `test` (scenario runner).
* `tools/editor/saved/` - the level editor's work-in-progress levels (not tracked by git).
* `bin/` - the game, the tools and the unit test runner (not tracked by git).
* `build/` - object files and libraries (not tracked by git).
* `tools/` - level tools (`level/`), level editor (`editor/`), model viewer (`model-viewer/`), Blender model scripts (`blender/`), sound and texture generators (`audio/`, `textures/`), the balance sim (`balance/`), scenario runner script, `profile_game.sh` ([performance-testing.md](performance-testing.md)), the check scripts (`check_*.sh`, `tidy.sh`, `load_gate.sh`), plan bookkeeping (`plan.py`, its tests `plan_test.py`, [plans.md](plans.md)), `friction_report.py` (what the checks and rules cost agents, from the Claude Code transcripts), `where.py` (the rows of [where-is-x.md](where-is-x.md) for a term), `code_map.py` (for an architecture review: includes between directories, headers that sit apart from their users, functions nothing calls; a report, not a check).
* `tests/` - unit tests (`unit/`), scenario scripts (`scenarios/`), play scripts (`play/`), test levels (`levels/`), riddles (`riddles/`), inscriptions (`inscriptions/`), royal names (`names/`) and save games (`saves/`), results (`out/`, not tracked).
* `external/` - third-party headers.
* `docs/slides/` - the presentation of the original game (the user's Bachelor's thesis), kept for history, not maintained.
* `Play`, `LevelEditor`, `ViewModel` - root launchers: start `bin/game`, `bin/editor` and `bin/model-viewer` from the repo root, for a release build tested on a system without dev tools. Plain `sh`, no make or dev-tool dependency.

## Libraries

The game and the tools share four static libraries, so every program builds the shared code the same way
([plan/solved/layered-build.md](plan/solved/layered-build.md)). Each sits on top of the ones above it in the table:
base, level, sim, render.

| Library | Sources | Rules | Linked by |
|---|---|---|---|
| `build/libbase.a` | `src/core/`: `timer` (the game clock), `logger`, `paths`, `worker_pool.h` | no GL; every library may include it | everything below |
| `build/liblevel.a` | `src/level/`: the level format (`level`), checker (`level_check`, `path_script` for levelcheck --script), generator (`level_gen`, its `LevelBuilder` in `level_builder.h` and `level_gen_*` by phase), `campaign`, the item and monster rows (`items`, `item_bag`, `loot`, `monster_kinds`), `tile_defs`, `player_stats`, `player_actions`, `progression`, the journal data, the pure layouts of the screens (`ui_layout`, `inventory_layout`, `skills_layout`, `status_list`, `message_log`, `page_turn`), `bindings`, `settings_ini`, `save_slots`, `scenario_script`, `gameplay_*.h` (the numbers, by topic) | no GL | game, editor, model viewer, level tools, balance sim, unit tests |
| `build/libsim.a` | `src/sim/`: the dungeon's rules (`dungeon.h`, `dungeon_*.cpp`, `dungeon_rules.h`, `sim_links.h`), `monster`, `monster_ai`, `monster_<locomotion>` (burrow, charge, climb, flight, leap), `monster_grab` (holds), `monster_reach` (the bite's reach), `monster_roll` (the dung ball's windup), `figures` (the toon scale), `player`, `particles` (the blood's motion), `md3_mesh` (the model files), `model_info` (the clips and frame 0 extents the sim reads), `render_config.h` (sizes in GL units) | no GL | game, editor, model viewer, balance sim, unit tests |
| `build/librender.a` | `src/render/`: `textures`, `font`, `animated_model`, `shader`, `lighting`, `ink`, `render_target`, `motion_fx`, `inscription_draw`, `ui_draw`; stb | GL allowed | game, editor, model viewer |

The directory is the library: the makefile picks up `src/<library>/*.cpp` and `*.h`, so a new file there is part of
it, and `make layers` reads the layer from the path. No library uses SDL or `Game()`, and a library file only includes
headers of its own library and of the ones under it. `make layers` (`tools/check_layers.sh`) checks this; `make tidy`
runs it first. To move a file into a library, move it into that directory (`git mv`) and fix the includes that name
it. Includes are relative to the including file (`"../level/level.h"`).

The sim (`src/sim/`) has no `Game()` and no screens; `make layers` checks that too (`tools/check_sim.sh`). It draws
nothing: `DungeonView` (`src/graphics/dungeon_view.h`, a friend of `Dungeon`) draws the dungeon in
`src/graphics/dungeon_render*.cpp` (in the game, not the library), with the state only the drawing keeps (contours,
item parts, the cells drawn) and the animations by the game clock; `GameState::dungeonView` owns it. Rules both
sides need go in `src/sim/dungeon_rules.h`. A monster is drawn by `DrawMonster` (`graphics/monster_draw.cpp`), the
player by `PlayerView`; both only read the sim's state ([plan/solved/sim-library.md](plan/solved/sim-library.md)).

## Adding a tile type, a monster or an item

The facts live in one table each; the compiler and the unit tests point at the rest.

* **Tile type:** a value in `DungeonTileType` (`src/level/level.h`) and a row in `TILES` (`src/level/tile_defs.cpp`:
  name, editor text, decor flags, glyph, editor icon; the icon drawn by `tools/editor/icons/make_icons.py`), and its
  glyph in the legend. A thing on the back wall is a wall type: add it to `isWallType` (`level.h`), read it from
  `Tile::fixture` and draw it in `DungeonView::drawFixture`. `-Wswitch` then flags `DungeonView::drawTileContent`; the checker (`level_check.cpp`), the draft
  map (`level/map_marks.h`: `thingMark`, a `MapMark` with its legend name and its sprite, [ui.md](ui.md#draft-map)) and the editor's hints (`describeCell`, `tools/editor/tile_info.cpp`) need a look by hand.
* **Monster:** a value in `MonsterTypeId` and a row in `KINDS` (`src/level/monster_kinds.cpp`): names, glyph,
  threat, stats, model, resistances, wading, poison, ranged attack, boss summons, levelgen weights. Only the fields that differ
  from the defaults in `MonsterKind`.
* **Weapon or potion:** a value in `ItemKind` at the end of its group, a free file id (saves go by it, not by position) and a row in
  `ITEMS` (`src/level/items.cpp`): file id, texts, and the weapon part (model, damage, reach, mix, growth, motion,
  sounds, missile, levelgen depth) or the potion part (gain, vessel model, texture, icon colour, journal note,
  levelgen weight, mimic loot). A potion's texture is a row in `POTIONS` (`tools/blender/models/items.py`), baked on
  its vessel. A weapon also needs its HUD icon (`tools/textures/hud_icons.py`, in `ItemKind` order).
* **Amulet type:** a value in `AmuletType`, its four `ItemKind` values and a row in `AMULET_TYPES`
  (`src/level/items.cpp`: model, amounts, texts); its bonus in `amuletBonus`.
* **Ring gem:** a value in `RingGem`, its four `ItemKind` values (copper, silver, gold, sky iron) and a row in `RING_GEMS`
  (`src/level/items.cpp`: names, slug, label, colour); its effect line in `ringEffect`, its bonus in `RingBonus` /
  `ringBonus` (`level/items.*`); models `tools/blender/models/rings.py`. A ring metal: `RingMetal`, the `RingEffect`
  columns, `METAL_WORDS` / `METAL_TEXTURES`, its file ids (`fileIdOf`, `itemFromFile`: a new metal's ids go after the
  last, as sky iron's `SKY_IRON_RING_FILE_ID`), `nextMetal`, `OFFER_XP_RING`, a band palette in `rings.py`.
* **Lock colour:** a row in `LOCK_COLOURS` (`level.h`) and its textures.
* Then the docs: the unit tests (`tests/unit/docs_test.cpp`) fail until `tools/editor/readme.md` and
  `docs/levels.md` list it.

## Assets

* `media/models/<category>/` - Quake 3 MD3 models (`characters`, `monsters`, `items`, `props`, `traps`, `decorations`, `ladders`, `mechanisms`). Monsters use `<name>.md3` (move), `<name>_att.md3` (attack), `<name>_die.md3` (death), optional `<name>_idle.md3` (idle) and, by kind, `_jump`, `_climb`, `_rise`, `_att_ranged` (`sim/model_info.h`).
* `media/textures/<category>/`, `media/textures/fonts/` - PNG textures (24-bit RGB; alpha is supported); same categories as `media/models/` plus `ui`, `dungeon`, `effects`.
* `media/sounds/` - WAV effects in the same `<category>/<name>_<clip>.wav` layout as `media/models/` and `media/textures/` (`_att`, `_att_ranged`, `_die`, `_jump`, `_wake`, `_roar`; weapons `_swing`, `_hit`; the game logs a warning for a monster sound the code can play but has no file, `playsSound` in `level/monster_kinds.cpp`), `mechanisms/`, `items/`, `water/`, `ui/`, `ambience/` (a zone's ambient loop, `ZoneLook::ambience` in `level/zone_look.h`, made by `tools/audio/zone_ambience.py`), and `music/`: `menu.ogg` (start menu, death and win screens) and `<zone slug>.ogg` (a zone's levels, slugs in `ZONE_INFO`, `level/campaign.h`; a copy or symlink shares a track), `soundtrack.ogg` (not tracked by git) where a file is missing or a level has no zone (`level/zone_music.h`, plan zone-music). The menu and zone tracks are CC0, fetched and loudness-normalised by `tools/audio/zone_music.py` (its table names each source; it writes `music/CREDITS.md`). `tools/audio/*.py` synthesize most of them, one script per monster or group (weapons, mechanisms, containers, water, ...).
* `content/levels/` - campaign levels, edited with the level editor.
* `content/riddles/` - riddle gate questions, one theme per text file. See [riddles.md](riddles.md).
* `content/names/` - the royal names of the name gates. See [names.md](names.md).
* `content/inscriptions/` - the wall inscriptions' texts. See [inscriptions.md](inscriptions.md).
* `content/texts/` - the player-facing prose (field notes, diary, lore). See [texts.md](texts.md).
* `saves/` - save games (not tracked by git).

Models are rebuilt procedurally with Blender Python scripts in `tools/blender/`; see [modeling.md](modeling.md).

## Code structure

* `game/game.cpp` - `main`: SDL, the GLUT window and callbacks. `Game()` (`state/game_state.h`) is the one
  `GameState`, created before the window.
* `state/assets.*` - `Assets`: everything loaded once and only read afterwards (textures, models, sounds, fonts,
  monster types). The rows come from the level library: `KINDS` (`level/monster_kinds.cpp`), `ITEMS` (`level/items.cpp`).
* `state/game_state.*` - `GameState`: the session. The player, the dungeon, the UI screens, camera, status lines
  (`ShowStatus`, `MessageLog`), save / load.
* `state/settings*` - `Settings`: the Options choices and key bindings, kept in `saves/settings.ini`
  ([settings.md](settings.md)).
* `sim/` - `Dungeon` (the level being played: map, monsters, mechanisms, decorations, save data), `MonsterType`
  (the rows plus the clips, shared), `Monster` (one monster on the level: position, health, AI state, clip playback),
  `Player` (with its `PlayerStats`, `level/player_stats.*`).
* `level/` - the GL-free level code shared with the tools (`level`, `level_check`, `level_gen`, `campaign`, the item
  and monster rows).
* `render/` - `AnimatedModel` (MD3; shared models take the playback as an argument), textures, fonts, lighting,
  toon ink, the sprint motion effects (`MotionFx`), the UI drawing (`ui_draw`).
* `graphics/` - the game's drawing: the dungeon (`DungeonView`, `dungeon_view.h`, `dungeon_render*.cpp`), `CharacterModel` (the clips, texture and
  sounds of a monster type or the player), `DrawMonster`, `PlayerView`, `Item`, `Trap`, particles, fire, the gameplay
  `Draw()` (`Update()` is in `state/game_loop.*`).
* `ui/`, `input/`, `test/` - screens, keyboard / mouse handling (through the key bindings, `level/bindings.h`), the
  scenario runner. UI look and layout conventions: [ui.md](ui.md).

## Where is X

Feature → files: [where-is-x.md](where-is-x.md), one row per feature. Ask `tools/where.py <term>...` for the rows that
match instead of reading it all.

## Tests

Scenario scripts in `tests/scenarios/` drive the game and take screenshots. `make test` runs all, `make test SCENARIO=path` runs one.
See [testing.md](testing.md). `make paths` plays levelcheck's path through every campaign level in the game
([levels.md](levels.md)).

## Code quality tools

* `make check`: builds, rewrites markdown links to plans that moved (`tools/plan.py links`), checks the plans' names
  and Status lines (`plan.py lint`) and runs the plan tool's tests, checks the docs' `#anchor` links and backticked
  code names (`tools/docs_check.py`: a name that is only an example goes in its `ALLOWED` list), tests `tools/job.sh`
  and the wait-loop hook (`tools/job_test.sh`, `tools/hooks/test_no_wait_loops.sh`), then `format`, the stability check of
  `format-check` on the changed files only (`tools/check_format.sh --changed`), then `tidy` and `unit` at the same time
  (output printed after both end, tidy first; a failure of either fails the run). Run it before each commit.
* `make format` formats the code. Formatting is the tool's job only: never align or wrap by hand, never think about
  it. `.clang-format` aligns nothing across lines (trailing comments, line continuations), so a diff shows only the
  lines that changed. A call too long for one line gets one argument per line and the `)` on its own line (the same
  for parameters). The whole-tree reformats for these are skipped by `git blame` ([version-control.md](version-control.md#blame)).
  The layout is settled: don't tune `.clang-format` further. Whitespace layout does not change how well a model
  works with code (arXiv 2508.13666); names and structure do (arXiv 2605.20049, 2605.26177). One argument per line
  costs about 3% more lines and no bytes (tabs). Effort on code quality goes to names (`identifier-naming` stays),
  small functions and clear file names, not layout.
* `make format-check` fails on a file `make format` would change, and on code clang-format does not leave stable (a
  file that flips between two layouts on every `make format`, often a long trailing comment that wraps onto a second
  line: move the comment above the line). Every file, in parallel (the load gate's job cap).
* `make complexity [THRESHOLD=N]`: the functions above a cognitive complexity (default 25), most complex first, as
  refactor candidates (`tools/code_map.py complexity`; a report, not a check; plan complexity-hotspots).
* `make tidy` (clang-tidy, `.clang-tidy`, and the layer checks): the sources changed since the merge base with
  master, and per changed header one source that includes it, its own `.cpp` if any (a header's warnings show in any
  of them; plan tidy-header-fanout). `make tidy-push`, before a push from master: the changes since `origin/master`
  with every source that includes a changed header (`TIDY_FANOUT=all`), so a warning a header change causes in a
  caller is caught too. `make tidy-all` checks every source (about 2.5 minutes). All
  run one clang-tidy per file through the load gate (`tools/load_gate.sh`, as the scenario games and the build:
  `JOBS`, `RESERVE_CORES`, `MIN_FREE_MB`, `MIN_SWAP_FREE_MB`, see [testing.md](testing.md)), check the project's
  headers too, not `external/`, and fail on any warning (`tools/tidy.sh`). `make tidy-fix` applies clang-tidy's fixes
  to every source. A clean source is cached by a hash of its text and headers (`TIDY_CACHE=0` turns it off), so a re-run
  tidies only what changed. `make check` logs the time of each stage (`tools/check_stages.sh`, read by
  `tools/friction_report.py`).

The tree is kept free of warnings: every warning `make tidy` prints is the change's own. Fix it; a false positive
gets a `NOLINTNEXTLINE(check): reason`. Naming rules (only these are checked; method and function
names are mixed in the code base and are not):

* classes, structs and enums: `CamelCase` (`Monster`, `PlayerStats`, `TexFilter`)
* variables and parameters: `camelBack`; local constants too (`const float dx`)
* global, `constexpr` and `static` local constants: `UPPER_CASE` (`MAX_MONSTERS`, `static const char KEY_CHARS[]`)
* enum values: `CamelCase` (`ModelState::Die`, `Locomotion::WalkJump`)
