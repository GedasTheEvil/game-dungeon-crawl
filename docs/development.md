# Development

Terms used in the game, the code and the docs: [glossary.md](glossary.md).

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

* `make` builds the game and every tool. Game: `./Play` (or `bin/game` from the repo root).
* `bin/game --level N|PATH|gen:SEED:D`: play a level by hand, menu skipped. `bin/game --play SCENARIO.txt`: play a
  scenario's setup (level, gear) by hand. `--no-sound` mutes either. See [testing.md](testing.md#play-by-hand).
* Level editor: `make editor`, `make run-editor`. See [tools/editor/readme.md](../tools/editor/readme.md).
* Model viewer: `make model-viewer`, `make run-model-viewer ARGS="media/models/monsters/anubis.md3"`.
* Level tools: `make level-tools`, then `bin/levelcheck content/levels/lvl*` (validate, rank by difficulty) and
  `bin/levelgen --seed 1 --difficulty 5 OUT` (random level), `bin/levelconvert FILE...` (old level files to the current
  format). See [levels.md](levels.md).
* Balance sim (plan [monster-balance](plan/monster-balance.md)): `bin/balance fight "giant rat" 30
  [--armor A] [--might M] [--hp P] [--difficulty D]` (one monster against a player of that level: tank in melee, kite
  with the bow, run past; HP lost as a share of max HP, seconds to the kill, potions drunk), `bin/balance table [--gear
  gen|play]` (every zone's monsters and boss against its player), `bin/balance curve` (the target campaign generated:
  XP and gear per level), `bin/balance today` (the same for `content/levels/`), `bin/balance tune` (each monster's
  HP and damage for its class's target in its first zone), `bin/balance home "giant rat" [--monster-hp HP]
  [--monster-damage D] [--minions N]` (one monster against the player of its first zone, with stats to try). The
  fights run in the unit tests'
  sim (`tests/unit/sim_world.h`) on an arena hall; `BALANCE_TRACE=1` prints each fight once a second.

## Project layout

* `src/` - game code: `core`, `graphics`, `entities`, `world`, `ui`, `input`, `state`, `test` (scenario runner).
* `tools/editor/saved/` - the level editor's work-in-progress levels (not tracked by git).
* `bin/` - the game, the tools and the unit test runner (not tracked by git).
* `build/` - object files and libraries (not tracked by git).
* `tools/` - level tools (`level/`), level editor (`editor/`), model viewer (`model-viewer/`), Blender model scripts (`blender/`), sound and texture generators (`audio/`, `textures/`), scenario runner script, the check scripts (`check_*.sh`, `tidy.sh`, `load_gate.sh`), plan bookkeeping (`plan.py`, [plans.md](plans.md)), `friction_report.py` (what the checks and rules cost agents, from the Claude Code transcripts).
* `tests/` - unit tests (`unit/`), scenario scripts (`scenarios/`), play scripts (`play/`), test levels (`levels/`), riddles (`riddles/`), inscriptions (`inscriptions/`) and save games (`saves/`), results (`out/`, not tracked).
* `external/` - third-party headers.

## Libraries

The game and the tools share four static libraries, so every program builds the shared code the same way
([plan/solved/layered-build.md](plan/solved/layered-build.md)). Each sits on top of the ones above it in the table:
base, level, sim, render.

| Library | Sources | Rules | Linked by |
|---|---|---|---|
| `build/libbase.a` | `core/timer` (the game clock), `core/logger`, `core/paths` | no GL; every library may include it | everything below |
| `build/liblevel.a` | `world/level`, `world/level_check`, `world/level_gen`, `world/campaign`, `world/items`, `world/item_bag`, `world/quick_potion`, `world/loot`, `world/progression`, `world/tile_defs`, `world/monster_kinds`, `world/decor_scatter`, `entities/player_stats`, ... (`LEVEL_LIB_SOURCES`) | no GL | game, editor, model viewer, level tools, unit tests |
| `build/libsim.a` | the dungeon's rules (`world/dungeon_*.cpp` but `dungeon_render*`), `entities/monster`, `entities/monster_ai`, `entities/monster_<locomotion>` (burrow, charge, climb, flight, leap), `entities/monster_grab` (holds), `entities/monster_reach` (the bite's reach), `entities/monster_roll` (the dung ball's windup), `entities/figures` (the toon scale), `entities/player`, `entities/particles` (the blood's motion), `entities/md3_mesh` (the model files), `entities/model_info` (the clips and frame 0 extents the sim reads) (`SIM_LIB_SOURCES`) | no GL | game, editor, model viewer, unit tests |
| `build/librender.a` | `graphics/textures`, `graphics/font`, `graphics/animated_model`, `graphics/shader`, `graphics/lighting`, `graphics/ink`, `graphics/render_target`, `graphics/motion_fx`, `ui/ui_draw`, stb | GL allowed | game, editor, model viewer |

No library uses SDL or `Game()`, and a library file only includes headers of its own library and of the ones under it.
`make layers` (`tools/check_layers.sh`) checks this; `make tidy` runs it first. To move a file into a library, add it to
`BASE_LIB_SOURCES`, `LEVEL_LIB_SOURCES`, `SIM_LIB_SOURCES` or `RENDER_LIB_SOURCES` in the makefile (a header-only file
to `LEVEL_LIB_HEADERS`).

The world and the entities (`src/world/dungeon*`, `src/entities/`) have no `Game()` and no screens; `make layers` checks
that too (`tools/check_sim.sh`). The dungeon draws itself only in `src/world/dungeon_render*.cpp` (in the game, not
the library): its other files include no GL and no graphics header but `render_config.h`. Rules both sides need go in
`src/world/dungeon_rules.h`. A monster is drawn by `DrawMonster` (`entities/monster_draw.cpp`), the player by
`PlayerView`; both only read the sim's state ([plan/solved/sim-library.md](plan/solved/sim-library.md)).

## Adding a tile type, a monster or an item

The facts live in one table each; the compiler and the unit tests point at the rest.

* **Tile type:** a value in `DungeonTileType` (`src/world/level.h`) and a row in `TILES` (`src/world/tile_defs.cpp`:
  name, editor text, decor flags, glyph, editor icon; the icon drawn by `tools/editor/icons/make_icons.py`), and its
  glyph in the legend. A thing on the back wall is a wall type: add it to `isWallType` (`level.h`), read it from
  `Tile::fixture` and draw it in `Dungeon::drawFixture`. `-Wswitch` then flags `Dungeon::drawTileContent`; the checker (`level_check.cpp`), the draft
  map (`ui/map_view.cpp`) and the editor's hints (`describeCell`, `tools/editor/tile_info.cpp`) need a look by hand.
* **Monster:** a value in `MonsterTypeId` and a row in `KINDS` (`src/world/monster_kinds.cpp`): names, glyph,
  threat, stats, model, resistances, wading, poison, ranged attack, boss summons, levelgen weights. Only the fields that differ
  from the defaults in `MonsterKind`.
* **Weapon or potion:** a value in `ItemKind` at the end of its group (save slots go by that order) and a row in
  `ITEMS` (`src/world/items.cpp`): file id, texts, and the weapon part (model, damage, reach, mix, growth, motion,
  sounds, missile, levelgen depth) or the potion part (gain, vessel model, texture, icon colour, journal note,
  levelgen weight, mimic loot). A potion's texture is a row in `POTIONS` (`tools/blender/models/items.py`), baked on
  its vessel. A weapon also needs its HUD icon (`tools/textures/hud_icons.py`, in `ItemKind` order).
* **Amulet type:** a value in `AmuletType`, its four `ItemKind` values and a row in `AMULET_TYPES`
  (`src/world/items.cpp`: model, amounts, texts); its bonus in `amuletBonus`.
* **Lock colour:** a row in `LOCK_COLOURS` (`level.h`) and its textures.
* Then the docs: the unit tests (`tests/unit/docs_test.cpp`) fail until `tools/editor/readme.md` and
  `docs/levels.md` list it.

## Assets

* `media/models/<category>/` - Quake 3 MD3 models (`characters`, `monsters`, `items`, `props`, `traps`, `decorations`, `ladders`, `mechanisms`). Monsters use `<name>.md3` (move), `<name>_att.md3` (attack), `<name>_die.md3` (death), optional `<name>_idle.md3` (idle) and, by kind, `_jump`, `_climb`, `_rise`, `_att_ranged` (`entities/model_info.h`).
* `media/textures/<category>/`, `media/textures/fonts/` - PNG textures (24-bit RGB; alpha is supported); same categories as `media/models/` plus `ui`, `dungeon`, `effects`.
* `media/sounds/` - WAV effects in the same `<category>/<name>_<clip>.wav` layout as `media/models/` and `media/textures/` (`_att`, `_att_ranged`, `_die`, `_jump`, `_wake`; weapons `_swing`, `_hit`), `mechanisms/`, `items/`, `water/`, `ui/`, and `music/soundtrack.ogg` (not tracked by git). `tools/audio/*.py` synthesize most of them, one script per monster or group (weapons, mechanisms, containers, water, ...).
* `content/levels/` - campaign levels, edited with the level editor.
* `content/riddles/` - riddle gate questions, one theme per text file. See [riddles.md](riddles.md).
* `content/names/` - the royal names of the name gates. See [names.md](names.md).
* `saves/` - save games (not tracked by git).

Models are rebuilt procedurally with Blender Python scripts in `tools/blender/`; see [modeling.md](modeling.md).

## Code structure

* `core/game.cpp` - `main`: SDL, the GLUT window and callbacks. `Game()` (`state/game_state.h`) is the one
  `GameState`, created before the window.
* `state/assets.*` - `Assets`: everything loaded once and only read afterwards (textures, models, sounds, fonts,
  monster types). The rows come from the level library: `KINDS` (`world/monster_kinds.cpp`), `ITEMS` (`world/items.cpp`).
* `state/game_state.*` - `GameState`: the session. The player, the dungeon, the UI screens, camera, status message
  (`ShowStatus`), save / load.
* `state/settings*` - `Settings`: the Options choices and key bindings, kept in `saves/settings.ini`
  ([settings.md](settings.md)).
* `entities/` - `CharacterModel` (the clips, texture and sounds of a monster type or the player), `MonsterType`
  (a `CharacterModel` plus stats, shared), `Monster` (one monster on the level: position, health, AI state, clip
  playback), `Player` (with its `PlayerStats`), `Item`, `Trap`.
* `graphics/` - `AnimatedModel` (MD3; shared models take the playback as an argument), textures, fonts, lighting,
  toon ink, the sprint motion effects (`MotionFx`), particles, the gameplay `Draw()` (`Update()` is in
  `state/game_loop.*`).
* `world/` - `Dungeon` (the level being played: map, monsters, mechanisms, decorations, rendering, save data) and the
  GL-free level code shared with the tools (`level`, `level_check`, `level_gen`, `campaign`).
* `ui/`, `input/`, `test/` - screens, keyboard / mouse handling (through the key bindings, `input/bindings.h`), the
  scenario runner. UI look and layout conventions: [ui.md](ui.md).

## Where is X

Feature → files, the first place to look (plan dev-friction-review). Keep it up to date in the commit that moves the
code. A feature's numbers (speeds, ranges, timings) sit in `core/gameplay_config.h`; terms in [glossary.md](glossary.md).

| Feature | Files |
|---|---|
| Monster type: stats, damage mix, resistances, abilities, threat | `world/monster_kinds.*` (one row per type) |
| Monster AI decisions (chase, flee, bite, lurk, ranged attack) | `entities/monster_ai.cpp` |
| Light shunners (the camel spider): lit cells, the stop at the light, the run for the dark | `Dungeon::Lit` (`world/dungeon_decor.cpp`), `Dungeon::walkerStops`, `wayOutOfLight` (`world/dungeon_walkers.cpp`) |
| Monster locomotion: state and update (`Flight`, `Leap`, `Burrow`, `Charge`, `Climb`) | `entities/monster_<flight\|leap\|burrow\|charge\|climb>.{h,cpp}`; a new one: a new pair, its member in `Monster` |
| Monster update on the level | `world/dungeon_monsters.cpp` (`UpdateMonsters`, hits, kills, spawns); per locomotion `world/dungeon_walkers.cpp` (walk, leap, flee, climbers' paths, with `world/climb_path.*`), `dungeon_flight.cpp`, `dungeon_burrow.cpp`, `dungeon_charge.cpp` |
| Monster drawing | `entities/monster_draw.*`; in-level effects `world/dungeon_render_effects.cpp` |
| Holds (a bite that grabs: the tell, the lunge, the death roll, the sting), the player held | `entities/monster_grab.{h,cpp}` (`Grab`), rules `HoldRules` in `world/monster_kinds.*`, `Dungeon::PlayerHeldBy` (`world/dungeon_monsters.cpp`), the locks in `world/dungeon_base.cpp` and `input/input.cpp` |
| Bite reach: the cone, the wind-up, a jump over a low monster dodges | `Monster::Reaches` / `UpdateBite` (`entities/monster_reach.cpp`), the bite in `world/dungeon_monsters.cpp` |
| Monster shots (ranged attack, spit) and arrow traps | `world/dungeon_arrows.cpp` |
| Dung balls (the giant scarab's and Khepri's roll): the windup, the kick, the roll, the fall, the hit | `entities/monster_roll.{h,cpp}` (`Roll`), `world/dungeon_balls.cpp` (`Dungeon::DungBall`), drawn by `Dungeon::drawBalls` (`world/dungeon_render_effects.cpp`); rules `RollRules` in `world/monster_kinds.*` |
| Boss fight (appear, summon, boss gates) | `world/dungeon_boss.cpp` |
| Player movement, jump, wading, ladders, traps underfoot | `world/dungeon_base.cpp`, `world/movement.h`, `entities/trap_hurt.h` |
| Player stats, XP, levels | `entities/player_stats.*`, `world/progression.*` |
| Items: weapons, potions, amulets (rows, rules) | `world/items.*`, `world/item_bag.*`; loot `world/loot.*` |
| Curses, poison, skills | `world/curses.*`, `world/poison.*`, `world/skills.*` |
| Damage types, fire, burning, fire arrows | `world/damage.h` (`Burning`), `Dungeon::updateBurning` (`world/dungeon_base.cpp`), `Dungeon::lightArrows` (`world/dungeon_interact.cpp`), drawn in `world/dungeon_render_effects.cpp`, `graphics/fire.*` |
| Run statistics, leaderboard (file, orders, scores) | `world/run_stats.*` (counters, in `PlayerStats`), `world/leaderboard.*`; screens `ui/win_entry.*`, `ui/records_view.*` |
| Interact: pick up treasure, use a gate (exit, teleport), travel, the order over a chest and a fixture, the prompt | `world/dungeon_interact.cpp` (`Dungeon::InteractVerb`, `world/interaction.h`), the prompt `ui/interact_prompt.*` |
| Treasure containers: kinds, shares by depth, stances, opening, drawing | `world/containers.h`, the look `scatterTreasures` (`world/decor_scatter.cpp`), opening `Dungeon::openContainer` (`world/dungeon_interact.cpp`), drawn by `Dungeon::DrawTreasureTile` (`world/dungeon_render.cpp`); models `tools/blender/models/containers.py`, the chest `items.py` |
| Keys, gates, levers, rock falls | `world/dungeon_mechanisms.cpp`, drawn in `world/dungeon_render_mechanisms.cpp` |
| Cut-scenes: the far gates shown, the camera's way, skip; the look | `world/dungeon_cutscene.cpp` (`CutShot`), drawn by `graphics/draw.cpp` (the camera, the HUD hidden) and `graphics/motion_fx.*` (tint, vignette, bars); skip in `input/input.cpp` |
| Dart traps | `world/dungeon_darts.*` (`DartTraps`) |
| Trapped chests, tomb curses | rolls `RollChestTrap`, `RollTrappedChestLoot` (`world/loot.*`), the traps `Dungeon::springChestTrap` (`world/dungeon_chest_traps.cpp`), the tell and the gas drawn in `world/dungeon_render.cpp`, numbers `CHEST_*` (`core/gameplay_config.h`) |
| Sunlight mirrors: the beam's way, turning a mirror, lighting a sun disc, the sun burn, the checker's search | `world/sun_beam.*` (`traceBeams`, `sunlitCells`), `world/dungeon_sun.cpp`, drawn in `world/dungeon_render_sun.cpp` (`SunBeam::ray` in `graphics/fire.*`); `checkSunBeams`, `Walker::turns` in `world/level_check.cpp` |
| Wall spikes and fire vents (rhythm traps): cycle, stab, jet, checker walk | `world/spike_cycle.h`, `world/dungeon_spikes.cpp` (the jet's hit: `Dungeon::updateTraps`, `Monster::StandInFire`), drawn in `world/dungeon_render_mechanisms.cpp`; `wallSpikesCrossable` in `world/level_check.cpp` |
| Expedition pages: pickup, texts, the journal page, placement | `Dungeon::updateMechanisms` (`world/dungeon_mechanisms.cpp`), drawn by `Dungeon::drawPageTile` (`world/dungeon_render_mechanisms.cpp`), the skeleton in `scatterProps` (`world/decor_scatter.cpp`), texts `content/texts/expedition.txt` (`expeditionPageText`, `world/journal_text.cpp`), `JournalScreen::DrawExpedition`, texture `tools/textures/expedition_page.py`; [levels.md](levels.md) lists the levels |
| Inscriptions: texts, reading, the plates and the hieroglyph sheet | `world/inscriptions.*` (data `content/inscriptions/*.txt`, [inscriptions.md](inscriptions.md)), `Dungeon::readInscription` in `world/dungeon_interact.cpp`, the layout `world/inscription_layout.*`, drawn by `graphics/inscription_draw.*` from `world/dungeon_render_decor.cpp` and `JournalScreen::DrawInscription`, plates and sheet baked by `tools/textures/inscriptions.py` |
| Decoration (props, decals, torches) | `world/decor*.h`, `world/decor_scatter.cpp`, `world/dungeon_decor.cpp`, `world/dungeon_render_decor.cpp`; placed by the decor and decal layers (`Dressing`, `world/level.h`), checked by `checkDressing` (`world/level_check.cpp`), painted with `tools/editor/dressing_brush.h`; the decoy drawn by `Dungeon::drawDecoys` (`world/dungeon_render_effects.cpp`) |
| Tile types, level file format, level grid, wall layer (fixtures) | `world/tile_defs.*`, `world/level.*` (`Fixture`, `isWallType`); drawn by `Dungeon::drawFixture` (`world/dungeon_render.cpp`) |
| Name gates, name cartouches, the royal names (per run) | `world/names.*` (data `content/names/*.txt`, [names.md](names.md)), `Dungeon::SpeakName`, `inscriptionPage` in `world/dungeon_interact.cpp`, `Riddle::AskName` (`ui/riddle.cpp`), the pulse `Dungeon::nameGateAlpha` (`world/dungeon_render.cpp`), `checkNameGates` in `world/level_check.cpp` |
| Level checker, generator, campaign order | `world/level_check.*`, `world/level_gen.*`, `world/campaign.*` |
| Save / load | `state/game_state.cpp`, `state/save_slots.*`, `world/dungeon_io.cpp` (the dungeon's part) |
| Autosave, death screen, Continue | `GameState::Autosave` / `CountDeath` (`state/game_state.cpp`), the safe moments `AutosaveMoment` (`world/world_events.h`) and `Dungeon::PlayerInDanger` (`world/dungeon_monsters.cpp`), `ui/death_screen.*`, the Load list and Continue in `ui/menu.cpp` |
| Models, textures, sounds loaded once | `state/assets.cpp` |
| Screens: inventory, journal, map, Heka, menu, riddle | `ui/inventory*`, `ui/journal_view.*` (data `world/journal.*`, its words: creature moves, field notes, diary `world/journal_text.*`, the prose in `content/texts/`; the new game's prologue `GameState::NewGame`), `ui/map_view.*`, `ui/skills_*`, `ui/menu.*`, `ui/riddle.*` |
| Player-facing prose: field notes, diary, expedition pages, creature moves, monster notes, skill and item lore | `content/texts/*.txt` ([texts.md](texts.md)), read by `gameText` (`world/texts.*`); the keys asked in `world/journal_text.cpp`, `world/skills.cpp`, `world/items.cpp` |
| HUD | `ui/player_hud*`, `ui/boss_bar.*`, `ui/status_box.*`, `ui/level_gem.*` |
| Keys and mouse | `input/input.cpp`, `input/bindings.*`, `input/input_actions.h` |
| Game loop, frame drawing | `state/game_loop.cpp`, `graphics/draw.cpp` |
| Blood splashes | `entities/particles.*` (motion), `graphics/particles_draw.*` |
| Scenario commands | `test/scenario*.cpp` |
| Plans: status, rename, solve | `tools/plan.py` ([plans.md](plans.md)) |

## Tests

Scenario scripts in `tests/scenarios/` drive the game and take screenshots. `make test` runs all, `make test SCENARIO=path` runs one.
See [testing.md](testing.md). `make paths` plays levelcheck's path through every campaign level in the game
([levels.md](levels.md)).

## Code quality tools

* `make check`: builds, rewrites markdown links to plans that moved (`tools/plan.py links`), then `format`,
  `format-check`, `tidy` and `unit`, in order. Run it before each commit.
* `make format` formats the code. Formatting is the tool's job only: never align or wrap by hand, never think about
  it. `.clang-format` aligns nothing across lines (trailing comments, line continuations), so a diff shows only the
  lines that changed. A call too long for one line gets one argument per line and the `)` on its own line (the same
  for parameters). The whole-tree reformats for these are in `.git-blame-ignore-revs`
  (`git config blame.ignoreRevsFile .git-blame-ignore-revs`).
* `make format-check` fails on a file `make format` would change, and on code clang-format does not leave stable (a
  file that flips between two layouts on every `make format`, often a long trailing comment that wraps onto a second
  line: move the comment above the line).
* `make tidy` (clang-tidy, `.clang-tidy`, and the layer checks): the sources changed since the merge base with
  master, and the ones that include a changed header. `make tidy-all` checks every source (about 2.5 minutes). Both
  run one clang-tidy per file through the load gate (`tools/load_gate.sh`, as the scenario games and the build:
  `JOBS`, `RESERVE_CORES`, `MIN_FREE_MB`, `MIN_SWAP_FREE_MB`, see [testing.md](testing.md)), check the project's
  headers too, not `external/`, and fail on any warning (`tools/tidy.sh`). `make tidy-fix` applies clang-tidy's fixes
  to every source.

The tree is kept free of warnings: every warning `make tidy` prints is the change's own. Fix it; a false positive
gets a `NOLINTNEXTLINE(check): reason`. Naming rules (only these are checked; method and function
names are mixed in the code base and are not):

* classes, structs and enums: `CamelCase` (`Monster`, `PlayerStats`, `TexFilter`)
* variables and parameters: `camelBack`; local constants too (`const float dx`)
* global, `constexpr` and `static` local constants: `UPPER_CASE` (`MAX_MONSTERS`, `static const char KEY_CHARS[]`)
* enum values: `CamelCase` (`ModelState::Die`, `Locomotion::WalkJump`)
