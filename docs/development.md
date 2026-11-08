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

## Project layout

* `src/` - one directory per library, then the game ([Libraries](#libraries), plan src-dirs-by-layer): `core` (base),
  `level`, `sim`, `render`; the game's own `game` (`main`, sound), `graphics` (the game's drawing), `ui`, `input`,
  `state`, `test` (scenario runner).
* `tools/editor/saved/` - the level editor's work-in-progress levels (not tracked by git).
* `bin/` - the game, the tools and the unit test runner (not tracked by git).
* `build/` - object files and libraries (not tracked by git).
* `tools/` - level tools (`level/`), level editor (`editor/`), model viewer (`model-viewer/`), Blender model scripts (`blender/`), sound and texture generators (`audio/`, `textures/`), the balance sim (`balance/`), scenario runner script, `profile_game.sh` ([performance-testing.md](performance-testing.md)), the check scripts (`check_*.sh`, `tidy.sh`, `load_gate.sh`), plan bookkeeping (`plan.py`, its tests `plan_test.py`, [plans.md](plans.md)), `friction_report.py` (what the checks and rules cost agents, from the Claude Code transcripts).
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
| `build/liblevel.a` | `src/level/`: the level format (`level`), checker (`level_check`, `path_script` for levelcheck --script), generator (`level_gen`, its `LevelBuilder` in `level_builder.h` and `level_gen_*` by phase), `campaign`, the item and monster rows (`items`, `item_bag`, `loot`, `monster_kinds`), `tile_defs`, `player_stats`, `progression`, the journal data, the pure layouts of the screens (`ui_layout`, `inventory_layout`, `skills_layout`, `status_list`, `message_log`, `page_turn`), `bindings`, `settings_ini`, `save_slots`, `scenario_script`, `gameplay_config.h` | no GL | game, editor, model viewer, level tools, balance sim, unit tests |
| `build/libsim.a` | `src/sim/`: the dungeon's rules (`dungeon.h`, `dungeon_*.cpp`, `dungeon_rules.h`, `sim_links.h`), `monster`, `monster_ai`, `monster_<locomotion>` (burrow, charge, climb, flight, leap), `monster_grab` (holds), `monster_reach` (the bite's reach), `monster_roll` (the dung ball's windup), `figures` (the toon scale), `player`, `particles` (the blood's motion), `md3_mesh` (the model files), `model_info` (the clips and frame 0 extents the sim reads), `render_config.h` (sizes in GL units) | no GL | game, editor, model viewer, balance sim, unit tests |
| `build/librender.a` | `src/render/`: `textures`, `font`, `animated_model`, `shader`, `lighting`, `ink`, `render_target`, `motion_fx`, `inscription_draw`, `ui_draw`; stb | GL allowed | game, editor, model viewer |

The directory is the library: the makefile picks up `src/<library>/*.cpp` and `*.h`, so a new file there is part of
it, and `make layers` reads the layer from the path. No library uses SDL or `Game()`, and a library file only includes
headers of its own library and of the ones under it. `make layers` (`tools/check_layers.sh`) checks this; `make tidy`
runs it first. To move a file into a library, move it into that directory (`git mv`) and fix the includes that name
it. Includes are relative to the including file (`"../level/level.h"`).

The sim (`src/sim/`) has no `Game()` and no screens; `make layers` checks that too (`tools/check_sim.sh`). It draws
nothing: the dungeon draws itself in `src/graphics/dungeon_render*.cpp` (in the game, not the library). Rules both
sides need go in `src/sim/dungeon_rules.h`. A monster is drawn by `DrawMonster` (`graphics/monster_draw.cpp`), the
player by `PlayerView`; both only read the sim's state ([plan/solved/sim-library.md](plan/solved/sim-library.md)).

## Adding a tile type, a monster or an item

The facts live in one table each; the compiler and the unit tests point at the rest.

* **Tile type:** a value in `DungeonTileType` (`src/level/level.h`) and a row in `TILES` (`src/level/tile_defs.cpp`:
  name, editor text, decor flags, glyph, editor icon; the icon drawn by `tools/editor/icons/make_icons.py`), and its
  glyph in the legend. A thing on the back wall is a wall type: add it to `isWallType` (`level.h`), read it from
  `Tile::fixture` and draw it in `Dungeon::drawFixture`. `-Wswitch` then flags `Dungeon::drawTileContent`; the checker (`level_check.cpp`), the draft
  map (`ui/map_view.cpp`: `markOf`, a `MapMark` with its legend name and symbol) and the editor's hints (`describeCell`, `tools/editor/tile_info.cpp`) need a look by hand.
* **Monster:** a value in `MonsterTypeId` and a row in `KINDS` (`src/level/monster_kinds.cpp`): names, glyph,
  threat, stats, model, resistances, wading, poison, ranged attack, boss summons, levelgen weights. Only the fields that differ
  from the defaults in `MonsterKind`.
* **Weapon or potion:** a value in `ItemKind` at the end of its group (save slots go by that order) and a row in
  `ITEMS` (`src/level/items.cpp`): file id, texts, and the weapon part (model, damage, reach, mix, growth, motion,
  sounds, missile, levelgen depth) or the potion part (gain, vessel model, texture, icon colour, journal note,
  levelgen weight, mimic loot). A potion's texture is a row in `POTIONS` (`tools/blender/models/items.py`), baked on
  its vessel. A weapon also needs its HUD icon (`tools/textures/hud_icons.py`, in `ItemKind` order).
* **Amulet type:** a value in `AmuletType`, its four `ItemKind` values and a row in `AMULET_TYPES`
  (`src/level/items.cpp`: model, amounts, texts); its bonus in `amuletBonus`.
* **Ring gem:** a value in `RingGem`, its three `ItemKind` values (copper, silver, gold) and a row in `RING_GEMS`
  (`src/level/items.cpp`: names, slug, label, colour); its effect line in `ringEffect`, its bonus in `RingBonus` /
  `ringBonus` (`level/items.*`); models `tools/blender/models/rings.py`.
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
* `graphics/` - the game's drawing: the dungeon (`dungeon_render*.cpp`), `CharacterModel` (the clips, texture and
  sounds of a monster type or the player), `DrawMonster`, `PlayerView`, `Item`, `Trap`, particles, fire, the gameplay
  `Draw()` (`Update()` is in `state/game_loop.*`).
* `ui/`, `input/`, `test/` - screens, keyboard / mouse handling (through the key bindings, `level/bindings.h`), the
  scenario runner. UI look and layout conventions: [ui.md](ui.md).

## Where is X

Feature → files, the first place to look (plan dev-friction-review). Keep it up to date in the commit that moves the
code. A feature's numbers (speeds, ranges, timings) sit in `level/gameplay_config.h`; terms in [glossary.md](glossary.md).

| Feature | Files |
|---|---|
| Monster type: stats, damage mix, resistances, abilities, threat | `level/monster_kinds.*` (one row per type) |
| Monster AI decisions (chase, flee, bite, lurk, ranged attack) | `sim/monster_ai.cpp` |
| Light shunners (the camel spider, the tiny spider): lit cells, the stop at the light, the run for the dark, no climb path through the light | `Dungeon::Lit` (`sim/dungeon_decor.cpp`), `Dungeon::walkerStops`, `wayOutOfLight`, `followPath` (`ClimbAbility::avoids`), `climbDown` (`sim/dungeon_walkers.cpp`) |
| Terrain affinity (native and hostile terrains): the table, the combining rules, the cell's terrains, the factors on a monster, the avoidance, the checker and levelgen | `level/terrain.h`, `TERRAIN_ROWS` (`level/gameplay_config.h`), `MonsterKind::native` / `hostile` (`level/monster_kinds.*`), `Dungeon::TerrainsAt`, `wayOutOfTerrain`, `terrainShuns` (`sim/dungeon_walkers.cpp`), `Monster::SetTerrain`, `WalkStep`, `HitDamage` (`sim/monster*.cpp`), `spawnTerrains`, `checkSpawnTerrains` (`level/level_check.cpp`), `LevelBuilder::hostileCell`, `lightsFoe` (`level/level_gen_*.cpp`) |
| Monsters in groups (the tiny spiders): levelgen's group placement | `MonsterKind::group` (`level/monster_kinds.h`), `placeGroup` (`level/level_gen_content.cpp`) |
| Monster locomotion: state and update (`Flight`, `Leap`, `Burrow`, `Charge`, `Climb`) | `sim/monster_<flight\|leap\|burrow\|charge\|climb>.{h,cpp}`; a new one: a new pair, its member in `Monster` |
| Monster update on the level | `sim/dungeon_monsters.cpp` (`UpdateMonsters`, hits, kills, spawns); per locomotion `sim/dungeon_walkers.cpp` (walk, leap, flee, climbers' paths, with `level/climb_path.*`), `dungeon_flight.cpp`, `dungeon_burrow.cpp`, `dungeon_charge.cpp` |
| Monster drawing | `graphics/monster_draw.*`; in-level effects `graphics/dungeon_render_effects.cpp` |
| Holds (a bite that grabs: the tell, the lunge, the death roll, the sting), the player held | `sim/monster_grab.{h,cpp}` (`Grab`), rules `HoldRules` in `level/monster_kinds.*`, `Dungeon::PlayerHeldBy` (`sim/dungeon_monsters.cpp`), the locks in `sim/dungeon_base.cpp` and `input/input.cpp` |
| Bite reach: the cone, the wind-up, a jump over a low monster dodges | `Monster::Reaches` / `UpdateBite` (`sim/monster_reach.cpp`), the bite in `sim/dungeon_monsters.cpp` |
| Monster shots (ranged attack, spit) and arrow traps | `sim/dungeon_arrows.cpp` |
| The player's ranged weapons: shots and their recovery, the minimum range, the HUD's pips | `WeaponDef::shots`, `minRange` (`level/items.*`), `PlayerStats::Shots` / `ShotRecoveryPace` (amulets and the Quiver of Neith skill), `ItemBag::ShotsLeft` / `RecoverShots` (`level/item_bag.*`, ticked in `Dungeon::Update`), `Dungeon::ShotRefused` / `ShootWeapon` (`sim/dungeon_arrows.cpp`), `drawShots` (`ui/player_hud.cpp`); numbers `SHOT_*` (`level/gameplay_config.h`) |
| Dung balls (the giant scarab's and Khepri's roll): the windup, the kick, the roll, the fall, the hit | `sim/monster_roll.{h,cpp}` (`Roll`), `sim/dungeon_balls.cpp` (`Dungeon::DungBall`), drawn by `Dungeon::drawBalls` (`graphics/dungeon_render_effects.cpp`); rules `RollRules` in `level/monster_kinds.*` |
| Boss fight (appear, summon, boss gates) | `sim/dungeon_boss.cpp` |
| Player movement, jump, wading, ladders, traps underfoot | `sim/dungeon_base.cpp`, `level/movement.h`, `level/trap_hurt.h` |
| Crouch: the down key, the crouched body and hitbox, the pose, the ducked attacks | `Dungeon::updateCrouch`, `roomToStand`, `playerBodyHeight` (`sim/dungeon_base.cpp`), `Player::crouched`, `UpdateCrouchPose`, `Covers` (`sim/player.*`), `AttackHeight` (`level/monster_kinds.h`, read by `Monster::Reaches`), the clips in `tools/blender/models/archeologist.py` |
| Player stats, XP, levels | `level/player_stats.*`, `level/progression.*` |
| Items: weapons, potions, amulets, rings (rows, rules) | `level/items.*`, `level/item_bag.*`; loot `level/loot.*` |
| Rings: the effects where they act | `RingBonus` (`level/items.*`) read by `PlayerStats` (`level/player_stats.*`: regeneration, stamina, walk factor, reach), `Poison::Advance` (feldspar), `Monster::TakeWeaponHit` (amethyst); worn per hand in `ItemBag`; the gem drawn see-through by `Item::DrawPart` (`graphics/item.*`, after the opaque scene: `Dungeon::drawItemParts`); models `tools/blender/models/rings.py` |
| Curses, poison, skills | `level/curses.*`, `level/poison.*`, `level/skills.*` |
| Skills' effects beyond a flag: the stat skills, Wisdom of Seshat (sealed rooms on the map, riddle hint, royal name), Disarm trap (its red contour) | `PlayerStats::Learn`, `CurrentMaxHP`, `WalkFactor` (`level/player_stats.*`); `sim/dungeon_seshat.cpp` (rules), `graphics/dungeon_render_outlines.cpp` (the contour); the hint and the name `ui/riddle.cpp`, `Dungeon::useGate` (`sim/dungeon_interact.cpp`); `Dungeon::springChestTrap`, `coffinCurse` (`sim/dungeon_chest_traps.cpp`) |
| Shrines of the gods (Isis, Montu, Ptah, Amun, Serket): the prayer, the blessings, their HUD cards and beams | `God`, `GODS`, `shrineGod` (`level/shrines.h`); `PlayerStats::Pray`, `Blessed`, `AdvanceBlessings`, `LayCurse` (`level/player_stats.*`), Serket's ward in `Player::Poison`; the verb in `sim/dungeon_interact.cpp`; cards in `statusList` (`level/status_list.cpp`), beams `SunBeam::PRAYER_LIGHT` (`graphics/fire.h`); props `DECOR_SHRINE_*` (`level/decor.h`), models `tools/blender/models/decor.py` (`naos` and a statue per god), sounds `tools/audio/shrine_sounds.py` |
| Damage types, fire, burning, fire arrows | `level/damage.h` (`Burning`), `Dungeon::updateBurning` (`sim/dungeon_base.cpp`), `Dungeon::lightArrows` (`sim/dungeon_interact.cpp`), drawn in `graphics/dungeon_render_effects.cpp`, `graphics/fire.*` |
| Run statistics, leaderboard (file, orders, scores) | `level/run_stats.*` (counters, in `PlayerStats`), `level/leaderboard.*`; screens `ui/win_entry.*`, `ui/records_view.*` |
| Interact: pick up treasure, use a gate (exit, teleport), travel, the order over a chest and a fixture, the prompt | `sim/dungeon_interact.cpp` (`Dungeon::InteractVerb`, `level/interaction.h`), the prompt `ui/interact_prompt.*` |
| Treasure containers: kinds, shares by depth, stances, placed looks, opening, drawing | `level/containers.h`, the look `scatterTreasures` and `containerStances` (`level/decor_scatter.cpp`), placed by the looks layer (`Dressing::container`, `level/level.h`; `checkPlacedContainer`, `level/level_check_dressing.cpp`; editor `paintLook`, `tools/editor/dressing_brush.h`), opening `Dungeon::openContainer` (`sim/dungeon_interact.cpp`), drawn by `Dungeon::DrawTreasureTile` (`graphics/dungeon_render.cpp`); models `tools/blender/models/containers.py`, the chest `items.py` |
| Keys, gates, levers, rock falls | `sim/dungeon_mechanisms.cpp`, drawn in `graphics/dungeon_render_mechanisms.cpp` |
| The Stargate chamber: the dial, the chevrons, the reward, the drawing | `sim/dungeon_stargate.cpp` (`Dungeon::startDial`, `updateDial`, `ChevronGlow`), drawn by `Dungeon::drawStargate` (`graphics/dungeon_render_decor.cpp`), props `DECOR_STARGATE`, `DECOR_DIAL` (`level/decor.h`); models `tools/blender/models/decor.py`, sounds `tools/audio/ring_sounds.py`; placed by levelgen `--stargate` (`placeTeleporterRoom`) |
| Ring platforms (ring transporter): the rings' travel, the flash, the drawing, the checker's rules | `RING_PLATFORM` (`level/level.h`), `Dungeon::startTravel` / `updateTravel` / `RingDrop` (`sim/dungeon_interact.cpp`), `Dungeon::drawRingPlatformTile` (`graphics/dungeon_render_mechanisms.cpp`), the flash in `graphics/draw.cpp`, `checkTeleporters` (`level/level_check_mechanisms.cpp`); models `tools/blender/models/mechanism.py`, sounds `tools/audio/ring_sounds.py` |
| Cut-scenes: the far gates shown, the camera's way, skip; the look | `sim/dungeon_cutscene.cpp` (the camera's way in a `CutShot`, `sim/dungeon.h`), drawn by `graphics/draw.cpp` (the camera, the HUD hidden) and `render/motion_fx.*` (tint, vignette, bars); skip in `input/input.cpp` |
| Dart traps | `sim/dungeon_darts.*` (`DartTraps`) |
| Trapped chests, tomb curses | rolls `RollChestTrap`, `RollTrappedChestLoot` (`level/loot.*`), the traps `Dungeon::springChestTrap` (`sim/dungeon_chest_traps.cpp`), the tell and the gas drawn in `graphics/dungeon_render.cpp`, numbers `CHEST_*` (`level/gameplay_config.h`) |
| Sunlight mirrors: the beam's way, turning a mirror, lighting a sun disc, the sun burn, the checker's search | `level/sun_beam.*` (`traceBeams`, `sunlitCells`), `sim/dungeon_sun.cpp`, drawn in `graphics/dungeon_render_sun.cpp` (`SunBeam::ray` in `graphics/fire.*`); `checkSunBeams` in `level/level_check_mechanisms.cpp`, `Walker::turns` in `level/level_walker.cpp` |
| Sandstorm halls: the gusts' push, the haze and the muffle, the arrows' drift, the blown sand and the pillars, the checker's runs | `sim/dungeon_gusts.cpp`, the cycle in `level/spike_cycle.h`, the drift in `sim/dungeon_arrows.cpp`, drawn by `Dungeon::drawGusts` / `drawWindbreak` (`graphics/dungeon_render_sand.cpp`, `Grit::gust` in `graphics/fire.*`), the fog in `graphics/draw.cpp`, the muffle in `GameState::ApplyWorldEvents`; `checkSandGusts`, `gustsCrossable` in `level/level_check_mechanisms.cpp`; sounds `tools/audio/sand_sounds.py` |
| Wall spikes and fire vents (rhythm traps): cycle, stab, jet, checker walk | `level/spike_cycle.h`, `sim/dungeon_spikes.cpp` (`Dungeon::updateRhythmTraps`, `stabWallSpikes`), the jet's hit `Dungeon::updateTraps` (`sim/dungeon_base.cpp`) and `Monster::StandInFire` (`sim/monster.cpp`), drawn in `graphics/dungeon_render_mechanisms.cpp`; `wallSpikesCrossable` in `level/level_check_mechanisms.cpp` |
| Poison drips: the lump, the drop, the puddle | `Dungeon::updateDrips`, `inFallingDrip`, `inPuddle` (`sim/dungeon_spikes.cpp`), drawn by `Dungeon::drawPoisonDrips` (`graphics/dungeon_render_drips.cpp`) |
| Sand and quicksand: the walk in sand, the sinking, the sand fall | `sim/dungeon_sand.cpp` (`Dungeon::PlayerInSand`, `updateQuicksand`, `landSandFall`) |
| Webs (the Mother of Webs): spinning, the slow, cutting and burning | `sim/dungeon_webs.cpp` (`Dungeon::updateWebs`, `updateSpinner`, `cutWebs`, `burnWeb`), drawn by `Dungeon::drawWebs` (`graphics/dungeon_render_webs.cpp`) |
| Burning floor (Babi's coal) | `sim/dungeon_blaze.cpp` (`Dungeon::PlaceBlaze`, `updateBlazes`, `PlayerInBlaze`), drawn by `Dungeon::drawBlazes` (`graphics/dungeon_render_blaze.cpp`) |
| Expedition pages: pickup, texts, the journal page, placement | `Dungeon::updateMechanisms` (`sim/dungeon_mechanisms.cpp`), drawn by `Dungeon::drawPageTile` (`graphics/dungeon_render_mechanisms.cpp`), the skeleton in `scatterProps` (`level/decor_scatter.cpp`), texts `content/texts/expedition.txt` (`expeditionPageText`, `level/journal_text.cpp`), `JournalScreen::DrawExpedition`, texture `tools/textures/expedition_page.py`; [levels.md](levels.md) lists the levels |
| Inscriptions: texts, reading, the plates and the hieroglyph sheet | `level/inscriptions.*` (data `content/inscriptions/*.txt`, [inscriptions.md](inscriptions.md)), `Dungeon::readInscription` in `sim/dungeon_interact.cpp`, the layout `level/inscription_layout.*`, drawn by `render/inscription_draw.*` from `graphics/dungeon_render_decor.cpp` and `JournalScreen::DrawInscription`, plates and sheet baked by `tools/textures/inscriptions.py` |
| Decoration (props, decals, torches) | `level/decor*.h`, `level/decor_scatter.cpp`, `sim/dungeon_decor.cpp`, `graphics/dungeon_render_decor.cpp`; placed by the decor and decal layers (`Dressing`, `level/level.h`), checked by `checkDressing` (`level/level_check_dressing.cpp`), painted with `tools/editor/dressing_brush.h`; the decoy drawn by `Dungeon::drawDecoys` (`graphics/dungeon_render_effects.cpp`) |
| Tile types, level file format, level grid, wall layer (fixtures) | `level/tile_defs.*`, `level/level.*` (`Fixture`, `isWallType`); drawn by `Dungeon::drawFixture` (`graphics/dungeon_render.cpp`) |
| Name gates, name cartouches, the royal names (per run), their frame (cartouche, serekh) | `level/names.*` (data `content/names/*.txt`, [names.md](names.md)), `RoyalName::frame` (its plate, `frameWord` for the scroll), `Dungeon::SpeakName`, `inscriptionPage` in `sim/dungeon_interact.cpp`, `Riddle::AskName` (`ui/riddle.cpp`), the pulse `Dungeon::nameGateAlpha` (`graphics/dungeon_render.cpp`), `checkNameGates` in `level/level_check_mechanisms.cpp` |
| Sub-cells: a cell's shape, the shape queries, the shaped glyphs, the editor's shape brushes, the map's drawing | `Tile::shape`, `solidAt`, `floorHeight`, `ceilingHeight`, `SHAPED_GLYPHS` (`level/level.h`), the shape list in `level/level.cpp`, `tools/editor/shape_brush.h`, `hatchShape` / `outlineShape` (`ui/map_view.cpp`) |
| Sub-cells in play: the player against steps and low ceilings, the drawn blocks, arrows and sunbeams stopped | `Dungeon::floorBelow`, `bodyBlocked`, `ceilingAt`, `playerOnStep` and the jump in `UpdateMovementState` (`sim/dungeon_base.cpp`), the jump's limits in `level/movement.h`; `Dungeon::drawShapeBlocks` (`graphics/dungeon_render.cpp`); `subcellAt` in `sim/dungeon_arrows.cpp`; `beamReach` (`level/sun_beam.h`) |
| Ceiling spikes and the ceiling death trap: the undersides, the hit (player, monster), the coward's leap, the drawing, the checker | `ceilingUndersides`, `spikedSubcell` (`level/level.h`), `jumpReachesUnderside` (`level/movement.h`, the loader in `level/level.cpp`), `Dungeon::ceilingTrapAt`, `ceilingSpikesHit` (`sim/dungeon_base.cpp`), `Monster::CeilingSpikes` (`sim/monster_leap.cpp`), `Dungeon::leapCeilingTrap`, `leapUnderSpikes` (`sim/dungeon_walkers.cpp`), `Dungeon::drawCeilingTrap` (`graphics/dungeon_render.cpp`), `PathMove::SpikedStep` (`level/level_walker.cpp`) |
| Sub-cells and monsters: heights and steps per kind, the walk, hop, charge and flight against shapes, paths through them | `heightQuarters`, `stepQuarters` (`level/monster_kinds.*`), `BodyFit`, `columnFits`, `shapePasses` (`level/climb_path.*`), `Dungeon::monsterFloor`, `stepAt`, `hopOnto` (`sim/dungeon_walkers.cpp`), `Dungeon::flightBand` (`sim/dungeon_flight.cpp`), `Monster::SetStepFloor` (`sim/monster.cpp`) |
| Sub-cells in the checker and its path script: the walker on standing places, steps, crawls, jumps; the script's run-ups, gates on a jump's arc, ladder to step | `Nodes`, `Walker::walk` / `jump` / `takeOff` (`level/level_walker.*`), `PathTakeOff` (`level/level_check.h`), `pathScript` (`level/path_script.cpp`); played in the sim by `tests/unit/subcell_mismatches_test.cpp` |
| Level checker, generator, campaign order | `level/level_check.*` (the path model `level_walker.*`, the checks in `level_check_dressing.cpp` and `level_check_mechanisms.cpp`), `level/level_gen.*` (entry points), `level/level_builder.h` (`LevelBuilder`) and its phases `level/level_gen_layout.cpp` (route, halls, wings, branches, water, sand), `level_gen_gates.cpp` (locks, gates, gate rooms, boss room), `level_gen_content.cpp` (monsters, traps, treasure, supplies), `level_gen_dressing.cpp` (texts, light, figures, props), `level/campaign.*` |
| Save / load | `state/game_state.cpp`, the file and its checks `sim/dungeon_save.*`, `level/save_slots.*`, `sim/dungeon_io.cpp` (the dungeon's part), the confirm box `ui/confirm_box.*` |
| Quick save (F5) and quick load (F9) | `GameState::Quicksave` / `LoadQuicksave`, the keys in `specialKeyPressed` (`input/input.cpp`), the Load list in `ui/menu_slots.cpp` |
| Autosave, death screen, Continue | `GameState::Autosave` / `CountDeath` (`state/game_state.cpp`), the safe moments `AutosaveMoment` (`level/world_events.h`) and `Dungeon::PlayerInDanger` (`sim/dungeon_monsters.cpp`), `ui/death_screen.*`, the Load list in `ui/menu_slots.cpp` and Continue in `ui/menu.cpp`, the newest save `GameState::LatestSave` / `SaveSlots::PickLatest` |
| Models, textures, sounds loaded once | `state/assets.cpp` |
| Zone looks: stone, decor set, light, haze, ambient loop, the level banner | `ZONE_LOOKS`, `levelLook` (`level/zone_look.h`), `Dungeon::Look`, the loop played in `state/game_loop.cpp`, the banner `ui/zone_banner.*` |
| Options and the settings file | `Settings` (`level/settings_ini.*`, the file `state/settings.*`, [settings.md](settings.md)), the Options rows and key cells in `ui/menu_options.cpp` |
| Frame rate cap | `graphics/frame_rate.*` (`FrameRate::set`, `idle`), `frameRedrawDue` (`level/settings_ini.*`) |
| Brightness, mouse sensitivity, master volume, window sizes | `render/brightness.*` (`Brightness::apply`, before the swap in `graphics/draw.cpp`), `brightenedChannel`, `mouseTurnPerPixel`, `playedVolume`, `windowSizes` (`level/settings_ini.*`) |
| Screens: inventory, journal, map, Heka, menu, riddle | `ui/inventory*`, `ui/journal_view.*` (each section's pages `ui/journal_pages.cpp`; data `level/journal.*`, its words: creature moves, field notes, diary `level/journal_text.*`, the prose in `content/texts/`; the new game's prologue `GameState::NewGame`), `ui/map_view.*`, `ui/skills_*`, `ui/menu*`, `ui/riddle.*` |
| Player-facing prose: field notes, diary, expedition pages, creature moves, monster notes, skill and item lore | `content/texts/*.txt` ([texts.md](texts.md)), read by `gameText` (`level/texts.*`); the keys asked in `level/journal_text.cpp`, `level/skills.cpp`, `level/items.cpp` |
| HUD (the quick slots: weapon, potions, antidote, amulet; the XP line and the level badge) | `ui/player_hud*` (`playerHudView` fills the slots), `ui/boss_bar.*`, `ui/status_box.*`, `ui/level_gem.*` |
| Status cards (the player's HUD row, a monster's over its health bar, the boss's over its HUD bar) and the shared vitals they read (HP, poison, fire, water, daze / stun) | `Vitals`, `Harm`, `Guard` (`level/vitals.h`, plan shared-vitals); `addConditions`, `statusList`, `monsterStatuses` (`level/status_list.*`); `PlayerHud::drawStatusCard` (`ui/player_hud.cpp`), `drawStatusRow` (`graphics/monster_draw.cpp`), `BossBar::draw`; icons `tools/textures/hud_icons.py` |
| A status effect on the HUD (blessing, condition or curse card): a cell in `tools/textures/hud_icons.py` (the icon; run it), an `Icon` entry (`level/hud_icon.h`), one `Add` in `statusList` (`level/status_list.cpp`: kind, icon, tint, pips, time left); no draw code. Cards: `tools/textures/status_cards.py` | `level/status_list.*`, `drawStatuses` (`ui/player_hud.cpp`) |
| Keys and mouse | `input/input.cpp`, `level/bindings.*`, `input/input_actions.h`; the camera peek and the look keys `level/camera_look.h`, the hidden cursor `followCursor` |
| Game loop, frame drawing | `state/game_loop.cpp`, `graphics/draw.cpp` |
| Blood splashes | `sim/particles.*` (motion), `graphics/particles_draw.*` |
| Scenario commands | `test/scenario*.cpp` |
| Plans: new, status, next, play-test, verdict, move, solve, reject, hold, lint, links, board | `tools/plan.py`, its tests `tools/plan_test.py` ([plans.md](plans.md)) |

## Tests

Scenario scripts in `tests/scenarios/` drive the game and take screenshots. `make test` runs all, `make test SCENARIO=path` runs one.
See [testing.md](testing.md). `make paths` plays levelcheck's path through every campaign level in the game
([levels.md](levels.md)).

## Code quality tools

* `make check`: builds, rewrites markdown links to plans that moved (`tools/plan.py links`), checks the plans' names
  and Status lines (`plan.py lint`) and runs the plan tool's tests, then `format`, the stability check of
  `format-check` on the changed files only (`tools/check_format.sh --changed`), `tidy` and `unit`, in order. Run it
  before each commit.
* `make format` formats the code. Formatting is the tool's job only: never align or wrap by hand, never think about
  it. `.clang-format` aligns nothing across lines (trailing comments, line continuations), so a diff shows only the
  lines that changed. A call too long for one line gets one argument per line and the `)` on its own line (the same
  for parameters). The whole-tree reformats for these are skipped by `git blame` ([version-control.md](version-control.md#blame)).
* `make format-check` fails on a file `make format` would change, and on code clang-format does not leave stable (a
  file that flips between two layouts on every `make format`, often a long trailing comment that wraps onto a second
  line: move the comment above the line). Every file, in parallel (the load gate's job cap).
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
