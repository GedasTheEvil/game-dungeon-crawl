# Sub-cells: the level checker

Status: solved 2026-11-04. Was: draft 2026-11-02. Needs first: [crouch](crouch.md) and
[subcell-monsters](subcell-monsters.md). From the user.

Implementer's choices:

* Nodes (`Nodes`, `fineCell` in `level_check.cpp`): a fine cell (shaped, or over a shaped cell whose top row is solid
  in part) has one node per standing place, a sub-cell over a solid one or on its bottom row; every other cell stays
  one node, so a level with no shapes gives the same report as before (all campaign and test levels diffed
  byte for byte against the old checker). The moves are worked out on sub-cells for every cell; a whole cell is left
  from its side sub-cell column.
* The body: two sub-cells standing, one crouched (`PLAYER_BODY_HEIGHT`, `static_assert`). A crawl is a walk into an
  opening a quarter high: `PathMove::Crawl`, `COST_CRAWL` 1 per sub-cell on top of the walk; a move inside one cell
  costs no walk and no hazard. The path lists cells (the sub-cells of one cell fold into one entry, a crawl in it
  marks the way in); `LevelReport::pathSteps`, `pathCrawls`; difficulty 0.2 a step, 0.1 a sub-cell crawled.
* Steps: `PathMove::Step` (`COST_STEP` 3), from the sub-cell beside the step with three quarters of head room; also
  from 2 to 5 sub-cells away (`JUMP_SUBCELLS`) with the arc's whole room: the arc's feet are over a quarter from
  about 0.32 to 1.37 tiles out (`Jump::ARC`: peak 0.405, walking reach 1.69). That keeps `tests/levels/subcells`
  passing: its step in half water (col 19) is jumped onto from the dry floor of col 17, over the water (the game
  allows no jump while wading, `Dungeon::JumpAllowed`).
* Gaps: a gap is a fall under the take-off's cell row (a lower floor in the same cell is not one). The jump lands on a
  floor no lower than that row's bottom, so a jump off a step comes down beside it; a jump from the floor lands
  level, as before. The whole arc needs a cell of head room (`JUMP_ROOM`).
* A ladder is never fine: its cell stays one node, climbed off onto a step beside it at any height in the cell.
* Warnings: an unreachable step top is one 2 or 3 quarters over a reached floor beside it that no state reaches
  (`checkStepTops`); a chest needs half a tile of room over its floor in the cell's middle (`roomToStand`); a crawl
  space warns at a drop of a whole cell or a trap where it ends (`checkCrawlEnds`, not noisy on the test levels);
  a prop needs a flat floor with nothing over it, a decal or torch the anchor's sub-cells free (floor: bottom row,
  ceiling: top row, others and torches: the middle 2 x 2).
* Left out: a step narrower than the player's body. The hitbox is about 0.14 tiles wide (`Player::HalfWidth`, about
  0.07), under a sub-cell's 0.25, and the foot stands on any part of a step: the warning could never fire.
* Monsters: `monstersNear` counts a spawn only if `shapePasses` lets its body through each cell to the path cell.
* `tests/levels/monster_head_bump` now warns about its entrance under a low ceiling (a test level, left as is: its
  play script starts the player there). Unit tests: `tests/unit/subcell_checker_test.cpp`.

The checker's walker (`src/world/level_check.cpp`) moves by whole cells; it learns the sub-cell moves.

* Edges: up a quarter step by a jump, down by walking off, through a crawl space crouched (slower, a higher cost), a
  one-cell gap jumped from a step's height as from the floor (the reach from `Jump::ARC`).
* Monster reach: a large monster cannot follow through a crawl space; a spawn is cut off where its kind cannot pass.
* New warnings:
  * a rise of 2 or more quarters with no ladder on the only way (unreachable);
  * a ladder, gate, door or treasure in a cell without full head room;
  * a placed decor or decal that does not fit the shape;
  * a step narrower than the player's body (standing on it is not possible);
  * a shape in a `Wall` or `DeepWater` cell;
  * a crawl space that ends at a drop or a trap the player cannot see crouched (open point: drop this one if noisy).
* Docs: the sub-cell rules in [levels.md](../../levels.md), next to the water and sand rules.
* Every campaign level still passes with no warnings (none has shapes yet); `tests/levels/subcells` passes.
* Unit tests for each edge and warning.
