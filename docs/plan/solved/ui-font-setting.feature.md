# UI font setting: parchment or DejaVu condensed bold

Status: solved 2026-11-23 (play-tested by the user). Was: implemented 2026-11-23, waiting for the user's play-test. Was: draft 2026-11-20, size M, needs nothing first.

## Idea

The user, 2026-11-20: a new option to pick the UI font: `parchment` (default) or `dejavu-condensed-bold`
(`media/textures/fonts/dejavu-condensed-bold.png`, already used by the level editor). It does not apply to headlines,
only the smaller text.

## Plan

* New `Settings` field (`level/settings_ini.*`, `state/settings.*`, [settings.md](../../settings.md)), a two-value enum, default
  parchment; an Options row in `ui/menu_options.cpp` (`tools/where.py options settings`). Old settings files: default.
* "Headlines" = the `title` and `heading` fonts of `ui::loadScreenFonts()` (`src/render/ui_draw.cpp`, [ui.md](../../ui.md#fonts));
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

* `bin/game --play tests/play/ui-font-setting.txt`; Esc > Options > Display > Text font: switch to DejaVu.
* Open the inventory (I), Heka (K), journal (J), How to Play and the credits' second page: body and small text in
  DejaVu, titles, tabs and big buttons still parchment, nothing overflows its panel.
* Restart the game: the font choice is kept. Options now has five tabs (Graphics split off Display).

## Implemented

* Settings: `UiFont` (`level/settings_ini.h`), `[display] ui_font = parchment | dejavu-condensed-bold`, default
  parchment, a bad name warns. Options > Display > Text font (Choice row, either step flips it).
* Options: the Text font row did not fit an eleventh Display row, so the `[graphics]` switches moved to a new Graphics
  tab (five tabs, 26 wide).
* `ui::loadBodyFont`, `ui::setBodyFont` (from `GameState::ApplySettings`), `ui::bodyFontGeneration`,
  `ui::ensureScreenFonts`: every screen (menu, inventory, Heka, journal, riddle, death, ending, win entry) reloads its
  body and small fonts on its next frame after a change; the HUD's `hudBody` / `hudSmall` through
  `FontSet::RefreshBodyFonts` at the start of each frame.
* Changed font: every `body` / `small` text (row names and hints, table text, footers, toasts, inventory details and
  its Equip / Upgrade buttons, Heka cards, How to Play rows and buttons, credits rows), the HUD numbers, key caps,
  status lines and the tooltip. Unchanged: `title`, `heading` (screen titles, tabs, menu buttons, choice tiles, section
  headings), the boss bar and zone banner, the loading screen, Kalam, Courier, Impact.
* DejaVu loads mipmapped at parchment's size and spacing: the line lengths come out about the same (checked in the
  scenario), so no per-font scale.
* Tests: `tests/unit/settings_test.cpp` (default, round trip, bad name), scenario `tests/scenarios/solved/ui_font_setting.txt`
  (each screen in both fonts).
