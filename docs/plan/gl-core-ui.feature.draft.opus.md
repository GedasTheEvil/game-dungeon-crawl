# Port the UI screens and fonts to the GL core path

Status: draft 2026-11-18, size L, needs [gl-core-foundation](gl-core-foundation.feature.draft.opus.md) first.

## Problem

After [gl-core-foundation](gl-core-foundation.feature.draft.opus.md) the UI still draws with immediate mode,
`glOrtho`, display-list fonts and, in the journal, fixed-function lights.

## Plan

* Files: `src/render/ui_draw.cpp`, `font.cpp`, `brightness.cpp`; `src/ui/*` (menu, options, inventory, riddle, map,
  journal pages and view, page curl, skills, boss bar, HUD, zone banner, interact prompt, level gem, status box,
  death and ending screens, screen tabs, menu icons, win entry); `src/test/scenario.cpp` (screenshot read-back).
* Projection: an ortho matrix in `MatrixStack` replaces `glOrtho`.
* Font: display lists (`glCallLists`) → glyph quads into a `Batch` from the existing font atlas texture; same
  metrics, same look.
* `ui_draw.cpp` parts (panels, frames, gradients) on `Batch`; all screens share them ([ui.md](../ui.md)), so most
  screens port by porting `ui_draw.cpp`.
* Journal page: its 3D lit page with the default lit shader (same light direction and colours as now).
* Page curl and map: `Batch`; the map's many small quads in one batch per frame.
* Each screen checked with `make snapshots` ([render-snapshots](solved/render-snapshots.tooling.md)).
* Update [ui.md](../ui.md) where it names GL calls.

## Play-test

* `bin/game --level 1`: open the menu, options, inventory, map, journal (turn pages), skills and Heka: same look as
  before, text sharp and aligned.
* Read a riddle and an inscription; trigger the death screen.
