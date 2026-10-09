# Sim library: Dungeon, monsters and the player without GL

Status: draft 2026-10-09 (idea, not decided). From the user: solve the blocker of
[scenarios-to-unit-tests](scenarios-to-unit-tests.draft.md). It covers
[sim-unit-tests-monster-rules](sim-unit-tests-monster-rules.draft.md) (step 4 of
[sim-unit-tests](solved/sim-unit-tests.md)) and goes further: the whole `Dungeon`, not only the monster AI.

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
[sim-unit-tests](solved/sim-unit-tests.md) step 2):

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

Checks: `check_layers.sh` gets the new files in the level library (or in a new `sim` library between level and
render). `check_sim.sh` extends to `src/entities/`.

## Open

* One `liblevel` or a separate `libsim` (level ← sim ← render ← app)?
* The cut: a sim class owned by the view class, or free functions on state structs as in the scatter.
* `ModelInfo`: parse the md3 at run time, or a generated table checked by a test?
* Order: step 1 alone already unlocks the hitbox / reach matrix and the clip tests. Steps 2-5 are big; the
  [sim-unit-tests-monster-rules](sim-unit-tests-monster-rules.draft.md) decision was "only together with an AI
  change" (the leap in [monster-balance](monster-balance.draft.md)). Keep that, or do the split first, on its own?
* Tooling only (no change to the game): once done and checked, straight to `solved/`.
