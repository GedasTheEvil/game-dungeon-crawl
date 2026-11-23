# Scrollbars drag with the mouse

Status: implemented 2026-11-23, waiting for the user's play-test. Was: draft 2026-11-20, size M, needs nothing first.

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

## Done

* `src/level/scroll_bar.h`: `ui::ScrollBar` (track, rows, rows shown; `Thumb`, `ScrollAt`, `Hit` with `GRAB_PAD`
  beside the thin track) and `ui::ScrollDrag` (press on the thumb holds it where it was grabbed, press on the track
  pages by the rows shown, motion follows by whole rows, release lets go). The thumb draws lighter while held.
* Used by every bar: inventory grid, Heka cards, credits' second page (menu, `MainMenu::CreditsBar`) and the level
  editor's palette, which had only the wheel and the edge arrows and now draws a bar too (its grid sits 0.6 left).
* Tests: `tests/unit/scroll_bar_test.cpp` (mapping, paging, every bar clear of its slots), scenario
  `tests/scenarios/scrollbar_mouse_drag.txt` (inventory and Heka dragged, Heka paged).

## Play-test

`bin/game --play tests/play/scrollbar-mouse-drag.txt`

* I: drag the weapons grid's thin bar right of the slots down and up: the rows follow; click the bar above / below
  the thumb: it pages.
* K: the same on the Heka cards' bar.
* Esc, Main Menu, Credits, More: drag the blue bar of the credits text.
* `make run-editor`, Decor layer: the palette has a bar at its right; drag it.
