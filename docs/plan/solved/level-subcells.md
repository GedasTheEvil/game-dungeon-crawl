# Sub-cells: the level format, editor and map

Status: solved 2026-11-03 (play-tested by the user). Was: implemented 2026-11-02, waiting for the user's play-test (`bin/game --level tests/levels/subcells`, M for the
map; the editor's structure palette). Was: draft 2026-11-02, needs nothing first. From the user.

Implementer's choices:

* The shape layer is a list like the other four (`shapes COUNT`, then `COL ROW MASK`, 4 hex digits), not a grid of
  `0000`s: the diff of every level stays one line, and the structure drawing already shows where the shapes are.
* The structure drawing has one shaped glyph per open structure (`SHAPED_GLYPHS`: `_` empty, `-` half water, `;`
  sand, `&` quicksand), so a shaped cell keeps its structure. The glyph and the list must agree (a load error
  otherwise).
* The shape queries read the structure and the shape only (gates and wall spikes stay `isSolidTile`'s). `isFloor`,
  `floorUnder`, `heightOverFloor`, `bodyBlocked` and the checker are unchanged: a shaped cell walks and checks as open
  until [subcell-physics](subcell-physics.md). So does "an object or fixture in a shaped cell stands on the
  floor under the cell's middle": it needs the drawing (`floorHeight(t, 0.5f)` is the height), left to that plan.
* Editor: the shape brushes sit in the structure palette after the six structures (no seventh layer button: the
  button row is full). "Ledge lip" became `Lip L` / `Lip R`: a rim a quarter high and wide at the cell's edge.
* Every campaign level converted to v6 (`levelconvert`: a `shapes 0` line; it also put a few hand-ordered list lines
  in file order).

The first of the sub-cell drafts; the others build on it: [subcell-physics](subcell-physics.md),
[crouch](crouch.md), [subcell-monsters](subcell-monsters.md),
[subcell-checker](subcell-checker.md), [subcell-campaign](../subcell-campaign.md).

## Idea (from the user)

Each cell is large next to the player. Split a cell into smaller parts, so a level stops being a flat corridor:
parts you can (and must) jump onto, and low places where the player has to crouch (a new action and animation).

## Scale today

* A cell is 1 tile (`TILE_SIZE`, 40 GL units). The player is about 0.375 tiles tall (`PLAYER_CLIMB_HEADROOM`), so a
  cell is about 2.7 player heights. Walls stop the player's centre 0.25 tiles off (`PLAYER_BODY_HALF_WIDTH`).
* A jump peaks at 0.45 tiles and carries about 1 cell forward (`Jump::ARC`, `src/world/movement.h`).
* Walkers follow the player along their row; objects, fixtures, traps, the map and the checker all work per cell.

## Sub-grid (from the user: 4 x 4)

A cell splits into **4 x 4 = 16 sub-cells**, a quarter tile each way. A quarter step (0.25) is below the jump's peak
(0.45), so it is a jump up; two quarters (0.5) are not.

## Data model (from the user: a mask)

* The 40 x 47 cell grid stays the logic grid: rows, walkers, objects, fixtures, traps, decor and the checker stay per
  cell.
* Each cell gets a **shape**: a 16-bit mask of solid sub-cells (bit = column + 4 x row, row 0 at the bottom, as the
  level grid). Structure `Wall` = all solid, `Empty` = none. A partly solid cell is an open structure (empty, half
  water, sand) plus a mask; a `Wall` or `DeepWater` cell takes no mask.
* Queries the game code uses instead of reading bits (`src/world/level.h`): solid at a point in the cell, the floor
  height under an x (top of the highest solid run from the bottom, or 0), the ceiling height over an x.
* Put the shape in `Tile` (`std::uint16_t shape`, next to `structure`) and make the shape queries take a `const Tile&`
  and a position inside the cell. Then the `Dungeon` map, save games, `LevelGrid`, `climb_path`, the checker and the
  editor carry shapes with no change to how they read the grid. The floor rules to change are already named
  ([floor-queries](floor-queries.md)): `isFloor` (`level.h`), `Dungeon::floorUnder`,
  `Dungeon::heightOverFloor`, `Dungeon::bodyBlocked`, the checker's `Walker::floor`.
* An object or fixture in a shaped cell stands on the floor under the cell's middle.
* Glossary: "sub-cell" (one of the 16), "shape" (the mask), "step" (a rise of a quarter tile), "crawl space" (an
  opening too low to stand in). Check clashes first: code uses "step" for the walk step (`PLAYER_MOVE_STEP`).

## File format

* Level version 6 (`LEVEL_VERSION`, `src/world/level.cpp`): a new shape layer after the existing ones, 4 hex digits a
  cell, `0000` for none. v2-v5 files read with no shapes, so every level loads unchanged.
* Old saves can be ignored (AGENTS.md).

## Editor and map

* Editor (`tools/editor/`): a shape palette with the common shapes (a quarter or half step on the left, the right or
  the whole floor, a stair of quarters, a low ceiling of one to three quarters, a ledge lip) and painting single
  sub-cells; readme updated. The structure drawing in `content/levels/` shows a shaped cell with a free glyph
  (AGENTS.md, Glyphs).
* Map screen (`src/ui/map_view.cpp`): draw the solid sub-cells at the map's cell size.
* Drawing in the game comes with [subcell-physics](subcell-physics.md); until then the game draws a shaped
  cell as open.

## Tests

* Unit: a v5 file loads with no shapes; v6 round-trip; floor and ceiling heights and solid-at from masks.
* Test level `tests/levels/subcells` (steps, stairs, a crawl space, a ledge lip); the later drafts extend it.
