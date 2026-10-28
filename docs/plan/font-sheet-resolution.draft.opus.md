# Sharper text: a bigger papyrus font sheet

Status: draft 2026-10-28, needs nothing first. From the user: the fonts come from a small PNG; big text, like the
menu's, already looks washed out. Regenerate the font texture at a higher resolution, Papyrus first, with the tool we
have.

## Today

* Every font is a 512 x 512 grey sheet in `media/textures/fonts/`: 10 x 10 cells of 51.2 px, printable ASCII
  (`Font::Load`, `src/graphics/font.cpp`). Papyrus is all the UI and HUD text (`src/ui/ui_draw.cpp`,
  `src/state/assets.cpp`), kalam the journal's hand, courier its numbers, impact the HUD numbers. `papyrus_i.png`
  is loaded nowhere.
* The menu title (size 8) and banners (7) stretch a 51 px cell over far more screen pixels: blurred edges
  (`TexFilter::Flat`, plain linear, no mipmaps so the cells do not bleed).
* The tool: `tools/textures/font_sheet.py FONT.ttf out.png [--weight W]` draws each glyph at 4x and downsamples it,
  with the papyrus sheet's baseline and cap height. The sheet size is fixed (`SHEET = 512`, `CELL = 51`).
* `Font::Load` reads the cell size from the image (`width / GRID`) and the glyph edges as fractions; the metrics in
  `font.h` (`QUAD_PX`, `BASELINE_PX`, `CAP_HEIGHT_PX`) are in px of the 512 sheet but only used as ratios. A bigger
  sheet with the same layout needs no code change.
* **No Papyrus TTF on this machine** (`fc-list`), and papyrus.png has no license file next to it, unlike kalam and
  courier. Papyrus is a commercial font (Linotype; ships with Windows Office / macOS). impact.png is the same case.

## Idea

* `font_sheet.py --size N` (default 512): the sheet side, cells at `round(i * N / 10)` so they match the loader's
  0.1 texture steps (today's 51 px cells drift 1.8 px by the last column).
* Regenerate `papyrus.png` at 2048 (204.8 px cells, 4x) from a Papyrus TTF, then compare it with the old sheet at the
  same size in a screenshot (`tests/scenarios/`, the menu and a HUD banner): same baseline, cap height and widths, so
  no layout shifts. Do the same for kalam and courier (their TTFs are free; kalam's OFL, courier's Bitstream); impact
  only if its TTF is at hand.
* Small text (`small` 3, `body` 3.6, the HUD's 3 / 4.2) minifies a 2048 sheet a lot: with plain linear it can shimmer
  or look thinner. Check it in the screenshots. If it does: mipmaps with the level count capped so a cell stays at
  least a few px (`GL_TEXTURE_MAX_LEVEL`, cells padded), or 1024 instead of 2048.

## Open (for the implementer)

* **The Papyrus TTF: ask the user** for the file (and whether its license allows a sheet in the game; the current
  sheet already ships one). Without it: upscale the existing 512 sheet (no TTF) or a free look-alike font, which
  changes the look: the user's call, show a screenshot first.
* Delete `papyrus_i.png` if it stays unused.
* Update `docs/ui.md` (Fonts) with the size and the new command line.
