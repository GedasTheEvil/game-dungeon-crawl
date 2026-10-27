# Text vertical centre

Status: draft 2026-10-27, from the user. Ready (needs nothing first).

## Problem

Button labels sit too high: in Options > Display the "On", "Off", "Monitor" and "1920 x 1168" labels poke out of the
top of their tiles ([screenshot](../screenshots/options-out-of-bounds.png)).

## Cause

`ui::text(font, x, y, ...)` puts the **bottom of the glyph quad** at `y`, not the baseline as
[ui.md](../../ui.md#fonts) says. The quad is `size` tall, and the sheet's glyphs sit in a 51 px cell with the baseline
34 px from the top and a cap height of 24 px (`tools/textures/font_sheet.py`). So, from `y`:

* baseline at `17/51 * size` (0.333 size),
* cap top at `41/51 * size` (0.804 size).

Callers place text with hand-picked offsets (`control.y + 0.2f`, `r.y + 1.4f`, `r.y + 0.6f`, ...), so every
call does its own guess at this maths. Example, the Options value labels (`MainMenu::DrawRows`, `src/ui/menu.cpp`):
the control is 3.6 tall, `heading` is size 5, text at `control.y + 0.2`. The cap top lands at 0.2 + 4.02 = 4.22, which is
0.6 above the tile. Centred caps need `y = control.y - 1.04`, so the labels are about 1.25 units too high.

## Fix

1. `Font`: expose the metrics from the sheet layout: `Baseline()` (0.333 size) and `CapHeight()` (0.47 size), with
   the 51 / 34 / 24 px constants in one place (font.cpp), matching `font_sheet.py`. All four sheets (papyrus,
   kalam, courier, impact) share this layout; check that impact does too.
2. `ui_draw`: a helper that centres the cap height on a y, e.g. `textCenteredIn(font, const Rect& r, str, c)` (centre
   in a rect, both axes) plus a `textMiddle(font, x, cy, ...)` for left-aligned labels. Lowercase without ascenders
   ("on") looks a bit low with cap centring; the cap height is still the right reference (as in "On", "Off").
3. Move the button and tile labels to the helper: menu buttons, Options tabs, switch / choice values, slider value,
   control cells, Heka / inventory / journal / riddle buttons. Grep `text(` / `textCentered(` in `src/ui/`
   (164 calls). Only labels inside a box need the helper; free text (body lines, paragraphs, titles on a rule) keeps its
   layout, but check none of it moves.
4. Fix [ui.md](../../ui.md#fonts): say what `y` really is (quad bottom, baseline 1/3 size above) and point to the
   helper for labels inside a box.

## Tests

* Unit test: the metrics against the sheet. Read `papyrus.png`, find the inked bottom of "H" and its top, and
  compare them with `Baseline()` / `CapHeight()` within 1 px.
* Scenario with screenshots: Options (Display, Audio, Controls tabs), main menu, inventory, Heka, riddle, journal
  buttons. Check every label sits inside its tile, centred by eye.
