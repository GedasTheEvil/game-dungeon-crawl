# Mouse look fixes

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

## Problems

1. Moving the mouse while the riddle scroll is open still turns the camera: `processMousePassiveMotion`
   (`src/input/input.cpp`) has no early return for the riddle screen. The view has moved when the scroll closes.
2. The camera jumps when the cursor leaves and re-enters the window: `processMouseEntry` does nothing, so the first
   motion after re-entry is a big delta.

## Plan

* Ignore mouse look on every screen that is not gameplay (riddle included); check how the other screens already
  return early and use the same test.
* On window entry (and on focus gain, on returning from any screen), reset the last mouse position so the next
  motion has zero delta.
* Unit test if the input code allows it; otherwise a short note in the plan how it was checked.
