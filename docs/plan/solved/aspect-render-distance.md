# Render distance from the window's aspect ratio

Status: draft 2026-10-01. Done 2026-10-01, verified in play (see [Implemented](#implemented)).

## The problem

The drawn part of the level is a fixed `ViewWindow` (`dungeon.h`): `WIDTH` 10 x `HEIGHT` 6 cells, from 3 cells
left/above the player (`Dungeon::view()`). The projection is `gluPerspective(45, resX / resY, ...)` (`draw.cpp`),
so the vertical field of view is fixed and the horizontal one grows with the aspect ratio.

The game is windowed, so any size is possible. At a wide window (seen at 1920 x 300) the camera sees past the
window's columns: at least two empty cells on each side. A tall window would do the same top and bottom.

## Idea

Size the view window from the aspect ratio (and the camera's distance and field of view), not from constants:
enough columns and rows to fill the frustum at the scene plane, plus a margin cell. Recompute on window resize.

## Things to check

* `ViewWindow::WIDTH` / `HEIGHT` are `constexpr`; every user of `view()` (render, decor, mechanisms, arrows,
  boss, monsters, `dungeon_base.cpp`) must take run-time sizes.
* The 3-cell offset in `view()` keeps the player off-centre; derive it from the new size.
* Gameplay hangs on the view: `spawnInView()` and `inView()` cull and spawn monsters by it. A wider window must
  not spawn or wake monsters earlier than a normal one. Option: keep a fixed gameplay window, widen only the
  drawn one (monsters outside the gameplay window are still drawn if active, or simply not spawned).
* `MUMMY_WAKE_RANGE` ("in view, about 2 tiles each side") assumes the current view.
* Cap the size: a very wide window should not draw half the level (cost, and level edges).
* Alternative or complement: clamp the aspect ratio (letterbox) beyond some limit.
* Scenario test: start at 1920 x 300 and at a tall size, screenshot, no empty cells at the edges.

## Implemented

Decided 2026-10-01 while implementing:

* Two windows. The **gameplay window** stays `ViewWindow` (10 x 6, now in `src/world/view_window.h`): monsters spawn
  in it, the journal sees them in it, the mummy wakes by it. A wide window changes nothing in play.
* The **drawn window** (`drawnWindow`, `src/world/view_window.cpp`) is computed each frame in `Dungeon::Draw` from the
  projection x modelview matrix: the screen corners' rays meet the rock face (z 0) and the back wall (z -tile), the
  cells they span plus one margin cell. It is never smaller than the gameplay window, so a normal window (16:9 sees
  about 5 x 3 cells) draws as before. Resizing and the turned camera (`rotM` / `rotN`) need nothing extra.
* Cap: 12 columns and 4 rows past the gameplay window on each side (`DRAWN_EXTRA_COLS`, `DRAWN_EXTRA_ROWS`). A ray
  that misses a plane (camera turned at a very wide window) takes the cap on its side. Past the cap it is black:
  seen only at about 6:1 with the camera turned all the way.
* Monsters are drawn in the drawn window if they are active; they still spawn in the gameplay window only. At a very
  wide window a monster tile at the edge shows nothing until the player comes within the gameplay window. Accepted.
* Flames and their lights follow the drawn window (lights 2 cells past it, as before; `Lighting` keeps the 16 nearest
  the player).
* Also: cells outside the level are drawn as rock, not left black (a level's edge was black at 16:9 too, at the start
  of level 3).
* Tall windows: `gluPerspective`'s field of view is vertical, so a tall window sees fewer columns, not more rows.
  Nothing to do.
* Tests: `tests/unit/view_window_test.cpp` (16:9, 1920 x 300, the cap), scenarios `aspect_wide.txt` (1920 x 300,
  turned camera) and `aspect_tall.txt` (400 x 1000).

