# Journal ribbons change sides

Status: draft 2026-10-05. Builds on [journal-real-book.md](journal-real-book.md) (page curl, ribbons).

## Problem

The section ribbons (bookmarks) feel static. They all hang out of the right edge at fixed heights; switching section
only makes the open one stick out a little further. Nothing on the book shows where in it you are.

## Idea

A ribbon click turns the page with the existing page-turn animation, and the ribbons move between the two sides of
the book according to the open section.

* The open section's ribbon is on the right.
* Ribbons of the sections before it are on the right too; ribbons of the sections after it are on the left.
  * Creatures open: Creatures right; Riddles, Field notes left.
  * Riddles open: Creatures, Riddles right; Field notes left.
  * Field notes open: all three right.
* Clicking a ribbon on the left (a later section) turns the page forward; clicking one on the right (an earlier
  section) turns it backward, the reverse animation.
* Decided 2026-10-05: a ribbon that changes sides travels with the turning page (drawn into the page's render
  target, bending with the curl) and lands on the other side with it. That is the point of the animation; no swap
  when the turn ends.
* Each ribbon keeps its height, so it is the same ribbon on either side.

## Open

* Direction: as described by the user. A physical book would be the mirror (bookmarks of read sections end up on the
  left page block, those ahead on the right). Confirm before building.
* Left-side ribbons: mirrored texture (swallowtail pointing left), hanging out past the strap?
* Arrow keys / wheel / corner drag crossing a section boundary should move the ribbons the same way.
* Hover label and letter (M / R / F) on left ribbons: label on the left page.

## Notes

* Code: `src/ui/journal_view.cpp` (`ribbonRect`, `DrawRibbons`, `GoTo`, `StartTurn`). Look: [docs/ui.md](../ui.md),
  "Book (journal)".
* Scenario to update: `tests/scenarios/journal_page_turn.txt`.
