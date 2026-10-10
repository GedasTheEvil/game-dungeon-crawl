# Cowards flee when they cannot reach the player

Status: draft 2026-10-09, refined 2026-10-09, implemented 2026-10-10 (see [Implemented](#implemented)), not play-tested. From the user: if a "coward" monster
cannot reach the player, instead of going towards it, it runs away to avoid ranged attacks.

## Today

* `Courage::Coward` (`src/world/monster_kinds.h`): afraid of traps. A walker stops at a trap's edge, a walk-jumper
  leaps over it (`tests/unit/sim_test.cpp`: the rat waits at an armed rock fall and never bites).
* A coward that cannot get to the player (a trap, a gap, another floor) keeps seeking: it stands as near as it can,
  facing the player, an easy target for the bow and throwing weapons.

## Idea

* When a coward cannot reach the player, it backs off out of the player's line of fire (off the row, behind a corner,
  out of range) instead of waiting at the obstacle.
* Once a way opens (the rock has fallen, the player comes down), it seeks again.

## Decided (2026-10-09)

* **Unreachable:** by the path search of [monster-climbers](monster-climbers.draft.md), with the monster's own
  abilities: walking, planned jumps for the walk-jumpers, ladders for the climbers; an armed trap blocks a coward
  (a walk-jumper may leap it). No path within the cap = unreachable. So this comes after monster-climbers.
* **Where to:** the nearest cell out of the player's line of fire (off the row, behind a corner). If none is near,
  away along its row, out of bow range.
* **When:** always when unreachable, whatever the player holds.
* **Who:** the moving cowards: scarab, rat, worm, scorpion, giant scorpion, giant rat, giant scarab. Not the bosses
  (the boss scarab holds its arena), so no boss ability rule here. Not the flyers, the stationary and ambush ones
  (plant, mimic, egg cluster), the cobras or the crocodile.
* **Back:** once a path opens (the rock has fallen, the player comes near), it seeks again.

## Open (for the implementer)

* Checker score: a coward that flees is harder to kill but no more dangerous; likely no `threat` change.
* An AI change: it gets unit tests on the sim harness ([sim-library](solved/sim-library.md)) (an unreachable rat
  leaves the row; a path opens, it comes back; a giant rat with a jump in reach does not flee).

## Implemented

* `Monster::flees` (Walk or WalkJump, coward, not a boss), `Dungeon::walkerReaches`, `Dungeon::fleeFrom`
  (`src/world/dungeon_monsters.cpp`), `Monster::Flee` / `Stand` (`src/entities/monster_ai.cpp`).
* **Unreachable**, without the climbers' path search (not built yet): a scan along the row from the monster to the
  player. A cell `walkerBlocked` stops a walker; a walk-jumper goes on from its `leapLanding`, if there is one. The
  jump cooldown does not count. A walker never leaves its row, so a player on another floor is no target: the coward
  does what it did before. When monster-climbers lands, its path search replaces the scan.
* **Where to:** off the row is not possible for a walker yet, so it runs along its row, away from the player, until
  `COWARD_SAFE_GAP` (4.5 tiles between the boxes, past the composite bow's 4.0) or its own dead end. It stays put
  while a wall is between them (out of the line of fire already). There it stands (idle), facing the player.
* **Back:** checked on every step; once the scan gets through, it seeks again.
* Checker: no `threat` change.
* Tests: `tests/unit/sim_test.cpp` (a rat behind a pit runs out of the bow's range and stands there; a rat behind a
  rock fall runs, then comes back and bites once the player is past the rock fall; the giant rat's leap over the rock
  fall is the old test). `tests/levels/hitbox_bow` got a wall behind its giant rat, so it stays to be shot.
