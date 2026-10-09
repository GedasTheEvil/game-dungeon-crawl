# Cowards flee when they cannot reach the player

Status: draft 2026-10-09 (idea, not decided). From the user: if a "coward" monster cannot reach the player, instead
of going towards it, it runs away to avoid ranged attacks.

## Today

* `Courage::Coward` (`src/world/monster_kinds.h`): afraid of traps. A walker stops at a trap's edge, a walk-jumper
  leaps over it (`tests/scenarios/coward_rock.txt`: the rat waits at an armed rock fall and never bites).
* A coward that cannot get to the player (a trap, a gap, another floor) keeps seeking: it stands as near as it can,
  facing the player, an easy target for the bow and throwing weapons.

## Idea

* When a coward cannot reach the player, it backs off out of the player's line of fire (off the row, behind a corner,
  out of range) instead of waiting at the obstacle.
* Once a way opens (the rock has fallen, the player comes down), it seeks again.

## Open

* "Cannot reach": how the AI knows. Blocked by a trap edge only, or a path search (the climbers' path-finding,
  [monster-climbers](monster-climbers.draft.md))?
* Where to: away along the row, a set distance, or to a cell out of the player's line of sight.
* Only when the player has a ranged weapon in hand, or always?
* Which monsters: all cowards, or only some (fliers and the stationary ones are out)?
* Bosses are all reckless today, so the boss ability rule (`AGENTS.md`) likely does not apply; check.
* Checker score: a coward that flees is harder to kill but no more dangerous; likely no `threat` change.
* An AI change: it goes with the sim cut ([sim-library](sim-library.draft.md)) and gets unit tests.
