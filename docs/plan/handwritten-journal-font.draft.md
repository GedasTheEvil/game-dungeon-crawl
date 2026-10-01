# Handwritten journal font

Status: draft 2026-10-01. Next after [journal-sections.draft.md](journal-sections.draft.md) stage 1; the creatures
section waits for it: the monster sketches are tinted in the handwriting's colour.

## Idea

All text uses `fonts/papyrus.png` ([docs/ui.md](../ui.md#fonts)), the game's ancient Egypt look. The journal is the
archaeologist's own notebook, written by hand in pencil or ink, not in an Egyptian style. Its notes get a
handwriting font of their own. The rest of the journal screen (title, ribbon names, page numbers) can stay papyrus.

## What

* Pick a free handwriting font (OFL or public domain, 1920s pencil / fountain pen feel, readable at the `body` and
  `small` sizes on the 160 x 100 canvas). Note its licence in the credits.
* Make a font sheet like `papyrus.png` (same grid and glyph set, printable ASCII). Check how the existing sheets
  were made first; if there is no tool, write one under `tools/`.
* Load it in the journal's fonts next to `ui::loadScreenFonts()`; colour like pencil (graphite grey) or ink (dark
  brown) on the page.
* Document it in [docs/ui.md](../ui.md#fonts): which text uses which font.
* Scenario screenshot of a journal page to check readability.

## Open

* Pencil or ink.
* Also for other "written by the archaeologist" text later (notes on the map)?
