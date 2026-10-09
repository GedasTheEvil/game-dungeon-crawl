# Sim library: Dungeon, monsters and the player without GL

Status: implemented 2026-10-09 (tooling only, so solved without a play test; see [Progress](#progress)). From the
user: solve the blocker of [scenarios-to-unit-tests](scenarios-to-unit-tests.md). It takes over step 4 of
[sim-unit-tests](sim-unit-tests.md) (the monster rules, merged in 2026-10-09 from its own draft) and goes
further: the whole `Dungeon`, not only the monster AI.

## Problem

The unit tests link `liblevel.a` and `libbase.a` only (no GL). `Dungeon`, `Monster`, `Player`, `Item` and `Trap` are in
the app, so their rules (positions, hitboxes, the AI, missiles, mechanisms, bosses, the water sink, the flames) can
only be checked by a scenario. That is 24 debug-only scenarios, and most of the geometry and animation ones.

What pulls GL in, by header:

| Header | GL side it includes |
|---|---|
| `world/dungeon.h` | `entities/monster.h` |
| `entities/monster.h` | `character_model.h`, `graphics/particles.h`, `graphics/texture_registry.h` |
| `entities/player.h` | `character_model.h`, `graphics/particles.h`, `graphics/texture_registry.h` |
| `entities/character_model.h` | `graphics/animated_model.h`, `graphics/textures.h`, `core/sound.h` |
| `entities/item.h` | `core/sound.h`, `graphics/animated_model.h`, `graphics/textures.h` |
| `entities/trap.h` | `graphics/animated_model.h`, `graphics/textures.h` |

By source file:

* `src/world/dungeon_{arrows,base,boss,decor,io,mechanisms,monsters}.cpp` have no GL calls (`check_sim.sh`), but include
  `state/assets.h`. Their asset use is small: `sim.assets->monsterTypes[...]` in `dungeon_boss.cpp:115` and
  `dungeon_monsters.cpp:256` (the spawn).
* `monster.cpp` (34 GL / model / particle uses), `player.cpp` (19), `item.cpp`, `trap.cpp`, `character_model.cpp`:
  sim and drawing mixed in one class.
* `monster_ai.cpp` includes only `ink.h` and `render_config.h`. `Ink::figureScale()` (toon mode) enters the hitboxes.

What the sim reads from a model: the bbox (half widths: `Monster::HalfWidth`, `monster.cpp:158`, `Player::HalfWidth`,
`player.cpp:58`), each clip's frame count and loop flag (`CharacterModel::Enter` / `Advance` / `Progress`,
`character_model.cpp:83-115`, `AnimPlayback`), and marks such as the spit's `release` and `mouthY`.

## Idea

Steps, each its own commit. The game plays the same after each one (scenario screenshots byte-identical, as in
[sim-unit-tests](sim-unit-tests.md) step 2):

1. **Model facts as data.** A GL-free `ModelInfo` (bbox, clips: frame count, loop, fps, marks). Read from the md3 by
   a parser split from `animated_model.cpp` (the parsing is GL-free until the display list compiles,
   `animated_model.cpp:241`). Or bake it into a table with a unit test against the md3 files. `AnimPlayback` and the
   clip state machine move next to it.
2. **Monster split.** `MonsterSim`: position, timers, state, HP, clip state, the AI (`monster_ai.cpp`). It reads its
   `MonsterKind` and its `ModelInfo`. The app's `Monster` (or `MonsterView`) owns the `CharacterModel`, the particles and
   the textures and draws a `MonsterSim`. Sound and particles go out as `WorldEvents`, as the scatter did.
3. **Player split.** The same for `Player`: `PlayerSim` with `PlayerStats` (already in the lib), the box, the climb,
   the wading and the swing (`swingPose` moves from `graphics/draw.cpp:62-87`).
4. **Item and trap split.** The rules (damage, reach, `Item::Reach()`) go to the lib, the model and the sound stay in
   the app.
5. **Dungeon in the lib.** `dungeon.h` includes only the sim headers. The `sim.assets->monsterTypes` lookups become a
   `MonsterKind` and `ModelInfo` table passed in through `SimLinks`. The `dungeon_render*.cpp` files stay in the app.
   `Dungeon::flamesAt` and `TORCH_FIRE` move out of `dungeon_render_decor.cpp`, with a plain light id instead of
   `Lighting::LightDef`. `Ink::figureScale()` becomes a value in the links.
6. **Test harness.** A unit test loads a level (`levels/lvl*` or a small test map), steps N fixed ticks on the virtual
   clock (`core/timer.h`, `UPDATE_TICK_MS`) with the seeded gameplay stream (`world/rng.h`), and reads positions, HP,
   (state, frame) and the `WorldEvents`. `Dungeon::Dump` / `LoadDump` set up a state.

Checks: `check_layers.sh` gets a `sim` library between level and render. `check_sim.sh` extends to `src/entities/`.

## Decided (2026-10-09)

* **Library:** a separate `build/libsim.a`: base ← level ← sim ← render ← app. The unit tests link sim, level and
  base. `liblevel` stays the map, the checker and the tools' library; `levelcheck` does not need the sim.
* **The cut:** a sim class owned by the view class (`Monster` holds a `MonsterSim`, `Player` a `PlayerSim`). Monsters
  and the player carry a lot of state from tick to tick, so a class fits them better than the scatter's free
  functions (which build a layout once). The draw code reads the sim's state and never writes it.
* **`ModelInfo`:** parsed from the md3 at run time by a GL-free parser split from `animated_model.cpp`. The unit tests
  read the real model files, so no generated table to drift from the models.
* **Order:** step 1 (`ModelInfo`) on its own: it is small and unlocks the hitbox / reach matrix and the clip tests.
  Steps 2-6 together with the first AI change that comes ([monster-balance](../monster-balance.draft.md)'s leap or
  [monster-climbers](../monster-climbers.draft.md)). Not on its own: the split is the biggest step and pays off only
  with a concrete AI change to test.
* **Monster rules to unit test first:** the decisions in `entities/monster_ai.cpp`: `Seek`, `canSpit`, `canDive`,
  `canCharge`, `UpdateCharge`, `Fly`. What the rules get through the links (the map, the player's box, the random
  stream), without the assets, is settled in step 5.
* Tooling only (no change to the game): once done and checked, straight to `solved/`.

## Progress

* 2026-10-09: the user asked for the whole plan now, not with the first AI change.
* Step 1 done: `build/libsim.a` with `entities/md3_mesh` (the MD3 parser and the frame measures, split from
  `AnimatedModel`) and `entities/model_info` (`ModelInfo`: the clips' frame counts and loop flags, the frame 0
  extents; `AnimPlayback`, `AdvancePlayback` and the clip state machine). `CharacterModel` loads through `LoadClips` and
  hands out its `ModelInfo`. `core/logger` moved to the base library. `tests/unit/model_info_test.cpp`. Scenario
  screenshots byte-identical (602).
* Step 2 done: `MonsterType` holds the `ModelInfo`, the app's `CharacterModel` per type is `Assets::monsterModels`;
  `DrawMonster` (`entities/monster_draw.cpp`) draws a monster. Two changes from the plan: the blood stays in the sim
  (`entities/particles`: the splash motion, its own random stream, no GL; `Particles::Draw` in graphics), so the
  splashes run in the same order as before; and the toon figure scale is `Figures::Scale()` in the sim library (set
  by `Ink::setToon`), not a value in the links: toon mode changes at run time, and a monster copies its links when it
  spawns. The monsters' sounds go out as `WorldEvents::PlayCharacter`.
* Step 3 done: `Player` keeps the stats, the pose (its `ModelInfo`, clip state, blood) and the jump and attack state;
  `PlayerView` (`entities/player_view.cpp`, held by `GameState`) loads the model, draws it and finds the fists. The
  god mode is `Player::god` (the scenario sets it), the die and jump sounds are world events. `swingPose` stays in
  `draw.cpp` for now: [scenarios-to-unit-tests](scenarios-to-unit-tests.md) technique 1 moves it.
* Step 4 done: `Item` keeps the model and the sounds; damage, range, mix and motion are read from `weaponDef`, the
  reach from `weaponReach` (`world/items.h`). `Trap` only draws, it stays as it is.
* Step 5 done: the dungeon's rule files (`world/dungeon_{arrows,base,boss,darts,decor,io,mechanisms,monsters}.cpp`),
  `monster.cpp`, `monster_ai.cpp` and `player.cpp` are in `libsim.a`. The monster types come through
  `SimLinks::monsterTypes` (`MonsterTypes`, `LoadMonsterTypes` for a sim without `Assets`); `SimLinks::assets` is
  left for the drawing only. The flames are `flamesAt` on the `DecorLayout` (`world/decor_scatter.h`, tile units, a
  `FlameKind`); `dungeon_render_decor.cpp` gives each its light and fire sprite. The cut differs from the decision:
  the `Dungeon` owns its monster slots, so the sim classes keep the names `Monster` and `Player`, and the drawing is
  a function (`DrawMonster`) or a view beside them (`PlayerView`), not a class that holds the sim.
* Step 6 done: `tests/unit/sim_world.h` (`SimWorld`): a level, the player and the monsters stepped tick by tick as the
  game loop does, with `WalkTo`, the journal's moves and the events. `tests/unit/sim_test.cpp`: the wall and the rock
  fall stop a walker (the numbers of `monster_walls` and `coward_rock`), the cobra's spit (`canSpit`), Sobek's charge
  and stun (`canCharge`, `UpdateCharge`), Apep's dive (`canDive`), the bat's swoop (`Fly`). The unit tests run in
  about 4 s (most of it loading the models).
* Every step: the scenario screenshots byte-identical to the start (602 of 106 scenarios).
