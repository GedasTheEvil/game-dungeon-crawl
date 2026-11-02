# Level format overhaul: sub-cells, steps and crawl spaces

Status: draft 2026-11-02, needs nothing first. From the user. Large: several phases (format, physics and drawing,
crouch, monsters, checker, campaign); the implementer may split it into drafts that need each other.

## Idea (from the user)

Each cell is large next to the player. Split a cell into smaller parts (4 or 8), so a level stops being a flat
corridor: parts you can (and must) jump onto, and low places where the player has to crouch (a new action and
animation).

## Scale today

* A cell is 1 tile (`TILE_SIZE`, 40 GL units). The player is about 0.375 tiles tall (`PLAYER_CLIMB_HEADROOM`), so a
  cell is about 2.7 player heights.
* A jump peaks at 0.45 tiles and carries about 1 cell forward (`Jump::ARC`, `src/world/movement.h`). The checker
  relies on "a jump cannot step up one cell" (`static_assert` there) and moves by whole cells.
* Walkers follow the player along their row; objects, fixtures, traps, the map and the checker all work per cell.

## Sub-grid

Every step the player has to jump onto must be lower than the jump's peak (0.45 tiles), so the vertical step is a
quarter tile. Proposal: **2 x 4 = 8 parts** a cell (half a tile wide, a quarter tile tall).

* A quarter step (0.25) is a jump up; two quarters (0.5) is out of reach of a plain jump: a wall, or a ladder.
* A crawl space is a gap 1 quarter tall (0.25) under a low ceiling: too low to stand (0.375), high enough to crouch.
* 2 x 2 (half tile each way) is too coarse: a half step is above the jump's peak, so nothing could be jumped onto.

## Data model

* Keep the 40 x 47 cell grid as the logic grid: rows, walkers, objects, fixtures, traps, decor, the map and the
  checker stay per cell.
* Add a **shape** to each cell: an 8-bit mask of solid sub-blocks (2 columns x 4 rows). Structure `Wall` = all
  solid, `Empty` = none; a partly solid cell is `Empty` (or water, sand) plus a mask. Only open cells take a mask.
* Derived per cell, for the code that only asks simple questions: the floor height under each half (the highest solid
  sub-block from the bottom), the ceiling height over each half. Most game code reads these, not the mask.
* An object or fixture in a cell with a shape stands on that cell's floor (the half it is in); a ladder, gate or door
  needs a full-height opening (the checker warns otherwise).
* Glossary: "sub-cell" (one of the 8), "shape" (the mask), "step" (a quarter-tile rise), "crawl space" (a passage
  under a low ceiling). Check for clashes first: "step" is used for the walk step in code (`PLAYER_MOVE_STEP`).

## File format

* Level version 6 (`LEVEL_VERSION`, `src/world/level.cpp`): a new shape layer after the existing ones, one hex
  digit pair a cell, `00` for none. v2-v5 files read with every shape at none, so the levels load unchanged.
* Save games store the level as loaded; old saves can be ignored (AGENTS.md).
* Editor (`tools/editor/`): a shape palette (the common shapes: low step left/right/full, two-step stair, low
  ceiling, ledge lip) plus painting single sub-blocks; the structure drawing in `content/levels/` shows a shaped cell
  with a free glyph (AGENTS.md, Glyphs).

## Physics (player)

* Collision against sub-blocks: walking into a quarter step stops the player (no auto step-up: the step is a jump,
  as the user asks). Open point: a tiny auto step for a smaller rise, if a finer grid is picked.
* Landing on a step: the fall stops at the floor height of the half the player is over.
* Head room: a jump under a low ceiling stops at the ceiling (bonk); the player cannot stand up under one.
* Walking off a step drops a quarter tile: a short fall, no damage.
* Keep `FALL_MAX_STEP` below a quarter tile (today 0.3 > 0.25), so one tick never skips a step's floor.

## Crouch (new action)

* A key (rebindable, default to pick: C or Left Ctrl; check the options screen's bindings). Hold to crouch.
* Crouched: about 0.6 of the height (below 0.25 tiles), walk at half speed, no sprint, no jump; it stays crouched
  under a low ceiling even when the key is let go.
* Hitbox: lower, so a high missile or a swing at head height misses (open point: which attacks; keep it simple).
* New archeologist clips in Blender 5 ([modeling.md](../modeling.md)): crouch idle, crouch walk, the change between
  stand and crouch.
* HUD / interact: the interact prompt works crouched; climbing a ladder needs standing.

## Monsters

* Each `MonsterKind` gets a height (in quarters) and whether it can take a step: small walkers (rats, scarabs) fit
  through crawl spaces, large ones (mummy, hippo, crocodile, Anubis) do not, so a crawl space is an escape route. Walk-jumpers and leapers jump a step; plain walkers stop at it like at a wall.
* Bosses follow their kin (AGENTS.md, Boss abilities): a boss of a step-taking kin takes steps too; a boss of a small
  kin may be too large for crawl spaces (that is fine; say so in the kind).
* Flyers: a low ceiling lowers their flight band.

## Drawing

* `Dungeon::Draw`: a solid sub-block is drawn as a block with the cell's wall texture (the zone look), its top as
  floor, its underside as ceiling. Decor, torches and decals of a shaped cell sit on its floor height or are skipped
  where they do not fit.
* Map screen: draw the mask at the map's cell size (a step as a filled strip).
* Beams (`traceBeams`), arrows, water: a solid sub-block stops them where it is; water fills a shaped cell above its
  floor.

## Checker

* The walker (`src/world/level_check.cpp`) gets step edges: up a quarter step by a jump, down by walking off, through
  a crawl space crouched. Replace the `static_assert` on "no step up one cell" with one on "a jump clears a quarter
  step and not a half".
* New warnings: a rise of 2+ quarters with no ladder (unreachable), a ladder, gate or door in a cell too low for it,
  a crawl space a monster spawn is too large for on its only path, a shape in a wall cell.

## Campaign

* No regeneration (AGENTS.md, Campaign levels). Add steps and crawl spaces by hand: first a test level
  (`tests/levels/subcells`), then a few campaign levels where they make the route more interesting (a stepped hall,
  a crawl space as a shortcut or escape from a large monster). Every touched level passes `bin/levelcheck` with no
  warnings.

## Tests

* Unit: v5 file loads with no shapes, v6 round-trip; floor/ceiling heights from a mask; jump onto a quarter step,
  not onto a half; crouch under a low ceiling, cannot stand up; checker step edges and warnings.
* Scenario: walk into a step (blocked), jump onto it, crouch through a crawl space (screenshots).
* Play script `tests/play/level-subcells.txt` on the test level.

## Open points

* Grid size: 2 x 4 (proposed) vs 4 x 4 (finer horizontal steps, 16-bit mask) vs 2 x 2 (too coarse, see above).
* Mask vs a simpler height profile (floor and ceiling height per half): the profile covers steps, ledges and low
  ceilings but no floating blocks; the mask is more general for the same cost.
* Auto step-up for small rises: no with a quarter grid (every step is a jump).
* Which attacks a crouch dodges.
* How many campaign levels get steps, and which.
