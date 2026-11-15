# Fallen rocks from an old save are invisible

Status: draft 2026-11-15, size S, needs nothing first.

## Repro

1. Load a save made before plan [fallen-rock-draggable](fallen-rock-draggable.bug.md) (commit d58cbd63) on a level
   whose rock falls (`RockFall`, glyph `v`, attribute 0) had already fallen.
2. Look at those cells.

## Expected

The landed rock lies on the floor as a draggable `Stone` (the same as a rock that lands in the current build).

## Actual

Neither the ceiling crack nor the rock is drawn. The save keeps the cell as a `RockFall` with `RockState::Fallen`.
Since plan fallen-rock-draggable, `drawRockFallTile` (`src/graphics/dungeon_render_mechanisms.cpp`) draws the crack
only for a not yet fallen cell and the rock only while it falls: a landed rock is meant to be a `Stone` already
(`Dungeon::landRock`, `src/sim/dungeon_mechanisms.cpp`). Nothing turns an old fallen `RockFall` into a `Stone` on
load, so the cell draws nothing. It is likely also not draggable and not a plate weight.

## Fix

Implementer's choice. Idea: after a dungeon is loaded (`Dungeon::LoadDump`, `src/sim/dungeon_save.cpp`, or
`resetMechanisms`), turn every `RockFall` that is `Fallen` and not a sand fall into a `Stone` (`landRock` does it).
Sand falls (attribute 1) stay fallen `RockFall` cells over their drift.

Check also: the map mark (`src/level/map_marks.h`) and `climb_path.cpp` of such a cell after the fix.

## Fixed when

* A test that names the plan fails before the fix and passes after it: a unit test that loads a dungeon dump with a
  fallen rock (not sand) `RockFall` cell and finds a `Stone` there; a fallen sand fall stays a `RockFall`.

## Play-test

*
