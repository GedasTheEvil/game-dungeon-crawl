# Drag mirrors along the flat corridor

Status: solved 2026-11-13 (play-tested by the user). Was: implemented 2026-11-13, waiting for the user's play-test. Was: draft 2026-11-13, size M, needs nothing first.

## Idea

A mirror that cannot be turned where it stands can be dragged to a light shaft. Example: a light shaft 1-2 cells
away from the mirror. Turning the mirror does nothing there. Dragging it to the shaft lets the player activate it.

## Plan

* Key `[r]` toggles dragging on and off. Check for a bind conflict first.
* While dragging, the mirror moves with the player, one cell per step, the player walking behind it.
* Dragging works only on the flat corridor: the cells the player can stand on at full height. It is refused on water,
  sand and subcell stairs (the mirror and the player both stay put). Other cells keep their existing rules.
* Speed while dragging: 75% of the crouch (crawl) speed (`CROUCH_WALK_FACTOR`), so about a quarter of walk speed.
* No attack while dragging. The player can still be hit (same as crouch).
* A mirror is activated (turnable) only when adjacent to a light shaft's beam path, once dropped. Dropping = toggling
  `[r]` off. Activation is kept after dropping, so the puzzle reads as "move it, then turn it".
* Existing levels stay untouched. A follow-up draft will add dragging puzzles once dragging exists.

## Decisions (2026-11-13, with the user)

* Turning: every mirror stays turnable, as before. A mirror off any beam just does nothing useful when turned;
  dragging it into the beam is the puzzle. No new lock, the checker unchanged.
* The pose: the player holds the thing with their hands, bent as in the crouch, and walks backwards first, facing
  the thing being dragged. Built generic: `isDraggable` (only the mirror for now).

## Implementation

* `R` (new binding `drag`, Options > Controls; `R` was free) takes hold of the mirror in the player's cell or the next
  cell they face (centre within `DRAG_REACH`). The player steps to `DRAG_HOLD` (0.25 tiles) from its centre, hands
  on the stand, and crouches facing it. Walking away pulls it, walking at it pushes it; it changes cells when its
  centre crosses a cell edge, drawn smoothly with the player, and slides to its cell's centre when let go.
* Flat corridor (`Dungeon::dragGround`): an `Empty` structure (no water, sand, quicksand), no shape, not solid, on a
  wall cell. The thing's new cell also needs no object, no fixture and no prop the player uses (shrine, brazier,
  stargate, dial). Refused, the player stays put.
* Crouched while dragging: no attack ("Let go to strike"), jump, climb or sprint; hit as crouched. Walk:
  `DRAG_WALK_FACTOR` (0.75) of the crouched walk.
* Lets go by itself when the player dies, jumps, falls, is held, or is out of reach (a teleport, a load).
* A first hint ("Drag the mirror: R (again to let go)") when a mirror is in reach.
* Not done: a dedicated drag clip (the crouch-walk clip plays, also when walking backwards).

## Play-test

`bin/game --play tests/play/mirror-drag.txt` (the test corridor):

* Face the mirror on the left, press `R`: the player crouches at its stand, facing it.
* Walk right: the player backs away pulling it, slowly; it follows smoothly. Under the light shaft press `R` again:
  it settles in its cell; stand at it and turn it right (`E` twice): the beam lights the sun disc, the red gate opens.
* Try to attack or jump while holding it: refused.
* Push the second mirror (far right) into the sand: it will not go, nor will the player.
