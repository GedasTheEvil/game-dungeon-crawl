# Journal as a real book

Status: draft 2026-10-01.

## Idea

The journal ([journal-sections.md](journal-sections.md)) reads as a flat board: two papyrus panels on a cover. It
should feel like a real, used field notebook, the kind an archaeologist carries on a dig. Reference:
[notebookstories.com example](https://www.notebookstories.com/wp-content/uploads/2020/03/001.jpg).

## Look

Open, to decide when the work starts:

* Paper instead of papyrus: off-white / cream, a paper grain texture, faint ruled or grid lines, a margin line.
* Wear: foxing spots, coffee rings, sand smudges, darker yellowed edges, dog-eared corners.
* Depth: pages curve down into the spine (shading gradient, slight bend), page edges stacked on the outer side
  (thicker on the left or right with progress through the book), cover leather visible around the pages.
* Pasted-in things: sketches with pencil hatching, a "photo" or a pressed note held by tape or a paper clip, arrows
  and annotations in the margin.
* Elastic band or ribbon ties on the cover, in keeping with the [ribbon bookmarks](journal-sections.md).

## Page flip

A 3D page turn instead of an instant swap: the page lifts at the corner, curls over the spine and lands on the
other side, showing its back on the way. Options:

* A mesh page (grid strip) bent around a cylinder that moves across the spread (classic page-curl), rendered with
  the page's texture front and back. Pages drawn into render targets first.
* Cheaper fallback: 2D fake (page scaled in x with a shading gradient and a shadow on the page below).

Open: flip speed, sound (paper rustle), flip several pages at once on a ribbon click, mouse drag to flip.

## Notes

* Shared UI look and canvas: [docs/ui.md](../ui.md), "Book (journal)". Code: `src/ui/journal_view.cpp`.
* The handwriting font stays ([solved/handwritten-journal-font.md](solved/handwritten-journal-font.md)).
