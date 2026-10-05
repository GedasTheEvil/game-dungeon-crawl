# Giant rat too slow

Status: draft (2026-10-05, from a playtest).

## Problem

The giant rat walks too slowly. The player can stand still and shoot it with the bow; it never reaches them.

## Findings

* Walkers step every 70 ms (`Monster::stepTimer`, `src/entities/monster.h`), each step
  `MONSTER_SEEK_STEP * speed` (`Monster::Seek`, `src/entities/monster_ai.cpp`): 0.0042 x 14.3 steps/s = 0.06
  tiles/s per speed point.
* Giant rat, speed 4 (`MONSTER_DEFS`, `src/state/assets.cpp`): about 0.24 tiles/s, a quarter of the player's walk
  (`WALK_SPEED` 1.0 tiles/s). The small rat (9) is 0.54 tiles/s.
* Bats use their own scale (`BAT_TILES_PER_SPEED`, 0.25 tiles/s per point), walkers have no such constant.

## Idea

* Make the giant rat faster than the player's walk (about 1.2 to 1.5 tiles/s), so the bow alone cannot kite it and
  the player has to sprint away or fight.
* Maybe use the leap (`Locomotion::WalkJump`) to close the gap on the player, as planned for the giant scarab in
  [monster-balance.draft.md](monster-balance.draft.md).
* Consider a `WALKER_TILES_PER_SPEED` constant, like the bats', so `speed` reads as tiles/s and the other walkers
  can be checked against the player's speed at the same time.
* Verify with a scenario: the player stands still and shoots, the giant rat reaches and bites them.
