# Render distance from the window's aspect ratio

Status: draft 2026-10-01. Not started.

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
