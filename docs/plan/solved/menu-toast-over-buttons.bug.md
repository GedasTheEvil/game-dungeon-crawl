# Main menu toast painted over the buttons

Status: solved 2026-11-09 (play-tested by the user). Was: implemented 2026-11-08, waiting for the user's play-test. Was: draft 2026-11-08, size S, needs nothing first.

## Repro

1. In game, Esc to the main menu, Save Game, pick a slot.
2. The menu returns to the main page with "Saved to slot N".

## Expected

The toast sits in a free spot of the main page, readable, not over a button. It reads like part of the shared UI look
([ui.md](../../ui.md#footer-and-toast)), e.g. in the footer area above the hint line, or on a small panel of its own.

## Actual

`MainMenu::DrawFooter` (`src/ui/menu.cpp`) draws the toast at y 12 on the main page (5.6 on a sub-screen), bare text
with no backing. On the main page y 12 falls on the button column, so the text is painted across a button. Same for the
other main-page toasts ("Could not load the newest save", ...).

## Notes

* Check the layout table in [ui.md](../../ui.md) (rows by y) for the free band on the main page; move the toast there and
  update that table row and the "Footer and toast" section.
* If no band is free, give the toast a backing (the shared panel / tile style) so it never reads as painted on.
* Sub-screen toasts (under Back, 5.6) only if they overlap something too; check with a screenshot.

## Fixed when

* A scenario that names the plan saves to a slot and screenshots the main page: the toast clears every button
  (`tests/scenarios/`, `make test SCENARIO=...`). It shows the overlap before the fix.

## Play-test

* `bin/game --play tests/play/menu-toast-over-buttons.txt`: Esc, Save Game, pick a slot: "Saved to slot N" shows in the hint line under the menu panel, clear of the buttons, readable.
