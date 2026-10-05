# Settings in an ini file

Status: draft 2026-10-05, refined 2026-10-05: all settled, ready to implement.

## Idea

Options choices should stay between runs, in a file a player can also edit by hand. Today only "Motion effects" is
kept, in `saves/settings.txt` as `name value` lines (`src/state/settings.cpp`, from
[sprint-motion-effect.md](solved/sprint-motion-effect.md)). Grow it into a proper settings file with sections, put
more options in the Options screen, and let the player remap the controls.

## Format

INI, a small parser of our own (no library):

```ini
; Dungeon crawl settings. The game rewrites this file on every change: own comments and unknown keys are lost.

; Window size in pixels, fullscreen on/off.
[display]
window_width = 1280
window_height = 720
fullscreen = off

; Visual effects, each on/off.
[graphics]
motion_effects = on
toon = off
blood = on
light_flicker = on

; Volumes 0-100.
[sound]
music_volume = 80
effects_volume = 100

; Up to two keys and one mouse button an action, "none" to clear.
[controls]
move_left = a, left
jump = space, mouse_right
attack = v, enter, mouse_left
```

* `key = value`, `[section]`, `;` / `#` comments. Booleans `on/off` (also `1/0`, `true/false` when read).
* A bad value logs a warning and keeps the default; a missing file means all defaults, written on the first change.
* Decided: saving rewrites the whole file from the settings, every key in a fixed order, with a comment line before
  each section that says what its keys mean. Hand-edited values survive (they were read in); hand-written comments
  and unknown keys do not.
* The parser is GL-free: it goes in `liblevel` or a small lib of its own, with unit tests (`tests/unit/`).

## Where the file lives

Decided: with the saves, `saves/settings.ini` (next to `save*.sav` and `gamelist.dat`).

* Moving from `settings.txt`: read it once if no ini exists, then write the ini and delete the old file.

## Key remapping

Decided: the existing controls can be remapped in Options > Controls (read-only today, `CONTROLS` rows in
`src/ui/menu.cpp`).

How the input works today (`src/input/input_actions.h`, `src/input/input.cpp`): GLUT sends ASCII keys
(`keyPressed`, upper and lower case arrive as different keys) and special keys (`specialKeyPressed`: arrows, F-keys,
PgUp/PgDn, Home/End, Shift) separately. `MapKeyboardGameplayAction`, `MapSpecialGameplayAction` and
`MapMouseGameplayAction` are hard-coded switches. Camera look, sprint (Shift) and the screen keys (I / M / J,
`ScreenTabs::ForKey`) are handled outside them.

* A binding table: action -> up to two keys + one mouse button. A key is either ASCII (stored lower case, both
  cases match) or special. The three switches become lookups in the table.
* Actions to bind: move left / right, climb up / down, jump, sprint, attack, interact, look left / right / up / down,
  equip club / sword / spear / bow, quick heal / stamina, inventory, map, journal.
* Not bindable: Esc (menu / back, always works, so a bad mapping can be undone), the F-keys (F1 toon, F3 hitboxes,
  debug-ish). F12 as a second interact key goes into the table as a default.
* Controls tab: select a row, "Press a key" prompt, the next key or mouse button binds it, Esc cancels. A key that is
  already bound elsewhere moves to the new action (the old one shows it is now unbound). A "Reset to defaults" row.
* Labels follow the bindings: the Controls tab rows and the HUD key caps (`QUICK_HEAL_KEY_LABEL`,
  `QUICK_STAMINA_KEY_LABEL`, `EQUIP_KEYS_LABEL`, the interact hint) read from the table, not from constants.
* Scenarios always use the default bindings (they never read the file), so `tests/scenarios/` key commands stay valid.

## Graphics options

Decided: separate on/off switches (a switch or a checkbox per row), no overall detail level. The game is light on the GPU
(fixed-function plus small GLSL programs), so a Low / Medium / High preset would save little and hide what it
changes. Switches are about taste and comfort:

* Motion effects (exists): sprint blur, FOV kick, vignette.
* Toon shading (F1 now, not saved): cel bands and ink outlines.
* Blood splashes (`ParticleSystem`): on, or off for a gentler look.
* Light flicker (`LightDef::flicker` of torches, braziers, lamps): off for players bothered by flicker.
* Window size and fullscreen (`RenderSettings::resX` / `resY`, `glutInitWindowSize`).
* Not: hitboxes (F3) stays a debug toggle.

A preset can come later if a slow machine needs it, as a row that only sets the switches above.

## Sound

Music and effects volume, 0-100 (nothing like this exists yet; SDL_mixer has `Mix_Volume` / `Mix_VolumeMusic`).

## Scope

Decided: all of it in one go: the INI file and its move from `settings.txt`, the graphics switches, window size and
fullscreen, the sound volumes and the key remapping.

Later, maybe: camera look speed, a graphics preset.

## Notes

* Settings are in `RenderSettings` (`src/state/game_state.h`) now. A `Settings` struct of its own (display, graphics,
  sound, controls) would hold what the file holds, with `RenderSettings` reading from it.
* The Options Display tab is one row (`MainMenu::DrawDisplay()` in `src/ui/menu.cpp`). More rows need a row list
  (name, hint, kind: on/off, choice, slider, key binding) instead of the single hard-coded switch. A slider and the
  key prompt are new UI ([docs/ui.md](../ui.md)). The tabs may grow to Controls / Display / Sound.
* Scenarios never read or write the file (`Scenario::active()`), so test runs stay the same on every machine.
  A scenario command could load a given ini to test the parser end to end.
* Document the file and its keys in `docs/` (players may edit it).
