# Port the UI screens and fonts to the GL core path

Status: solved 2026-11-19 (play-tested by the user). Was: implemented 2026-11-18, waiting for the user's play-test. Was: draft 2026-11-18, size L, needs [gl-core-foundation](gl-core-foundation.feature.md) first.

## Problem

After [gl-core-foundation](gl-core-foundation.feature.md) the UI still draws with immediate mode,
`glOrtho`, display-list fonts and, in the journal, fixed-function lights.

## Plan

* Files: `src/render/ui_draw.cpp`, `font.cpp`, `brightness.cpp`; `src/ui/*` (menu, options, inventory, riddle, map,
  journal pages and view, page curl, skills, boss bar, HUD, zone banner, interact prompt, level gem, status box,
  death and ending screens, screen tabs, menu icons, win entry); `src/test/scenario.cpp` (screenshot read-back).
* Projection: an ortho matrix in `MatrixStack` replaces `glOrtho`.
* Font: display lists (`glCallLists`) → glyph quads into a `Batch` from the existing font atlas texture; same
  metrics, same look.
* `ui_draw.cpp` parts (panels, frames, gradients) on `Batch`; all screens share them ([ui.md](../../ui.md)), so most
  screens port by porting `ui_draw.cpp`.
* Journal page: its 3D lit page with the default lit shader (same light direction and colours as now).
* Page curl and map: `Batch`; the map's many small quads in one batch per frame.
* Each screen checked with `make snapshots` ([render-snapshots](render-snapshots.tooling.md)).
* Update [ui.md](../../ui.md) where it names GL calls.

## Play-test

* `bin/game --level 1`: open the menu, options, inventory, map, journal (turn pages), skills and Heka: same look as
  before, text sharp and aligned.
* Read a riddle and an inscription; trigger the death screen.

## Done

* `ui_draw.cpp`, `font.cpp` and every `src/ui/` file draw only through `Batch` under `gMv` / `gProj`: their legacy counts are 0
  (`tools/gl_legacy.py report`). Canvases: `ui::beginCanvas`, `ui::beginSquareCanvas`, `ui::beginOrtho` (also
  copied into GL for the legacy world); a screen ends with `ui::endScreen`. Legacy callers adopt the GL matrices first
  (`draw.cpp`'s tooltip, `monster_draw.cpp`'s status cards).
* Font: no display lists; the glyph quads of a text in one batch, each corner transformed on the CPU as the old
  per-glyph `glTranslatef` did, split as Mesa split a display list's quads (a, b, d / b, c, d). `Font::print` stays
  for the loading screen (legacy projection and colour).
* Map: marks in one batch, pencil strokes one batch per width; clip planes became a scissor box (nested in a
  scenario's blind scissor). Page curl: `Batch`, alpha test by `DefaultShader::Kind::AlphaTest`, `GlState` and the
  depth func, cull face, front face, line smooth saved by hand instead of `glPushAttrib`.
* Journal photo: the light is a `DefaultShader::Light`; the creature's model (`AnimatedModel::Show`, client arrays,
  gl-core-world's) still draws legacy, so `GlLegacy::setLight` / `lighting` set `GL_LIGHT0` from it. The lit shader
  takes over once the models have a VAO (gl-core-world / gl-core-profile).
* `src/test/scenario.cpp` needed nothing: its read-back (`glReadPixels`, `glReadBuffer`) is core.
* Snapshots: 0 changed for every world, zone, trap and monster shot; ±1 a channel at a few text pixels in 9 UI shots
  (at most 37 pixels an image, the journal's FBO pages most) and one pixel where the curled page's front and back meet
  at the fold (`ui-journal_turn`): precision of the shader path, references taken anew.

