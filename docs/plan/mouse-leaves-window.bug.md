# Mouse leaves the window during mouse look

Status: implemented 2026-11-08, waiting for the user's play-test. Was: draft 2026-11-08, size S, needs nothing first.

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

## Done

* `MouseLook::Hold` (`src/level/mouse_look.h`): after a looking motion, a pointer more than a quarter of the window off
  its centre is warped back to it (`glutWarpPointer`, `holdPointer` in `src/input/input.cpp`). The warp's own motion
  turns nothing; up to `MOUSE_WARP_STALE` motions queued before it are dropped (no jump back).
* Warps only while the cursor is hidden (`pointerHidden`, the same test as `followCursor`) and the pointer is over the
  window (GLUT entry: left on Alt+Tab or another window on top).
* Only the GLUT callbacks warp (`windowPassiveMotion`, `windowActiveMotion`); a scenario calls the process functions,
  no warp.
* Tests: `tests/unit/mouse_look_test.cpp` (5000 px past the edge still turns, the warp and stale motions turn
  nothing). Checked live under xvfb with xdotool: a 3600 px swing keeps the pointer near the centre.

## Fixed when

* A test that names the plan fails before the fix and passes after it (a unit test on the look/warp logic: a motion
  far past the edge still turns, the warp's own event turns nothing).
* Playing windowed, mouse look never lets the pointer out of the window; menus still have a free pointer.

## Play-test

* `bin/game --level 1` windowed: swing the mouse far left and right; the view keeps turning, the pointer never shows
  outside the window, a click never focuses the desktop.
* Open the inventory (I): the pointer shows and moves freely; close it: no camera jump.
