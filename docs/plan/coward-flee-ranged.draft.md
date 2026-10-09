# Cowards flee when they cannot reach the player

Status: draft 2026-10-09, refined 2026-10-09 (decided, not implemented). From the user: if a "coward" monster
cannot reach the player, instead of going towards it, it runs away to avoid ranged attacks.

## Today

* `Courage::Coward` (`src/world/monster_kinds.h`): afraid of traps. A walker stops at a trap's edge, a walk-jumper
  leaps over it (`tests/scenarios/coward_rock.txt`: the rat waits at an armed rock fall and never bites).
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
