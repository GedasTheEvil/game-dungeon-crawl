# Floor queries: one place that says where the floor is

Status: draft 2026-11-02, needs nothing first. From a code review for the sub-cell drafts. A refactor: no change to
gameplay, visuals or levels (once implemented, straight to "solved/", docs/plans.md).

## Why

[subcell-physics](subcell-physics.draft.opus.md) lets the player and monsters stand on a quarter step inside a cell.
Today the code assumes the floor of the player's cell is always the cell's bottom edge (`mapY` a whole row), and
asks "is there a floor under this cell" inline, with the same expression in many files. Each place would need the
sub-cell rule on its own. Name both now, behaviour unchanged, so the sub-cell drafts change two or three functions
instead of about twenty call sites.

## 1. The player's floor (`Dungeon`, `src/world/dungeon_base.cpp`)

The rule "a fractional `mapY` means in the air over the floor of row `floor(mapY)`" is written out in:

* `UpdateMovementState`: the snap to the nearest row (`FALL_START_THRESHOLD`), the fall start, the landing
  (`floorY = std::floor(mapY)`), the settle (`mapY = std::floor(mapY)`).
* `PlayerSink`: `above = mapY - row`.
* `PlayerOnLadder`: `mapY - std::floor(mapY) >= STANDING_EPSILON` with `floorBelow`.
* `Move`: the side probe tests one point at `mapY` (`isSolidTile(Map(probeX, mapY))`), not the body.

Add to `Dungeon` (private, next to `MapAt`):

* `float floorUnder(float x, float y) const`: map y of the floor the player at (x, y) would stand on in their cell:
  today `std::floor(y)` when the cell below is solid. Doc comment: sub-cells will raise it inside the cell.
* `float heightOverFloor() const`: `mapY - floorUnder(mapX, mapY)`, for the sink, the ladder check and the fall
  start.
* `bool bodyBlocked(float x, float y) const`: the player's body at (x, y) hits something solid: today the two
  `isSolidTile` tests of `Move` (the cell the player is in and the probe cell). The side of the body stays the
  caller's (`probeX`), as now.

Route the listed sites through them. The float rounding the snap fixes (the comment at
`dungeon_base.cpp:83`, 38.9999 for 39) must stay fixed: keep the snap, now to `floorUnder`.

Out of scope: the jump (`jump.start_y`, the jump ends at its take-off height) stays as is; subcell-physics decides
how a jump lands on a step.

## 2. "A floor under this cell" (`src/world/level.h`)

Add `[[nodiscard]] inline bool isFloor(const Tile& t) { return isSolidTile(t); }`: the cell under an open one carries
it. Doc comment: the one place that says what a creature stands on; sub-cells (a cell with its top row solid) change
it here. Use it where the code asks about the cell under another one:

* `dungeon_base.cpp`: `PlacePlayer` (38), and the sites of part 1.
* `dungeon_monsters.cpp`: on the rungs (43), the drop down a shaft (49), the spawn floor (175).
* `dungeon_flight.cpp`: `openBelow` (53).
* `dungeon_balls.cpp`: the roll off an edge (81).
* `dungeon_webs.cpp` (44), `dungeon_blaze.cpp` (14, 22).
* `climb_path.cpp`: `walkerBlockedAt` (27).
* `level_check.cpp`: `Walker::standable`, `settle`, and any other "cell below" test there.

Keep each caller's out-of-bounds rule exactly as it is: `Dungeon::MapAt` does not check bounds (callers test
`IsInBounds` first), `climb_path` reads out of bounds as "no floor", `LevelGrid::at` reads it as wall. The helper
takes a `Tile`, so it does not change them.

Line numbers are from 2026-11-02; grep `isSolidTile(.*- 1)` for the current ones.

Not here: `level_gen.cpp` (the generator; levels are never generated again unless asked) and `decor_scatter.cpp`
(its own `floorAt` / `rockAt` read walls for the drawing, a different question: is there stone to draw).

## 3. Note for level-subcells

Put the shape in `Tile` (`std::uint16_t shape`, next to `structure`) and make the shape queries take a `const Tile&`
and a position inside the cell. Then the `Dungeon` map, save games, `LevelGrid`, `climb_path`, the checker and the
editor carry shapes with no change to how they read the grid. (The review found three ways to read the grid: the
`Dungeon` map array, `climb_path`'s own `at` / `inBounds`, `LevelGrid`. Merging them is not worth it on its own.)
Add this line to [level-subcells](level-subcells.draft.opus.md) when implementing this plan.

## Tests

* No new behaviour: `make check` and `make test` (all scenarios) pass unchanged. In particular
  `tests/unit/movement_test.cpp`, `climb_test.cpp`, `water_test.cpp`, `sand_test.cpp`, `screech_test.cpp`.
* Unit: `floorUnder` on a floor, over a gap, on a ladder; `heightOverFloor` mid-jump and after the landing snap;
  `isFloor` for wall, deep water, a closed and an open gate, wall spikes out.
* Every campaign level still passes `bin/levelcheck content/levels/lvl*` with the same report as before (diff the
  output before and after).
* Glossary: "floor" names `isFloor` and `Dungeon::floorUnder` as its code.
