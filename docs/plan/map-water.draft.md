# Water on the draft map

Status: draft 2026-10-07, from the user.

The draft map (`src/ui/map_view.cpp`, `DraftMap::Draw`) shows no water. Half water and deep water cells
(`Structure::HalfWater`, `Structure::DeepWater`, `src/world/level.h`) are drawn like open floor: only walls get
hatched (`hatch`) and outlined (`outline`). The player can't see on the map where the flooded stretches are.

## Idea

Explored water cells get pencil scribbles, the way someone would mark water on a hand-drawn map:

* Wavy lines (two or three short "~" strokes per cell) in a blue pencil, next to the graphite and the lock colours
  (`LOCK_PENCILS`).
* With the same wobble as the other strokes (`Sketch::pencil`, `JITTER`, `cellSeed`), so neighbouring cells don't
  repeat the same pattern exactly.
* Half water: waves only in the lower part of the cell (the water line). Deep water: waves over the whole cell,
  denser, maybe with light blue hatching, so it reads as "you can't walk here".

## Open

* Blue pencil or graphite waves.
* Whether deep water gets an outline like a wall (it blocks like one), or only the waves.
* A legend entry, if the map ever gets a legend.
* Check: the crocodile levels (flooded halls, lvl21) and a scenario screenshot of the map over water.
