# Fallen rock from a rock fall is not draggable

Status: draft 2026-11-15, size M, needs nothing first.

## Repro

1. Step on a loose ceiling (`RockFall`, glyph `v`, attribute 0: a rock, not a sand fall) and step off.
2. The rock falls and lies on the floor of that cell.
3. Stand next to it, face it, press the drag key.

## Expected

The player takes hold of the rock and drags it like a `Stone` (same drag rules: bare flat floor, onto a pressure
plate, the drag hint shows).

## Actual

Nothing happens. The landed rock stays a `RockFall` tile with `RockState::Fallen`
(`Dungeon::updateRocks`, `src/sim/dungeon_mechanisms.cpp`) and is drawn by `drawRockFallTile`
(`src/graphics/dungeon_render_mechanisms.cpp`). `isDraggable` (`src/level/level.h`) takes only `Mirror` and `Stone`.

## Fix

Implementer's choice. Ideas:

* On landing, turn the cell into a `Stone` (the spent crack goes; or keep the crack drawn over a cell whose ceiling
  fell). The rock then uses the stone's drag, save and render paths. Check the stone's model matches the fallen rock,
  or draw a moved stone with the rock's model.
* Or make a fallen rock draggable in `isDraggable` and teach the drag code to move it out of its `RockFall` cell
  (leaving a spent `RockFall`, ceiling crack only, behind).

Check also:

* The other place a rock lands already fallen: `src/sim/dungeon_mechanisms.cpp` near line 44 (sets `Fallen` without
  the fall). Same result there.
* Sand falls (attribute 1) stay as they are: no rock, the cell turns to a drift.
* The map mark (`MapMark::RockFall` in `src/level/map_marks.h`) and `climb_path.cpp` read `Fallen`; keep them right
  once the rock can leave the cell.
* Save and load of a dragged rock (a level reload after a drag keeps it where it was left).
* The level checker: a rock that can be dragged onto a pressure plate may count as a plate weight; no campaign
  level may get a new warning (`bin/levelcheck content/levels/lvl*`).

## Fixed when

* A test that names the plan fails before the fix and passes after it: a scenario or unit test where a rock falls,
  the player drags it one cell.

## Play-test

* `bin/game --level N` on a level with a loose ceiling (pick one with `grep -l v content/levels/lvl*` and its play
  command): trigger the rock, drag it along the floor, onto a pressure plate if one is near.
