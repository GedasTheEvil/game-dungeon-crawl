# Split Dungeon::UpdateMonsters into its phases

Status: draft 2026-11-15, size M, needs nothing first.

## Problem

`Dungeon::UpdateMonsters` (`src/sim/dungeon_monsters.cpp:15`) has a cognitive complexity of 103 over 145 lines
(`make complexity`, plan complexity-hotspots): one loop over the monsters that handles the call, the death and the
drop, the step out of the water, the rungs, flight, leaps, chests, lurking, stun, the bite, the hold, the
locomotions, the ranged shot and kiting, each as a nested `if` with `continue`s. Monster plans touch it often (12
commits since 2026-10-12) and each has to read all of it to find its place.

## Plan

* Split the loop body into phase functions on `Dungeon` (or free functions in the file), in the same order:
  `updateMonsterDeath` (call, drop, rungs, chest), `updateMonsterMotion` (water step, flight, leap, locomotions),
  `updateMonsterAttack` (lurk, stun, bite, hold, ranged, kite). A phase returns whether the monster is done for this
  tick (the old `continue`).
* Keep the order of the checks exactly: the sim tests and the balance sim must give the same numbers
  (`make unit`, `bin/balance` before and after, the same output).
* Rerun `make complexity`: each phase under 40; note the numbers here. Where is X row for monster update. 
