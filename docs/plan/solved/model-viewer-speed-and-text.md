# Model viewer: animation speed keys, readable text

Status: done 2026-10-07.

## Problem

* The loop duration of `tools/model-viewer/viewer.cpp` is fixed by the `[seconds]` argument (default 5.0). Changing
  it means restarting the viewer.
* The stats text (`fonts/papyrus_i.png`, drawn straight over the 3D view) is hard to read on most models and
  backgrounds.

## Idea

* `+` / `-` (also keypad `+` / `-`, `=` as unshifted `+`) speed up / slow down the animation: change
  `gDurationSeconds` by a factor (e.g. x0.8 / x1.25), clamped. Keep the loop position (ratio) when the speed changes,
  so the model does not jump. Show the current duration or speed in the stats text. Add the keys to the usage
  comment at the top of the file.
* Draw the text on a box in line with the game's UI ([ui.md](../../ui.md)): `ui::panel` (dark brown fill, bronze
  frame), as the status box does, with `GOLD` / `LABEL` text. `ui_draw` is already in `librender.a`, which the
  viewer links. The user's first idea was a semi-transparent white box; the papyrus look (light fill, ink text) is
  the alternative that fits that.

## Open

* Panel style: dark panel with gold text, or papyrus with ink text.
* Speed step and limits.

## Done

* `+` / `=` divide the loop duration by 1.25, `-` / `_` multiply it, clamped to 0.2–60 s (`SetDuration`); the loop
  start moves so the pose stays. The `[seconds]` argument is clamped the same way.
* Stats on a `ui::panel` at the top right (status box look): dark panel, `GOLD` text in the UI body font, the name in
  `GOLD_BRIGHT`, a key hint in the small font in `GOLD_DIM`. Lines: name, clip i / n, frames and polygons, loop
  seconds, texture i / n and its file.
