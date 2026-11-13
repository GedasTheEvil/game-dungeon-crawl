# Drag mirrors along the flat corridor

Status: draft 2026-11-13, size M, needs nothing first.

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

## Play-test

*
