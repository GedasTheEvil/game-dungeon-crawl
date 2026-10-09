# Unit tests for the simulation: rules out of the GL files

Status: draft 2026-10-07. Steps 1-3 done 2026-10-07 (see [Implementation](#implementation)); step 4 moved to
[sim-library](../sim-library.draft.md). From the
[architecture review](architecture-review.md), refactor 3. The leftover that
[world-without-game](world-without-game.md) left for "its own stage".

## Problem

Monster AI, player stats, the decoration scatter and the mechanism / missile / boss rules only run as scenarios (97,
~7-8 min). They have no `Game()` any more, but they are not in the unit test binary: every `dungeon_*.cpp` includes
`state/assets.h` and most include GL; `monster.h` pulls in particles, textures and the character model; drawing
(`draw*`) sits in the same files as the rules (`dungeon_decor.cpp:328-493`, `dungeon_mechanisms.cpp:211-290`,
`dungeon_boss.cpp:94-121`, `dungeon_arrows.cpp:165-258`, `dungeon_monsters.cpp:230`).

## Steps (each worth it on its own)

1. **Clock.** `core/timer.cpp` (`GameClock`) needs only `<chrono>`, but the render library uses it too, and
   `check_layers.sh` lets a library include only its own headers: a small base library both link (or a check
   exception for `core/timer.h`). Then
   `entities/player_stats.cpp` (stamina, heal, `HitDamage`, `AdvanceLevel`) can join it and get unit tests.
2. **Decor scatter.** Split `dungeon_decor.cpp` into the scatter rules (seed + tier, no GL) and the drawing; the rules
   join the level library with tests.
3. **Draw code out of the sim files.** Move the `draw*` functions into `dungeon_render*.cpp`, so `make layers` can
   check that the other `dungeon_*.cpp` include no GL (extend `check_sim.sh`).
4. **Monster rules.** The decisions in `monster_ai.cpp` (`Seek`, `canSpit`, `canDive`, `canCharge`, `UpdateCharge`,
   `Fly`) testable without the model and particles: the monster's sim state apart from its drawing parts. The biggest
   step; only if 1-3 show the way.

## Implementation

Done 2026-10-07, one commit per step. Tooling only (no change to the game), so straight to solved.

1. **Clock.** `core/timer.cpp` is `build/libbase.a`, linked by everything that links a library. `check_layers.sh`
   takes a library named `base` first: every later library may include its headers, and it has no GL. `PlayerStats`
   (`entities/player_stats.cpp`) is in `liblevel.a`; `tests/unit/player_stats_test.cpp` covers level ups, stamina
   (sprint drain, regeneration, refusal on the virtual clock), hits, amulets, potions, save and load. Found on the
   way: an archive was not rebuilt when a file moved into its source list (the object is older than the archive), so
   each archive now depends on the makefile.
2. **Decor scatter.** `world/decor_scatter.{h,cpp}` (level library): `scatterDecor` fills a `DecorLayout` (props,
   decals, torches, ladders, surfaces) from a plain `Tile` grid, the level name and the depth, `bossCoffinCell` and
   `decorTierUsed`. The boss table comes in as a `CoffinBoss` predicate, the counts go back for the log. `Dungeon`
   holds one `DecorLayout` and draws it. Same output as before: a scenario over all 30 levels and four generated ones
   gives byte-identical screenshots and log counts on the old and the new build. Tests in `tests/unit/decor_test.cpp`
   (same layout per file name, props on the floor of empty cells, tiers by depth, torch spacing, ladder shafts, cave
   surfaces, boss coffins alive and slain); breaking the torch gap or the ladder piece rule makes them fail.
3. **Draw code out of the sim files.** `dungeon_render_decor.cpp`, `dungeon_render_mechanisms.cpp` and
   `dungeon_render_effects.cpp` (monsters, missiles, venom, summon effects) next to `dungeon_render.cpp`. What both
   sides use (the missile rules, `motionProgress`) is in `world/dungeon_rules.h`. `check_sim.sh` fails on a GL or
   graphics include (but the constants of `render_config.h`) in any other `src/world/dungeon*` file.

Checks: `make` with no warnings, `make format-check`, `make tidy` (with `make layers`), `make test` (unit tests and
every scenario), `./levelcheck levels/lvl*`. Docs: [development.md](../../development.md) (Libraries),
[testing.md](../../testing.md), [remodeling.md](../../remodeling.md).
