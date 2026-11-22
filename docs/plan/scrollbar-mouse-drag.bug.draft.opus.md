# Scrollbars drag with the mouse

Status: draft 2026-11-20, size M, needs nothing first.

## Repro

1. Open a screen with a scrollbar (inventory `src/ui/inventory.cpp` `DrawScrollBar`, skills `src/ui/skills_view.cpp`
   `DrawScrollBar`, any other scrollable screen).
2. Click and drag the scrollbar thumb with the mouse.

## Expected

The thumb follows the mouse and the list scrolls. A click on the track jumps / pages. Same in every scrollable screen.

## Actual

Nothing happens. Only the mouse wheel scrolls.

## Fixed when

* Dragging a scrollbar thumb scrolls the list, on every screen that draws a scrollbar (one shared helper for hit
  test, drag state and offset mapping, not a copy per screen; see [ui.md](../ui.md)).
* A test that names the plan fails before the fix and passes after it (the helper's drag-to-offset mapping).

## Play-test

* Drag the scrollbar in the inventory and the skills screen (and any other screen with one): the list follows.
