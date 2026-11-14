# A drag pose and clip for the player pulling or pushing a thing

Status: solved 2026-11-14 (play-tested by the user). Was: implemented 2026-11-14, waiting for the user's play-test. Was: draft 2026-11-13, size M, needs nothing first.

## Idea

[mirror-drag](mirror-drag.feature.md) played the crouch clips while dragging (`Player::UpdateCrouchPose`): the
crouch-walk legs stepped forwards while the player backed away pulling, and the hands did not hold the thing. The user
play-tested the mechanism as OK; the animation needs adjustment.

## Decisions (the user)

* The pose is between standing and the crouch, not the crouch: bent over a thing too heavy to lift.
* No weapon is held while dragging: both hands are full with the thing dragged.

## Plan

* New clips on the archeologist (`tools/blender/models/archeologist.py`, `DRAG_CLIPS`, `drag_pose`): the knees half
  bent, the trunk bent far forward from the hips, both arms reaching down to the thing in front, the head up; a still
  hold (`_drag`, a strained breath), a pull (`_drag_pull`, the stride run backwards), a push (`_drag_push`) and standing
  to bent over (`_drag_down`, its frame from `Player::crouchPhase` like `_crouch_down`).
* `ModelState::Drag`, `DragPull`, `DragPush`, `DragDown` (`src/sim/model_info.h`, `PLAYER_CLIPS`). The mechanism stays
  as play-tested: dragging keeps `crouched` (the hitbox, the walk speed); `Player::UpdateCrouchPose` shows the drag clips
  over the crouch's by `Player::BentPose`, pulling when the last step (`Dungeon::walkDir`) went away from
  `DragFacing()` (`Dungeon::DragPulling`).
* `drawWeapon` draws no weapon while `Dungeon::Dragging()`.
* Scenario `tests/scenarios/solved/drag_animation.txt`: holding, pulling, pushing, held still, let go.

## Play-test

`bin/game --play tests/play/drag-animation.txt`
* R at the start: you bend half way over the stone (not down in the crouch), both hands on it, no weapon in hand.
* Walk right: you step backwards pulling it; walk left: you step forwards pushing it.
* R again: you stand up, the weapon back in your fist.
