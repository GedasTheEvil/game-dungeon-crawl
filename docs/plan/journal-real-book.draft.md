# Journal as a real book

Status: draft 2026-10-01, refined 2026-10-01 from the reference image and user feedback.

## Idea

The journal ([journal-sections.md](solved/journal-sections.md)) reads as a flat board: two papyrus panels on a cover. It
should feel like a real, used field notebook, the kind an archaeologist carries on a dig. Reference:
[notebookstories.com example](https://www.notebookstories.com/wp-content/uploads/2020/03/001.jpg), an excavation
notebook from 1997.

## What the reference shows

* **Book:** grey cloth hardcover, a little larger than the pages, so a cover edge shows all round. A cream elastic
  strap loops round the left cover edge. On the right the page block is visible as a stack of thin edges. The pages
  dip into a dark gutter at the spine, with a red headband peeking out top and bottom.
* **Paper:** white, a fine light blue-green grid (about 5 mm squares) over the whole page, no margin rule. Clean, not
  aged: the realism comes from the grid, the slight curve and the stacked edges, not from stains.
* **Page numbers:** stamped / typewriter numerals, top outer corner (`8160`, `8161`), with a small pencilled
  cross-reference above (`363, 368`).
* **Pasted photos:** black-and-white photos with a white border, slightly askew, taking most of a page. Each has a
  red catalogue number above it (`97-34-25`) and a handwritten caption with an arrow below ("↑ FROM SOUTH-WEST").
* **Handwriting:** block capitals in blue / black ink. A centred, underlined heading ("SE EXTENSION ..."). Dated
  entries ("JULY 14, 1997. BEGIN TO DISMANTLE ...").
* **Margin column:** short labels on the left of the right page ("PHOTO 8160", "CLOSED", `S2287` in red) next to
  their entry, like an index.
* **Form fields:** "POTTERY = ..., COINS: ..., FINDS: ..." filled in by hand, some blank.
* **Two inks:** red for numbers and catalogue IDs, blue for the text, a pencil note squeezed in sideways.

## Look for the game

Proposals, to confirm when the work starts:

* Book: cloth cover (dark sand or grey, not the current bronze) with the cover edge showing, an elastic strap on the
  left edge, stacked page edges on the right, gutter shadow and page curve at the spine.
* Decided 2026-10-01: the ribbon bookmarks ([journal-sections.md](solved/journal-sections.md)) stay on the right edge; the
  strap goes on the left. Each ribbon gets a letter: M (monsters / creatures), R (riddles), F (field notes). The letter shows only on the open section's ribbon and on the hovered one.
* Paper: white-cream with the grid, as a texture (grain + grid + slight shading towards the spine) from a
  `tools/textures/` script, like the ribbon.
* Creatures: the monster sketch becomes a pasted b/w "photo" with a white border and a small tilt, a red catalogue
  number above (level and monster, e.g. `L07-03`), a caption with an arrow below. Before the kill a pencil sketch on
  the grid instead (as now), so the photo is the reward for the kill.
* Notes as the reference does them: underlined heading per monster, dated entries (day / level instead of a date),
  a margin column with short labels (`SEEN`, `HIT`, `KILLED`, resistances), and a form block filled in as learnt
  ("WEAPON = ..., WEAK TO: ..., RESISTS: ...", blank fields left empty, like "FINDS:" in the reference).
* Inks: blue for the text, red for numbers / IDs / page numbers, pencil for guesses and minor notes. Decided
  2026-10-01: the font stays Kalam ([solved/handwritten-journal-font.md](solved/handwritten-journal-font.md)), no
  block capitals.
* Decided 2026-10-01: riddles and field notes are text only, on the same grid paper; no photo or rubbing.
* Optional wear, light: a dog-ear or small smudge, not the stains-everywhere look; the reference is clean.

## Page flip

A 3D page turn instead of an instant swap: the page lifts at the corner, curls over the spine and lands on the
other side, showing its back on the way.

Decided 2026-10-01: a real 3D page, no 2D fake. A mesh page (grid strip) bent around a cylinder that moves across
the spread (classic page curl), rendered with the page's texture front and back. Pages drawn into render targets
first.

The stacked page edges could shift from right to left as the player goes through the book.

Decided 2026-10-01:

* Sound: a paper rustle per flip.
* Mouse drag: grab a page corner and drag it over, the curl following the mouse; let go past the spine to finish
  the flip, before it to let the page fall back. Do it if it comes easily with the curl mesh; if not, a click on
  the page corner starts the flip animation (arrow keys / wheel as now).

Open: flip speed, flip several pages at once on a ribbon click.

## Notes

* Shared UI look and canvas: [docs/ui.md](../ui.md), "Book (journal)". Code: `src/ui/journal_view.cpp`.
* Decided 2026-10-01: the journal comes from outside the tomb, so leaving the Egyptian look on this screen is fine.
