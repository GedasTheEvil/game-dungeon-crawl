# Sharper text: a bigger font sheet, Parchment MF for Papyrus

Status: draft 2026-10-28, refined 2026-10-28 (Parchment MF replaces Papyrus), needs nothing first. From the user: the
fonts come from a small PNG; big text, like the menu's, already looks washed out. Regenerate the font texture at a
higher resolution with the tool we have. There is no Papyrus TTF: use Parchment MF instead
(`/home/gediminas.skucas/Font/Parchment-MF/Parchment MF.ttf`).

## Today

* Every font is a 512 x 512 grey sheet in `media/textures/fonts/`: 10 x 10 cells of 51.2 px, printable ASCII
  (`Font::Load`, `src/graphics/font.cpp`). Papyrus is all the UI and HUD text (`src/ui/ui_draw.cpp`,
  `src/state/assets.cpp`), kalam the journal's hand, courier its numbers, impact the level gem's number
  (`level_gem.cpp`). `papyrus_i.png` is loaded nowhere.
* The menu title (size 8) and banners (7) stretch a 51 px cell over far more screen pixels: blurred edges
  (`TexFilter::Flat`, plain linear, no mipmaps so the cells do not bleed).
* The tool: `tools/textures/font_sheet.py FONT.ttf out.png [--weight W]` draws each glyph at 4x and downsamples it,
  with the papyrus sheet's baseline and cap height. The sheet size is fixed (`SHEET = 512`, `CELL = 51`).
* `Font::Load` reads the cell size from the image (`width / GRID`) and the glyph edges as fractions; the metrics in
  `font.h` (`QUAD_PX`, `BASELINE_PX`, `CAP_HEIGHT_PX`) are in px of the 512 sheet but only used as ratios. A bigger
  sheet with the same layout needs no code change.
* Parchment MF (Rick W. Mueller, 1995, from ffonts.net): all 95 printable ASCII glyphs, a Papyrus-like hand, a bit
  narrower and with thinner strokes. The TTF stays outside the repo, as kalam's and courier's do.

## Idea

* `font_sheet.py --size N` (default 512): the sheet side, cells at `round(i * N / 10)` so they match the loader's
  0.1 texture steps (today's 51 px cells drift 1.8 px by the last column).
* A new `media/textures/fonts/parchment.png` at 2048 (204.8 px cells, 4x) from Parchment MF, same baseline and cap
  height. Every `papyrus.png` load points to it (`ui_draw.cpp`, `assets.cpp`); `papyrus.png` and `papyrus_i.png` go
  once the user approves the look.
* Its strokes are thinner than Papyrus: check small text (`small` 3, `body` 3.6, the HUD's 3 / 4.2) on dark panels
  and on papyrus scrolls. Too thin: a stroke option in the tool (PIL `stroke_width` at the 4x scale) to thicken it.
* Narrower glyphs: lines get shorter, nothing should overflow. Check the screens with long text (riddle, inventory
  details, credits, journal buttons) for layouts that relied on Papyrus widths.
* Regenerate kalam and courier at 2048 too (their TTFs are free: kalam OFL, courier Bitstream; fetch them if they
  are not on the machine). impact stays at 512: the level gem looks fine (the user, 2026-10-28).
* Small text minifies a 2048 sheet a lot: with plain linear it can shimmer. If it does: mipmaps with the level count
  capped so a cell stays at least a few px (`GL_TEXTURE_MAX_LEVEL`, cells padded), or 1024 instead of 2048.
* Screenshots (`tests/scenarios/`): the menu, a HUD banner, a riddle, the inventory, the journal. Old sheet vs new.

## Open (for the implementer)

* **License:** the TTF says "All rights reserved worldwide", with no license file; ffonts.net lists it as free.
  Tell the user before committing the sheet; add a `parchment-LICENSE.txt` with what is known (author, source).
* Update `docs/ui.md` (Fonts: the file, the size, the new command line) and the `font.h` comment ("Papyrus, kalam
  and courier share it").
