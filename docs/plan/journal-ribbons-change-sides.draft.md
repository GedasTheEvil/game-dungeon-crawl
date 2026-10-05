# Journal ribbons change sides

Status: draft 2026-10-05. Builds on [journal-real-book.md](journal-real-book.md) (page curl, ribbons).

## Problem

The section ribbons (bookmarks) feel static. They all hang out of the right edge at fixed heights; switching section
only makes the open one stick out a little further. Nothing on the book shows where in it you are.

## Idea

A ribbon click turns the page with the existing page-turn animation, and the ribbons move between the two sides of
the book according to the open section.

Decided 2026-10-05, real-book layout (a ribbon riding a forward turn lands on the left):

* Creatures ribbon: always on the left, never moves. Creatures is the first section, so its ribbon has no page to
  ride on.
* Riddles and Field notes ribbons: on the right while their section lies ahead, on the left once it is open or
  passed.
  * Creatures open: Creatures left; Riddles, Field notes right.
  * Riddles open: Creatures, Riddles left; Field notes right.
  * Field notes open: all three left.
* Clicking a ribbon on the right (a later section) turns the page forward and that ribbon rides over to the left.
  Clicking one on the left (an earlier section) turns it backward, the reverse animation, and the ribbons of the
  sections after it ride back to the right. Clicking the Creatures ribbon turns the pages back; it stays put.
* Decided 2026-10-05: a ribbon that changes sides travels with the turning page (drawn into the page's render
  target, bending with the curl) and lands on the other side with it. That is the point of the animation; no swap
  when the turn ends.
* Each ribbon keeps its height, so it is the same ribbon on either side.
* Decided 2026-10-05: depth. Now all ribbons come out of the book at the same x (`RIBBON_X`), as if they marked
  the same page. Each ribbon sits at its section's depth in the page block instead: a section deeper in the stack
  has its ribbon further out sideways (x), following the stacked page edges (`EDGE_STEP`, more pages under = further
  out). Heights (y) stay as they are.

## Open

* Field notes to Creatures is one page turn (one page a ribbon click): both Riddles and Field notes ribbons ride
  back on that one page.
* Left-side ribbons: mirrored texture (swallowtail pointing left); clear of the strap on the left edge?
* Arrow keys / wheel / corner drag crossing a section boundary should move the ribbons the same way.
* Hover label and letter (M / R / F) on left ribbons: label on the left page.

## Notes

* Code: `src/ui/journal_view.cpp` (`ribbonRect`, `DrawRibbons`, `GoTo`, `StartTurn`). Look: [docs/ui.md](../ui.md),
  "Book (journal)".
* Scenario to update: `tests/scenarios/journal_page_turn.txt`.
