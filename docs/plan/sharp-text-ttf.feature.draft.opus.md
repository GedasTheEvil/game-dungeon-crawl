# Sharp text: glyphs rasterised from the TrueType font at the screen pixel size

Status: draft 2026-11-23, size L, needs nothing first.

## Idea

The user, 2026-11-23: with Options > Display > Text font on DejaVu ([ui-font-setting](ui-font-setting.feature.md)),
the text looks nice but blurry. Investigate TrueType fonts instead of the font sheet textures; other ways to make the
text sharp are welcome too.

## Why it is blurry (investigated 2026-11-23)

* `media/textures/fonts/dejavu-condensed-bold.png` is 1024 px: a 102 px cell. The `body` font (3.6 canvas units) is a
  39 px quad at 1080p, `small` (3) a 32 px quad: the sheet is drawn 2.6 to 3.2 times smaller.
* It loads `TexFilter::Mipmapped` (`GL_LINEAR_MIPMAP_LINEAR`, `src/render/textures.cpp`): at that ratio trilinear
  filtering blends the 51 px and 26 px mip levels, so a 2 px stem smears over 3 to 4 px. That is most of the blur.
* The glyph quads sit at fractional pixels (`Font::draw`, pen + proportional advance in canvas units, no snapping), so
  even a perfect texel-to-pixel glyph is bilinear-resampled once more.
* The sheet itself is antialiased by a 4x Lanczos downsample (`tools/textures/font_sheet.py`), with no hinting:
  stems don't land on whole pixels at any size.
* Parchment (2048 sheet, `Flat` = `GL_LINEAR`, no mips) at 39 px minifies about 5x with plain bilinear: it aliases
  rather than blurs; its brush strokes hide that.

## Options weighed

| Option | Sharpness | Cost |
|---|---|---|
| A. Bake glyphs from the TTF at runtime, at the real pixel size, pen snapped to whole pixels | best (1 texel = 1 pixel) | L: `Font` keeps its API, new atlas code, re-bake on resize |
| B. Quick fixes on the sheet: snap to pixels, `GL_TEXTURE_LOD_BIAS` about -0.5..-1, or a sheet baked near 1080p size (512) | better, still resampled; wrong at other window heights | S |
| C. Signed distance field sheet (one sheet, a `smoothstep` in the text shader) | sharp edges at any size and zoom, soft small text (no hinting) | M: sheet tool, shader kind, the two-pass draw becomes one alpha pass |
| D. Several baked sheets per size, pick the nearest | good at the baked sizes only | M, more PNGs |

**Pick A**, with B's pixel snapping as its first step (it is part of A anyway). C is the fallback if A proves too
heavy; D is a weaker A.

Library for A, the implementer's choice:

* `stb_truetype.h` (recommended): one public-domain header next to `external/stb/stb_image.h`, no new package, no
  hinting (its output at 15 to 20 px is clean with pixel snapping and a bold font).
* SDL3_ttf (FreeType + HarfBuzz, hinting, best small text): the game already links SDL3; needs
  `sudo apt install libsdl3-ttf-dev`, a pkg-config line in the `makefile`, and the install helper
  ([install-helper-script](install-helper-script.feature.md)). Ask the user to install it; never work around it.

## Plan

* **Step 1, cheap, measure first.** In `Font::draw`, snap the pen's first quad corner to a whole pixel (from `gProj`
  and the viewport: canvas → pixels → round → canvas) and let the advances keep their fractions or round them too.
  Try `GL_TEXTURE_LOD_BIAS` -0.5 on the mipmapped sheet. Scenario screenshots of the inventory and Heka body text in
  both fonts, before and after, cropped at 1:1 (`tests/out/`). Show the user; if they find it sharp enough, stop here,
  write it down, and size the plan down (S).
* **Step 2, TrueType atlas.** `Font::Load` gets a TTF path variant (or a `FontFace` source next to `FontSheet`): the
  size in canvas units times the current pixels per canvas unit (window height / `ui::CANVAS_H` for the 160 x 100
  canvas; the HUD canvases' own scale) gives the pixel size; bake printable ASCII (95 glyphs, maybe Latin-1 later) into
  a single-channel atlas (`GL_R8`, swizzled or sampled as alpha), `GL_LINEAR` or `GL_NEAREST`, no mips. Advances and
  kerning from the font, converted back to canvas units, so `TextWidth`, `wrap`, `textCenteredIn` and every layout
  stay in canvas units and unchanged.
* Atlases shared per (file, pixel size), like `loadSheet`'s cache. Baseline and cap height from the font's metrics,
  so `Font::Baseline()` / `CapHeight()` still centre labels (`textMiddle`); match parchment's 18 / 24 of 51.2 layout
  where the boxes rely on it.
* **Resize:** a window size change re-bakes: bump a generation like `ui::bodyFontGeneration` (or fold the pixel
  scale into it) so `ensureScreenFonts` and `FontSet::RefreshBodyFonts` reload. Same for the scenario window size.
* **Draw:** pen snapped to whole pixels, quads exactly texel-sized. With an alpha atlas `ui::text` can draw once
  (`GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA`) instead of its cut-out + add two passes; keep both paths while sheet fonts
  remain.
* **Scope:** DejaVu body / small first (the TTF from `/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf`,
  copied to `media/fonts/`, its licence `dejavu-LICENSE.txt` is already in the repo). Then, the implementer's choice,
  the editor's small text (`tools/editor/editor.cpp`, same sheet). Parchment, Kalam, Courier, Impact stay sheets in
  this plan: parchment's TTF has no licence file (`parchment-LICENSE.txt`, "all rights reserved"), ask the user before
  shipping the TTF itself. Kalam (OFL) and Courier could follow in a later plan.
* `tools/textures/font_sheet.py` and the sheets stay for the fonts not moved. Docs: [ui.md](../ui.md#fonts) Fonts
  section (TTF vs sheet, the re-bake), glossary row for the atlas term, `tools/where.py` row.
* Tests: a unit test that a baked font's `TextWidth` in canvas units stays within a few percent at 720p and 1440p
  (layout must not depend on resolution beyond rounding); a scenario screenshotting body text at two window sizes.
  Render snapshot tests that cover DejaVu text change: update them on purpose.

## Open questions

* stb_truetype vs SDL3_ttf (hinting, a new package). Recommendation: stb_truetype, unless step 2's 15 px `small` text
  still looks soft; then SDL3_ttf.
* Snapped advances shift line lengths by up to 1 px a glyph: check the widest panels (inventory details, Heka cards,
  credits) still fit at 720p.
* Does the user want parchment as a TTF too (licence)?

## Play-test

* `bin/game --play tests/play/sharp-text-ttf.txt`; Esc > Options > Display > Text font: DejaVu.
* Inventory (I), Heka (K), How to Play: body and small text crisp, no soft edges, nothing overflows.
* Resize the window (or change resolution): text stays crisp, lines wrap as before.
