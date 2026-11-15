# Fallen rock from a rock fall is not draggable

Status: implemented 2026-11-15, waiting for the user's play-test. Was: draft 2026-11-15, size M, needs nothing first.

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

## Done

* On landing (and on a load taken mid-fall) a rock's cell turns into a `Stone` (`Dungeon::landRock`,
  `src/sim/dungeon_mechanisms.cpp`): the stone's drag, plate weight, save and render paths (the same rock model). The
  spent ceiling crack goes with it. Sand falls stay fallen `RockFall` cells over their drift.
* `drawRockFallTile` no longer draws a landed rock. The map mark of a landed rock is the stone's.
* The level checker counts an armed rock fall (not a sand fall) as a stone's weight on a plate's floor run
  (`dragWeight`). No campaign level has a new warning.
* Test: `tests/unit/pressure_plate_test.cpp` ("a fallen rock is a stone", level `tests/levels/fallen_rock_drag`).

## Play-test

* `bin/game --play tests/play/fallen-rock-draggable.txt`: walk over the loose ceiling and on; the rock lands. Face
  it, drag it (the drag key) onto the plate, let go: the silver gate rises.
* Change the script's level line to `level 4 at 24 27`: lvl4's rock fall in the campaign, its rock drags the same.
