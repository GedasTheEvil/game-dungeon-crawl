# Sub-cells: where the checker and the game disagree

Status: draft 2026-11-04, size M, needs nothing first. Found while shaping the campaign ([subcell-campaign](subcell-campaign.feature.md)): `levelcheck` passed these, the `--script` path then failed in the game. Each was worked around in the levels; the checker (or the game) should agree.

* A jump onto a step from 2 to 5 sub-cells away (`JUMP_SUBCELLS`, `level_check.cpp`) often lands short in the game and
  the step's face stops the player (lvl51 after the lever, lvl52 by a pit, lvl27 from the dead-end wall, lvl9 next to
  the lever from a standstill). Either tighten the checker's reach or the script's run-up.
* A jump across a gate's cell onto a step: the checker allows it, the gate blocks it (lvl23, red gate col 7).
* Stepping off a ladder onto a step beside it: the checker allows it at any height in the ladder's cell, the game
  blocks the player at the ladder's foot (lvl28, col 9 row 4).
* A crawl space next to a ladder: down climbs in a ladder's cell instead of crouching (plan crouch), so the crawl
  cannot be entered from there; the checker passes it (lvl30, lvl47).
* A one-cell stair (`Stair L`, 0137) the checker passes blocks the player in the game (lvl48).
* `--script`: after a drop down a shaft it jumps at once while still facing the old way and lands short of a step two
  cells away (lvl38).

Each: a unit test on a small test level (the checker's verdict next to the game's), then the fix on the side that is
wrong. Every campaign level must still pass with no warnings (`make paths`).
