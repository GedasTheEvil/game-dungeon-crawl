# A drag pose and clip for the player pulling or pushing a mirror

Status: draft 2026-11-13, size M, needs nothing first.

## Idea

[mirror-drag](solved/mirror-drag.feature.md) plays the crouch clips while dragging (`Player::UpdateCrouchPose`): the
crouch-walk legs step forwards while the player backs away pulling, and the hands do not hold the thing. The user
play-tested the mechanism as OK; the animation needs adjustment.

## Plan

* New clips on the archeologist (`tools/blender/models/archeologist.py`, [modeling.md](../modeling.md)): bent half
  as in the crouch, both hands forward on the thing held (at `DRAG_HOLD` from the body, stand height); a still hold, a
  pull (walking backwards) and a push (walking forwards).
* `ModelState` entries for them (`src/sim/model_info.h`), picked while `Dungeon::Dragging()` by the walk direction
  against `DragFacing()`; the weapon hidden or lowered while both hands hold.
* Check the hands meet the mirror stand in a scenario screenshot (`tests/scenarios/solved/mirror_drag.txt` as a start).

## Play-test

*
