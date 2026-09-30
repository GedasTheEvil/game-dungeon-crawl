# Gate plasma faces the adjacent wall

Status: implemented. Test: `tests/scenarios/gate_facing.txt` (level `tests/levels/gate_facing`).

## Problem

The Anubis gates (`Door` tiles) draw a plasma quad on a fixed side, chosen by gate type: the entrance on one side,
the exit shifted 39 units to the other (`src/world/dungeon_render.cpp`, the `GateEntrance || GateExit` block).
The level layout plays no part.

## Wanted

* The plasma side follows the walls next to the gate:
  * wall on the right: plasma on the right, against the wall
  * wall on the left: plasma on the left
  * walls on both sides: does not occur. The gate would be a dead end with no way left or right
  * no wall on either side: the whole gate (statues and plasma) turns 90° so it faces the camera
* The gate turns only in fixed 90° steps. It does not billboard or follow the camera. At the level start the
  plasma faces left.
* The facing rule applies to every gate type: `GateEntrance`, `GateExit`, `GateRiddle` and `GateEmpty`.
* Only the entrance and exit gates get plasma, because plasma means a teleport. The riddle gate shows only its
  question mark. Once the riddle is answered, the gate becomes `GateEmpty`, a dead gate with no plasma and no mark.
  This is intended.

## Gates without a purpose

* A new map has no dead gates. `GateEmpty` appears only at runtime, from an answered riddle gate.
* The level checker (`src/world/level_check.cpp`, next to the other `r.warnings`) warns about a `GateEmpty` gate in
  the level data. Checker only: `generateLevel()` already drops every candidate with a warning
  (`src/world/level_gen.cpp`), and the editor and `tools/level/levelcheck` run the same check.
