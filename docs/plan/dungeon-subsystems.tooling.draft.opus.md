# Split Dungeon into subsystems

Status: draft 2026-11-16, size L, needs nothing first.

## Problem

The architecture review (2026-11-16, smell 12): `Dungeon` (`sim/dungeon.h`, about 1090 lines, about 165 data
members, 23 nested structs, about 380 methods, 28 `dungeon_*.cpp` files) owns the map, decor, containers, plates,
webs, blazes, sun and mirrors, the stargate, cut-scenes, missiles, the boss fight, drag, save and the player's motion.
`friend class DungeonView` lets the drawing read about 49 private members. Ten test-only methods
(`KillNearestMonster`, `SetBossHealth`, ...) sit in its public API.

## Plan

* One subsystem at a time, each with its own state, update and save part, and a const view for the drawing:
  `CutScene`/`GateTravel` and the stargate first (they don't touch monsters), then `Mechanisms` (plates, dial,
  mirrors, sun beams), `Projectiles` (missiles, shots, balls), `Hazards` (webs, blazes, chest gas, holes, puffs).
* The test-only methods to a `DungeonCheats` friend used only by `src/test/`.
* Each step: no behaviour change; unit tests and the archived scenarios give the same results as before.
* Easier after [player-body](player-body.tooling.draft.opus.md).
