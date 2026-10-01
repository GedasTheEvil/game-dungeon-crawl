# Handwritten journal font

Status: done 2026-10-01, verified in play 2026-10-01. Kalam (SIL OFL) in pencil grey (`ui::PENCIL`, the draft map's
pencil) on the journal's pages. The creatures section's monster sketches use the same colour
([journal-sections.md](journal-sections.md)).

## Idea

All text uses `fonts/papyrus.png` ([docs/ui.md](../../ui.md#fonts)), the game's ancient Egypt look. The journal is the
archaeologist's own notebook, written by hand in pencil or ink, not in an Egyptian style. Its notes get a
handwriting font of their own. The rest of the journal screen (title, ribbon names, page numbers) can stay papyrus.

## What

* Pick a free handwriting font (OFL or public domain, 1920s pencil / fountain pen feel, readable at the `body` and
  `small` sizes on the 160 x 100 canvas). Note its licence in the credits.
* Make a font sheet like `papyrus.png` (same grid and glyph set, printable ASCII). Check how the existing sheets
  were made first; if there is no tool, write one under `tools/`.
* Load it in the journal's fonts next to `ui::loadScreenFonts()`; colour like pencil (graphite grey) or ink (dark
  brown) on the page.
* Document it in [docs/ui.md](../../ui.md#fonts): which text uses which font.
* Scenario screenshot of a journal page to check readability.

## Done

* Candidates compared at the in-game size on papyrus (2026-10-01): Kalam, Patrick Hand, Caveat, Architects
  Daughter, Gochi Hand, Shadows Into Light Two, Covered By Your Grace, Nothing You Could Do. Kalam reads best at the
  `small` size and looks most like pencil; Patrick Hand was second, Caveat too small at the same cap height.
* `tools/textures/font_sheet.py` makes the sheet (`fonts/kalam.png`), licence in `fonts/kalam-OFL.txt`.
* Pencil, not ink: decided by the agent, matches the draft map. One constant to change (`ui::PENCIL`).
* Handwritten: the riddle pages' theme, level, question, hint and answer, "Nothing written yet". Papyrus: the running
  head, page numbers, the Answer button.

## Open

* Also for other "written by the archaeologist" text later (notes on the map)?
* The credits sheet is an image; Kalam's credit is the licence file only so far.
