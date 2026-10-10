# Ceiling under deep water

Status: draft 2026-10-10 (idea, not decided).

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
