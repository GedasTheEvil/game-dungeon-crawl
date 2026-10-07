# Unit tests for the simulation: rules out of the GL files

Status: draft 2026-10-07. From the [architecture review](solved/architecture-review.md), refactor 3. The leftover
that [world-without-game](solved/world-without-game.md) left for "its own stage".

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

## Open

* How far to go after step 3.
