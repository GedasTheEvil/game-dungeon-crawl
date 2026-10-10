# Ceiling under deep water

Status: draft 2026-10-10, implemented 2026-10-10 (see [Implemented](#implemented-2026-10-10)); to play-test.

## Problem

An open corridor cell (Empty) with a DeepWater ("full water") cell right above it has no ceiling: the water hangs
over the corridor with nothing holding it back. Expected: at least a thin layer of rock between them.

## Where

`Dungeon::drawCellSurfaces` (`src/world/dungeon_render.cpp`) draws an open cell's ceiling only when the cell above is
rock (`isRock(i, j + 1)`). DeepWater is solid for movement (`isSolidStructure`) but not rock for rendering, so the
ceiling is skipped.

## Ideas

* Draw the ceiling quad also under DeepWater, plus a thin rock slab (the rock face texture) at the bottom of the
  water cell, so the water sits on a visible stone shelf and the corridor has its normal ceiling.
* The slab eats into the water cell's lower edge; the water surface / volume drawing may need to start above it.
* Check the same for HalfWater above an open cell: the basin's floor (`isRock(i, j - 1)` test) is probably missing
  too, so the basin would show the corridor below through it.
* Check the side walls: an open cell next to DeepWater on the left / right (water cut off at the side).

## Implemented (2026-10-10)

* A deep water cell meeting a cell that is neither rock nor deep water, below or beside it, gets a rock slab
  `WATER_SLAB` (4 units) inside the cell: a rock front strip, its water box (`Dungeon::deepWaterBox`) shrinks to the
  inside, and its floor and side walls are drawn on the slab. The dark front (`drawWaterCell`) covers only the box.
* The cells around deep water treat it as rock: a ceiling under it, a side wall beside it.
* Half water over an open cell (the checker flags it, so in tests only): the open cell gets a ceiling at the basin's
  floor and a rock front over it, as a rock cell below would show.
* Test: `tests/scenarios/water_ceiling.txt` on `tests/levels/water_ceiling` (screenshots, also tilted and toon).
