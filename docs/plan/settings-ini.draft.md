# Settings in an ini file

Status: draft 2026-10-05.

## Idea

Options choices should stay between runs, in a file a player can also edit by hand. Today only "Motion effects" is
kept, in `saves/settings.txt` as `name value` lines (`src/state/settings.cpp`, from
[sprint-motion-effect.md](sprint-motion-effect.md)). Grow it into a proper settings file with sections, and put
more options in the Options screen.

## Format

INI, a small parser of our own (no library):

```ini
; Dungeon crawl settings. Unknown keys are kept as they are.
[display]
motion_effects = on
toon = off
window_width = 1280
window_height = 720
fullscreen = off

[sound]
music_volume = 80
effects_volume = 100
```

* `key = value`, `[section]`, `;` / `#` comments. Booleans `on/off` (also `1/0`, `true/false` when read).
* A bad value logs a warning and keeps the default; a missing file means all defaults, written on the first change.
* When saving: keep comments, the order and unknown keys (rewrite only the lines that changed), so hand edits
  survive.
* The parser is GL-free: it goes in `liblevel` or a small lib of its own, with unit tests (`tests/unit/`).

## Where the file lives

* Now: `saves/settings.txt`, next to the game. Options: keep it there as `saves/settings.ini`, or follow XDG
  (`$XDG_CONFIG_HOME/dungeon-crawl/settings.ini`, i.e. `~/.config/...`). Saves could move with it later.
* Moving from `settings.txt`: read it once if no ini exists, then write the ini.

## Candidate options

* Display: motion effects (exists), toon shading (F1 now, not saved), window size, fullscreen.
* Sound: music and effects volume, or mute (nothing like this exists yet; SDL_mixer has `Mix_Volume` /
  `Mix_VolumeMusic`).
* Gameplay: maybe camera look speed.
* Later, maybe: key bindings (the Controls tab is read-only today).
* Not: hitboxes (F3) stays a debug toggle.

## Notes

* Settings are in `RenderSettings` (`src/state/game_state.h`) now. A `Settings` struct of its own (display, sound)
  would hold what the file holds, with `RenderSettings` reading from it.
* The Options Display tab is one row (`MainMenu::DrawDisplay()` in `src/ui/menu.cpp`). More rows need a row list
  (name, hint, kind: on/off, choice, slider) instead of the single hard-coded switch. A slider is new UI
  ([docs/ui.md](../ui.md)).
* Scenarios never read or write the file (`Scenario::active()`), so test runs stay the same on every machine.
  A scenario command could load a given ini to test the parser end to end.
* Document the file and its keys in `docs/` (players may edit it).

## What to settle

* Location: `saves/` or `~/.config/`.
* Which options go in the first step: just a move to INI for the existing one plus toon, or with sound / window too.
* Keep comments and unknown keys on save, or just rewrite the whole file (simpler).
