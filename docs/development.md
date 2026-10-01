# Development

## Requirements

* Make
* GCC
* OpenGL
* GLUT
* SDL, SDL Mixer (the game's audio; the tools do not need them)

PNG loading uses the bundled single-header [stb_image](https://github.com/nothings/stb) (`external/stb/stb_image.h`), no extra library needed.

## Build and run

Asset paths are relative to the repo root: run every program from there.

* `make` builds the game and every tool. Game: `./Play` (or `./game` from the repo root).
* Level editor: `make editor`, `make run-editor`. Runs from the repo root, see [tools/editor/readme.md](../tools/editor/readme.md).
* Model viewer: `make model-viewer`, `make run-model-viewer ARGS="models/monsters/anubis.md3"`.
* Level tools: `make level-tools`, then `./levelcheck levels/lvl*` (validate, rank by difficulty) and
  `./levelgen --seed 1 --difficulty 5 OUT` (random level). See [levels.md](levels.md).

## Project layout

* `src/` - game code: `core`, `graphics`, `entities`, `world`, `ui`, `input`, `state`, `test` (scenario runner).
* `tools/editor/saved/` - the level editor's work-in-progress levels (not tracked by git).
* `build/` - object files and the editor / model viewer binaries (not tracked by git).
* `tools/` - level tools (`level/`), level editor (`editor/`), model viewer (`model-viewer/`), Blender model scripts (`blender/`), sound and texture generators (`audio/`, `textures/`), scenario runner script.
* `tests/` - scenario scripts (`scenarios/`), test levels (`levels/`), test riddles (`riddles/`), results (`out/`, not tracked).
* `external/` - third-party headers.

## Libraries

The game and the tools share two static libraries, so every program builds the shared code the same way
([plan/solved/layered-build.md](plan/solved/layered-build.md)):

| Library | Sources | Rules | Linked by |
|---|---|---|---|
| `build/liblevel.a` | `world/level`, `world/level_check`, `world/level_gen`, `world/campaign`, `world/items`, `world/item_bag`, `world/quick_potion`, `world/loot`, `world/progression`, `world/tile_defs`, `world/monster_kinds` | no GL | game, editor, levelcheck, levelgen, unit tests |
| `build/librender.a` | `core/logger`, `core/timer`, `graphics/textures`, `graphics/font`, `graphics/animated_model`, `graphics/shader`, `graphics/lighting`, `graphics/ink`, `ui/ui_draw`, stb | GL allowed | game, editor, model viewer |

Neither library uses SDL or `Game()`, and a library file only includes headers of its own library. `make layers`
(`tools/check_layers.sh`) checks this; `make tidy` runs it first. To move a file into a library, add it to
`LEVEL_LIB_SOURCES` or `RENDER_LIB_SOURCES` in the makefile (a header-only file to `LEVEL_LIB_HEADERS`).

## Adding a tile type or a monster

The facts live in one table each; the compiler and the unit tests point at the rest.

* **Tile type:** a value in `DungeonTileType` (`src/world/level.h`) and a row in `TILES` (`src/world/tile_defs.cpp`:
  name, editor text, decor flags), its glyph in `tileGlyph` and the legend. `-Wswitch` then flags
  `Dungeon::drawTileContent`; the checker (`level_check.cpp`), the draft map (`ui/map_view.cpp`) and the editor's
  icon list (`tools/editor/tile_info.cpp`, `icons/make_icons.py`) need a look by hand.
* **Monster:** a value in `MonsterTypeId`, a row in `KINDS` (`src/world/monster_kinds.cpp`: label, glyph, threat, boss),
  a row in `MONSTER_DEFS` (`src/state/assets.cpp`: stats, model), and for a boss one in `BOSS_DEFS`.
* **Lock colour:** a row in `LOCK_COLOURS` (`level.h`) and its textures.
* Then the docs: the unit tests (`tests/unit/docs_test.cpp`) fail until `tools/editor/readme.md` and
  `docs/levels.md` list it.

## Assets

* `models/<category>/` - Quake 3 MD3 models (`characters`, `monsters`, `items`, `props`, `traps`, `decorations`, `ladders`, `mechanisms`). Monsters use `<name>.md3` (move), `<name>_att.md3` (attack), `<name>_die.md3` (death), optional `<name>_idle.md3` (idle).
* `textures/<category>/`, `fonts/` - PNG textures (24-bit RGB; alpha is supported); same categories as `models/` plus `ui`, `dungeon`, `effects`.
* `sounds/` - WAV effects in the same `<category>/<name>_<clip>.wav` layout as `models/` and `textures/` (`_att`, `_die`, `_jump`, `_wake`; weapons `_swing`, `_hit`), `mechanisms/`, `items/`, and `music/soundtrack.ogg` (not tracked by git). `tools/audio/*.py` synthesize the jump, rat, bat, mimic, weapon, arrow, key, gate, lever and rock sounds.
* `levels/` - campaign levels, edited with the level editor.
* `riddles/` - riddle gate questions, one theme per text file. See [riddles.md](riddles.md).
* `saves/` - save games (not tracked by git).

Models are rebuilt procedurally with Blender Python scripts in `tools/blender/`; see [remodeling.md](remodeling.md).

## Code structure

* `core/game.cpp` - `main`: SDL, the GLUT window and callbacks. `Game()` (`state/game_state.h`) is the one
  `GameState`, created before the window.
* `state/assets.*` - `Assets`: everything loaded once and only read afterwards (textures, models, sounds, fonts,
  monster types). Monsters and items are rows of the `MONSTER_DEFS` / `ITEM_DEFS` tables in `assets.cpp`.
* `state/game_state.*` - `GameState`: the session. The player, the dungeon, the UI screens, camera, status message
  (`ShowStatus`), save / load.
* `entities/` - `CharacterModel` (the clips, texture and sounds of a monster type or the player), `MonsterType`
  (a `CharacterModel` plus stats, shared), `Monster` (one monster on the level: position, health, AI state, clip
  playback), `Player` (with its `PlayerStats`), `Item`, `Trap`.
* `graphics/` - `AnimatedModel` (MD3; shared models take the playback as an argument), textures, fonts, lighting,
  toon ink, particles, the gameplay `Draw()` / `Update()`.
* `world/` - `Dungeon` (the level being played: map, monsters, mechanisms, decorations, rendering, save data) and the
  GL-free level code shared with the tools (`level`, `level_check`, `level_gen`, `campaign`).
* `ui/`, `input/`, `test/` - screens, keyboard / mouse handling, the scenario runner. UI look and layout conventions:
  [ui.md](ui.md).

## Tests

Scenario scripts in `tests/scenarios/` drive the game and take screenshots. `make test` runs all, `make test SCENARIO=path` runs one.
See [testing.md](testing.md).

## Code quality tools

* Format source files: `make format`
* Check formatting without changing files: `make format-check`. It also fails on code clang-format does not leave
  stable (a file that flips between two layouts on every `make format`, often a long trailing comment that wraps
  onto a second line: move the comment above the line).
* Run static analysis (clang-tidy): `make tidy`

`clang-tidy` uses the project configuration from `.clang-tidy`. `make tidy` runs one clang-tidy per file in parallel
(`TIDY_JOBS`, default `nproc - 4`, at least 1, under a minute) and checks the project's headers too, not `external/`.
The build and `make tidy` are expected to print no warnings. Naming rules (only these are checked; method and function
names are mixed in the code base and are not):

* classes, structs and enums: `CamelCase` (`Monster`, `PlayerStats`, `TexFilter`)
* variables and parameters: `camelBack`; local constants too (`const float dx`)
* global, `constexpr` and `static` local constants: `UPPER_CASE` (`MAX_MONSTERS`, `static const char KEY_CHARS[]`)
* enum values: `CamelCase` (`ModelState::Die`, `Locomotion::WalkJump`); enums take `: unsigned char` (`performance-enum-size`)
