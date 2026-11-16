# The dungeon drawing only reads the sim

Status: solved 2026-11-16. Was: draft 2026-11-16, size S, needs nothing first.

## Problem

The architecture review (2026-11-16, smell 3): `DungeonView` held a non-const `Dungeon&` and its draw code expired
the burrow holes, sprays, sand puffs and landing puffs, and started the sand pours (`dungeon_render_effects.cpp`,
`dungeon_render_sand.cpp`). The sim pruned puffs and landings as well, only when it added one. What a scenario or a
unit test read (`DustPuffs`, `LandingPuffs`) depended on whether a frame was drawn.

## Plan

* `Dungeon::updateEffects()`, each tick: drops what ran out (`BURROW_HOLE_MS`, `BURROW_SPRAY_MS`, `SAND_PUFF_MS`,
  `LANDING_PUFF_MS`, `SAND_POUR_MS` + `SAND_POUR_TAIL_MS`) and starts a sand fall's pour as its warning ends.
* `DungeonView` holds a `const Dungeon&`; the draw skips an entry that ran out since the tick.
* The friend stays (the draw reads about 49 private members); accessors for them are a later step if wanted.
