# More settings

Status: implemented 2026-11-08, waiting for the user's play-test. Was: draft 2026-10-30, size M, needs nothing first. From the project audit of 2026-10-30.

Opus: a shader change (brightness) and several Options rows.

## Problems

Options has window size, fullscreen, animation rate, frame rate, motion effects, toon, blood, flicker, cut-scenes,
music and effects volume (`src/level/settings_ini.h`, Options rows in `src/ui/menu.cpp`). Missing:

* Brightness / gamma. The game is lit only by fire; monitors differ a lot.
* Mouse sensitivity (a constant, `MOUSE_LOOK_SENSITIVITY`, `src/level/input.h`) and invert Y.
* Master volume / mute.
* Window sizes stop at 1920x1080 (fixed list in `menu.cpp`).

## Plan

* Brightness: a gamma or exposure value applied in the final pass (where toon / ink already post-process,
  `src/graphics/`), range about 0.7-1.5, with a calibration hint ("the symbol should be barely visible").
* Mouse sensitivity slider and invert Y toggle.
* Master volume and a mute key or toggle.
* Window sizes: add the sizes up to the desktop size (query it), not a fixed list.
* All new keys in `settings.ini` and documented in `docs/settings.md`.

## Done

* Brightness: `[display] brightness` 70-150 %, a gamma over the whole finished frame (`render/brightness.cpp`, the
  curve `brightenedChannel`), menus and HUD included; the Display tab's Brightness slider has a calibration box (a
  near-black ankh in a black square).
* New Mouse tab: Mouse sensitivity slider (`[mouse] sensitivity` 20-300 %), Invert mouse Y, and Camera recentre moved
  there from Display (its ini key stays in `[graphics]`).
* Sound tab: Master volume (`[sound] master_volume`, scales music and effects, `playedVolume`) and a Mute switch
  (`mute`). No mute key: the switch covers it, and the free F keys are left to the controls-help plan.
* Window size row: the common sizes up to 7680x4320 that fit the desktop (`glutGet(GLUT_SCREEN_*)`), then the
  desktop's own size (`windowSizes`).
* Unit tests: `tests/unit/settings_test.cpp`; screenshots: `tests/scenarios/settings_extras.txt`.

## Play-test

`bin/game --level 1`, Esc, Options:

* Display: drag Brightness; the whole picture (menu too) lightens / darkens, the ankh in the black box goes from
  invisible to clear. Set it so the ankh is barely visible, back to the game: the dungeon is lit to match.
* Mouse: Mouse sensitivity changes how fast the view turns with the mouse; Invert mouse Y flips up / down looks.
* Sound: Master volume lowers music and effects together; Mute silences both and keeps the sliders.
* Display: Window size steps past 1920 x 1080 up to your desktop's size. Restart: every choice is kept.
