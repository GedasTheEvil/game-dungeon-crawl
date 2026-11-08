# Mouse leaves the window during mouse look

Status: draft 2026-11-08, size S, needs nothing first.

## Repro

1. Start a level windowed (`bin/game --level 1`).
2. Move the mouse to look around, keep moving it one way.

## Expected

In gameplay (no screen over the game) the pointer stays in the window: mouse look keeps turning however far the mouse
moves, and a click never lands on the desktop or another window.

## Actual

The cursor is only hidden (`followCursor`, `src/input/input.cpp`: `GLUT_CURSOR_NONE`), never held. Mouse look is the
delta from the last seen position (`MouseLook`, `src/level/mouse_look.h`), so once the hidden pointer reaches the window
edge or leaves it, the look stops (`processMouseEntry` calls `mouseLook.Forget()`), and a click goes elsewhere.

## Notes

* GLUT has no grab; the usual way: `glutWarpPointer` to the window centre after each look motion while
  `followCursor` shows `GLUT_CURSOR_NONE`. The warp itself sends a motion event: skip it (the position equals the centre
  just warped to) so it turns nothing; `MouseLook` then measures from the centre.
* Warp only while looking (the same condition as `followCursor`). Menus, inventory, map, journal, riddle, death screen:
  the pointer is free, no warp. Coming back to gameplay must not jump the camera (`MouseLook::Forget` / fresh position).
* Losing the window focus (Alt+Tab) frees the pointer; no warp while unfocused if GLUT tells (entry / visibility).
* Scenario runs (`processMousePassiveMotion` from `src/test/scenario.cpp`) under xvfb: warping must not break them.

## Fixed when

* A test that names the plan fails before the fix and passes after it (a unit test on the look/warp logic: a motion
  far past the edge still turns, the warp's own event turns nothing).
* Playing windowed, mouse look never lets the pointer out of the window; menus still have a free pointer.

## Play-test

* `bin/game --level 1` windowed: swing the mouse far left and right; the view keeps turning, the pointer never shows
  outside the window, a click never focuses the desktop.
* Open the inventory (I): the pointer shows and moves freely; close it: no camera jump.
