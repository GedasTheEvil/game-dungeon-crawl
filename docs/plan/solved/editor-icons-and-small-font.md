# Editor: missing tile icons, readable small text

Status: solved 2026-11-04. Was: draft 2026-11-04, needs nothing first. From the user's look at the editor's wall palette
([screenshot](../screenshots/editor-missing-icons.png)).

Opus: icon design for several tiles and a new font sheet (font choice, rendering at small sizes). Tooling only (the
level editor): once implemented and `make check` passes, it can go straight to "solved/" (docs/plans.md, Solved).

## Problems

1. Empty icon wells. Tiles with no `icon` in `src/world/tile_defs.cpp` show a black square in the palette (and no
   symbol on the grid):
   * wall layer: `WallSpikes`, `SunDisc`, `FireVent`, `PoisonDrip`, `SandGust`, `CeilingSpikes`, `CeilingDeath`;
   * object layer: `LightShaft`, `Mirror`, `Page`.
   (`None`, `Unknown` and the structures need none: structures draw a swatch.)
2. The small text is hard to read. The palette tile names (`tiny`, 1.9 canvas units, `tools/editor/editor.cpp`
   `drawPalette`) and the `small` texts (readout, hints) use `media/textures/fonts/parchment.png`: a decorative
   display face, drawn from 51 px glyph cells shrunk to a few screen pixels with `TexFilter::Flat` (no mipmaps), so
   thin strokes break up. Long names (`CeilingSpikes`, `CeilingDeath`) also overflow their tile.

## Plan

### Icons

* Add one function per tile to `tools/editor/icons/make_icons.py`, same style as the rest (64 x 64, 4x supersampled,
  UI palette, transparent background, soft glow). Ideas:
  * `WallSpikes`: a wall slab with three spikes pointing out sideways, green poison tips.
  * `SunDisc`: a gold disc with rays on a wall slab, a lapis gem (it acts as a lever, the gem hints at the colour).
  * `FireVent`: a low vent slot in the wall, a red/orange flame jet rising.
  * `PoisonDrip`: a ceiling crack with green drops falling.
  * `SandGust`: a wall slot blowing curved sand streaks sideways.
  * `CeilingSpikes`: a ceiling slab with spikes pointing down (mirror of `spikes()`).
  * `CeilingDeath`: the same, longer and red-tipped (like `death.png` vs `spikes.png`).
  * `LightShaft`: a crack in rock with a pale yellow beam.
  * `Mirror`: a bronze disc on a stand.
  * `Page`: a torn papyrus sheet (or one on a skull).
  Read each tile's description in `tile_defs.cpp` first; the icon shows what the game shows.
* Set the `icon` field of each in `tile_defs.cpp`; run the script, commit the PNGs.
* Check the result: a scenario or an `xvfb-run` screenshot of the editor's wall and object palettes (and a few on the
  grid), every well filled, each icon tells apart from its neighbours at palette size.

### Small-size font

* Make a sans-serif sheet for small UI text with `tools/textures/font_sheet.py`, from a font built for small sizes
  and free to ship: DejaVu Sans (Bitstream Vera licence, installed under `/usr/share/fonts`) is the first pick;
  alternatives Noto Sans, Inter, Liberation Sans. Ship its licence next to the sheet (`media/textures/fonts/`, as
  `kalam-OFL.txt`).
* Make it sharp when shrunk. Options, implementer's choice (try them, compare screenshots):
  * load the sheet with mipmaps (`TexFilter::Mipmapped` in `Font`; maybe a flag on `Font::Load`);
  * a heavier weight / small stroke (`--weight`, `--stroke`) so thin strokes survive;
  * a larger sheet (1024 or 2048) if 512 is not enough.
* Use it in the editor for `tiny` and `small` (palette names, readout, hints, field labels); keep `parchment.png` for
  the title and headings so the editor keeps the game's look ([docs/ui.md](../../ui.md)).
* Names fit their tile: with the new font, check the longest (`CeilingSpikes`, `TrappedTreasure`, the props); if one
  still overflows, shrink that label to fit (measure with `TextWidth`) or break it at the capital letter on two lines.
* Optional, separate decision: the game's own small texts (`ui_draw.cpp` `small`, HUD) could use the same sheet.
  Not part of this plan unless the user asks; note it in the report.

## Docs

* `tools/editor/readme.md`: the new icons (the paragraph on `make_icons.py`) and the small font.
* `docs/ui.md` if `Font` gains a mipmap option.
* Glossary only if a new term comes up.

## Done (2026-11-04)

* Icons: the ten tiles above, in `make_icons.py` (`later_tiles()`), each with its `icon` in `tile_defs.cpp`. Checked in
  the wall and object palettes and painted on the grid at 3x zoom.
* Font: DejaVu Sans Condensed Bold, 1024 sheet, no extra stroke (`media/textures/fonts/dejavu-condensed-bold.png`,
  licence `dejavu-LICENSE.txt`), loaded mipmapped (`Font::Load` gained a `TexFilter` argument). Compared in screenshots
  with DejaVu Sans, Sans Condensed and Sans Bold: all far sharper than parchment; Condensed Bold reads best and its
  names fit. The editor's `small` and `tiny` use it; title, heading and body stay parchment.
* Names: a palette name wider than its tile (`CeilingSpikes`, `CeilingDeath`) is drawn in `tinier` (1.6).
* Added at the user's request: the papyrus text scrolls (mouse wheel over it) when it is longer than the papyrus (a
  monster's hint lists every monster kind); clipped under the rule, a red scroll bar on its right, a new hint starts
  at its top.
* Not done (separate decision): the game's own small texts (`ui_draw.cpp` `small`, HUD) could use the same sheet.
