# Sub-cells: where the checker and the game disagree

Status: implemented 2026-11-05, waiting for the user's play-test. Was: draft 2026-11-04, size M, needs nothing first. Found while shaping the campaign ([subcell-campaign](subcell-campaign.feature.md)): `levelcheck` passed these, the `--script` path then failed in the game. Each was worked around in the levels; the checker (or the game) should agree.

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

## Done

Each case is a small level in `tests/unit/subcell_mismatches_test.cpp`: the checker's verdict, then the script
`levelcheck --script` writes for it, played in the sim. The script moved from `tools/level/levelcheck.cpp` to
`src/world/path_script.cpp` (the level library) for that. The checker now reports per path cell its jumps
(`PathTakeOff`: take-off sub-cell, way, landing, distance to the step's face) and the arrival height (`pathRise`).

* Jumps onto a step 2 to 5 sub-cells off: the game reaches a quarter step's face up to about 1.55 tiles from the
  player's middle (jump with the walk key held), more than the checker's farthest take-off needs. The checker was
  right. The script jumped from the cell's grip (0.45) and kept the facing of its last walk, so after a walk the other
  way (to a lever, down a shaft) the jump went backwards. Fixed in the script: it runs up to the take-off sub-cell's
  middle, or against the face when it is right in front, and turns the jump's way (`hold <way> 1`) first.
* A jump across a key gate's cell: the gate opens only when the player comes up to it with the key. Fixed in the
  script: it walks up to each closed gate on the arc (not one a pulled lever opened) and waits for it before the run-up.
* Off a ladder onto a step beside it: the game lets the player step off once they have climbed to the step's height.
  The checker was right. Fixed in the script: `walk up` to the step's height first.
* A crawl space beside a ladder: in a ladder's cell the down key climbs, and the body (`Player::HalfWidth`, under a
  sub-cell) meets the crawl space's ceiling while the middle is still in that cell. Fixed in the checker: no crawl into
  a crawl space from a ladder's cell (`Walker::intoCrawlOffLadder`). A player who steps back off the ladder, crouches
  and crawls across its cell gets in all the same; the checker does not count that. Two more campaign levels had one:
  lvl11 (col 14, row 15) and lvl15 (col 11, row 12), now with the crawl space's sub-cell column next to the ladder
  open floor to ceiling.
* A one-cell stair: the game climbs it, a jump per tread. The path merged the three steps inside the cell into one
  path cell, so the script jumped once. Fixed: the cell keeps every jump, and the script makes each, then ends the walk
  on the sub-cell it landed on, not the cell's grip (back down the stair).
* After a drop down a shaft: the facing, as above.

`make paths`: 54/54. The workarounds in the campaign levels stay (they play well).

## Play-test

`bin/game --play tests/play/subcell-checker-mismatches.txt`

* lvl11: climb down the ladder, walk left one step, crouch (down) and crawl left through the crawl space.
* From the ladder's foot, crouch right away: the player does not crouch (still the ladder), walking left stops at the
  crawl space; one step left of the ladder the crouch works.
