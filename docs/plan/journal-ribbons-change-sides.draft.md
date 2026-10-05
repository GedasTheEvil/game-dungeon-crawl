# Journal ribbons change sides

Status: draft 2026-10-05, all points decided. Builds on [journal-real-book.md](solved/journal-real-book.md) (page curl,
ribbons).

## Problem

The section ribbons (bookmarks) feel static. They all hang out of the right edge at the same x, as if they marked the
same page; switching section only makes the open one stick out a little further. Nothing on the book shows where in it
you are.

## Idea

A ribbon marks the first page of its section, like a real bookmark. It rides on that page when the page turns, so it
changes sides with it, and it comes out of the page block at that page's depth.

## Sides

A ribbon riding a forward turn lands on the left (real-book layout).

* Monsters (creatures) ribbon: always on the left, never moves. Monsters is the first section, its first page never
  turns.
* Riddles and Field notes ribbons: on the right while their section lies ahead, on the left once it is open or passed.
  * Monsters open: Monsters left; Riddles, Field notes right.
  * Riddles open: Monsters, Riddles left; Field notes right.
  * Field notes open: all three left.
* A ribbon on the right (a later section): its click turns the page forward, the ribbon rides over to the left.
* A ribbon on the left (an earlier section): its click turns the page backward (the reverse animation); the ribbons of
  the sections after it ride back to the right. A click on the Monsters ribbon turns back; it stays put.
  * Field notes to Riddles: Field notes rides back right, Riddles stays left.
  * Field notes to Monsters: one page turn (a ribbon click turns one page); Riddles and Field notes both ride back on
    that one page.
* Arrow keys, wheel and a corner drag do the same when the turn crosses a section's first page. Paging inside a
  section moves no ribbon.
* A drag let go before the spine: the page falls back, its ribbons with it.

## Riding the page

* The ribbon travels with the turning page and bends with the curl; no swap when the turn ends. That is the point of
  the animation.
* The ribbon hangs out past the page edge, so the page's render target gets wider to hold it, and the curl mesh
  covers that margin.
* Two ribbons on one turning page (Field notes to Monsters) ride at the same depth, then slide to their own depths as
  the page lands.

## Depth

* Each ribbon comes out of the page block at its section's depth: the more pages lie between the open spread and the
  ribbon's page, the further out sideways (x) it hangs, following the stacked page edges (`EDGE_STEP`).
* Both sides. On the left the Monsters ribbon is the deepest, so it sticks out furthest. Depth follows the page edges
  as they move from right to left through the book.
* Heights (y) stay as they are; each ribbon keeps its height on either side, so it is the same ribbon.
* The open section's ribbon no longer sticks out further (`RIBBON_OPEN_LEN` goes); depth sets how far a ribbon hangs
  out. The open section shows by its letter (M / R / F), as now.

## Look

* Left ribbons: mirrored texture, the swallowtail pointing left. They hang over the strap.
* Hover label of a left ribbon: on the left page, next to the ribbon.

## Notes

* Code: `src/ui/journal_view.cpp` (`ribbonRect`, `ribbon`, `DrawRibbons`, the page edges in the book drawing, `GoTo`,
  `StartTurn`), `src/ui/page_curl.h`. Look: [docs/ui.md](../ui.md), "Book (journal)".
* Scenario to update: `tests/scenarios/journal_page_turn.txt` (ribbons on both sides, mid-turn shot with a ribbon on
  the curl).
