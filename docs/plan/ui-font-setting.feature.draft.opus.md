# UI font setting: parchment or DejaVu condensed bold

Status: draft 2026-11-20, size M, needs nothing first.

## Idea

The user, 2026-11-20: a new option to pick the UI font: `parchment` (default) or `dejavu-condensed-bold`
(`media/textures/fonts/dejavu-condensed-bold.png`, already used by the level editor). It does not apply to headlines,
only the smaller text.

## Plan

* New `Settings` field (`level/settings_ini.*`, `state/settings.*`, [settings.md](../settings.md)), a two-value enum, default
  parchment; an Options row in `ui/menu_options.cpp` (`tools/where.py options settings`). Old settings files: default.
* "Headlines" = the `title` and `heading` fonts of `ui::loadScreenFonts()` (`src/render/ui_draw.cpp`, [ui.md](../ui.md#fonts));
  they stay parchment. The `body` and `small` fonts (and HUD / toast body text) switch to the chosen sheet. Implementer
  checks the button labels (`heading`) stay parchment and lists which texts changed.
* DejaVu is drawn much smaller than its sheet: load it with `TexFilter::Mipmapped`. Its metrics differ (wider/narrower
  than parchment, full Latin-1 maybe): tune size and spacing per font so text still fits its panels; check
  the widest screens (inventory, skills, journal text, credits). Kalam / courier / impact text stays as is.
* Fonts load on a screen's first frame: a changed setting must take effect on screens already loaded (reload fonts on
  change, or at screen open).
* Docs: ui.md Fonts section, settings.md, glossary row if a term is new.

## Fixed when

* The option switches body and small text in every screen, headlines unchanged; persists across restarts.
* A test covers the settings round trip and the default; a scenario (`tests/scenarios/`) screenshots both fonts.

## Play-test

* Options: switch the font, open the inventory, skills, journal, credits: text readable, nothing overflows its panel.
