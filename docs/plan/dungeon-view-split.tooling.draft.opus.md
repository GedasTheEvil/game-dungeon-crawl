# The dungeon draw members and their state out of Dungeon into a DungeonView

Status: draft 2026-11-13, size L, needs nothing first.

## Problem

Plan sim-library made the `Dungeon` a GL-free sim class, but its drawing stayed a part of the class: the
`src/graphics/dungeon_render*.cpp` files (10 files, about 80 `Dungeon::` definitions) define member functions that
`src/sim/dungeon.h` declares. So the sim's central header (1078 lines) carries the game's drawing:

* 71 draw declarations (`grep -cE "^\s*(static )?void (draw|Draw|add[A-Z][a-zA-Z]*Lights|pushLevelFrame|queueContour)|drawPoisonDrips" src/sim/dungeon.h`):
  `DrawMonsters`, `DrawTreasureTile`, `drawWebs`, `addSunLights`, ...
* State only the drawing uses: `contours` (a `std::function<void(int texture)>` per frame, `dungeon.h:146-162`),
  `itemParts` (an `Item*` and a modelview matrix, 373-378), `drawnCells` (389), and the animation counters
  `portalTimer`, `portalScroll`, `riddleMarkYaw`, `treasureSpin` (750-753), which the sim's own `Update` advances
  (`src/sim/dungeon_base.cpp:207-210`).
* `SimLinks::assets` (`src/sim/sim_links.h:21`): the app's `Assets` in the sim's links, "only the drawing reads them".

What it costs:

* **Rebuilds.** `dungeon.h` is included by 37 files, 25 of them the sim's (`grep -rln '#include ".*dungeon.h"' src
  tools tests/unit`). Of the 71 commits that changed `dungeon.h` between 2026-10-15 and 2026-11-06 (then
  `src/world/dungeon.h`), 26 changed a draw line: each one rebuilt the whole sim library and the unit tests for a
  change to the drawing.
* **Reading.** A feature's sim and drawing members sit interleaved in one class (a web: `webs`, `updateWebs`,
  `drawWebs`, `dungeon.h:530-552`), so the sim's interface is hard to see, and `check_sim.sh` cannot tell a draw
  member from a rule.
* The sim ticks visual state (`treasureSpin += 1.f` per update): the drawing's speed is tied to the sim's tick.

## Plan

Incremental: one `dungeon_render*.cpp` file per commit; the game draws the same after each.

1. `src/graphics/dungeon_view.{h,cpp}`: `class DungeonView` holding a `const Dungeon&` (or taking it per call), the
   render-only state (contours, item parts, drawn cells, the portal scroll, the riddle mark's yaw, the treasure spin)
   and the draw members. `Dungeon` declares `friend class DungeonView;`, so the moved bodies keep reading its private
   state unchanged (a later step may replace that with const accessors where few are needed).
2. Move the members file by file: `dungeon_render_webs.cpp` (1 member) and `_blaze`, `_drips`, `_outlines` first,
   `dungeon_render.cpp` (24) last. In each file `Dungeon::drawX` becomes `DungeonView::drawX`; the calls from
   `graphics/draw.cpp` go through the view (`GameState` owns one, as it owns `PlayerView`).
3. The animation counters advance in the view, from the frame's time (`GameClock::now()`), not in `Dungeon::Update`.
   Check the speeds stay the same (one step per 16 ms tick today).
4. When no draw member is left: drop `class Item;`, `<functional>` if unused, and `SimLinks::assets` (the view gets
   `Assets` from `GameState`).
5. docs/development.md ("Libraries", "Where is X"): the dungeon draws itself in `DungeonView`.

## Risk

Medium, mechanical: no rule changes, but many lines move. Screenshot scenarios (`make test`) are the check: each step
must leave them byte-identical. `check_layers.sh` and `check_sim.sh` still pass (the view is in the game, not in a
library).

## Verify

* `make check`, `make test` after every step.
* At the end: `grep -c "draw\|Draw" src/sim/dungeon.h` near zero; `touch src/graphics/dungeon_view.h && make` rebuilds
  no file under `src/sim/`.

## Open questions

* `friend` vs const accessors: `friend` keeps the move mechanical; accessors document what the drawing reads but add
  many one-line getters. Implementer's choice, per file.
