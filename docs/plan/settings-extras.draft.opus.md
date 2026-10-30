# More settings

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

Opus: a shader change (brightness) and several Options rows.

## Problems

Options has window size, fullscreen, animation rate, frame rate, motion effects, toon, blood, flicker, cut-scenes,
music and effects volume (`src/state/settings_ini.h`, Options rows in `src/ui/menu.cpp`). Missing:

* Brightness / gamma. The game is lit only by fire; monitors differ a lot.
* Mouse sensitivity (a constant, `MOUSE_LOOK_SENSITIVITY`, `src/input/input.h`) and invert Y.
* Master volume / mute.
* Window sizes stop at 1920x1080 (fixed list in `menu.cpp`).

## Plan

* Brightness: a gamma or exposure value applied in the final pass (where toon / ink already post-process,
  `src/graphics/`), range about 0.7-1.5, with a calibration hint ("the symbol should be barely visible").
* Mouse sensitivity slider and invert Y toggle.
* Master volume and a mute key or toggle.
* Window sizes: add the sizes up to the desktop size (query it), not a fixed list.
* All new keys in `settings.ini` and documented in `docs/settings.md`.
