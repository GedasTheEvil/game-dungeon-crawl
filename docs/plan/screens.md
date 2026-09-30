# Stage 8: one screen at a time, shared screen parts

Status: implemented 2026-09-30, not yet reviewed by the user. Stage 8 of the
[code structure review](code-structure-review.draft.md).

## Why

Four `show` flags (menu, inventory, riddle, map) picked the screen in a priority order, and only ad hoc checks in
`input.cpp` kept two from being open at once. Every screen copied the canvas size, the font loading, the toast and
the end of the frame (which pulled `test/scenario.h` into `ui/`); five HUD parts copied the square-canvas setup.

## Done

* `8a` (`2f82bc7`): `Screen` (`ui/screen.h`) and `GameState::ui.screen` replace the four flags. A screen stack was
  not needed: the input never opens one screen over another (the menu opens from the game only, the map and the
  inventory exclude each other, the riddle takes all keys). Screenshots pixel-identical, all scenarios pass.
* `8b` (`c051169`): `Draw()` ends the frame once for every screen; `ui::CANVAS_W` / `CANVAS_H`, `ui::Toast`,
  `ui::loadScreenFonts`, `ui::beginSquareCanvas`; GLUT's own `GLUT_DOWN` / `GLUT_UP`. `ui/` no longer includes the
  scenario runner. [../ui.md](../ui.md) updated. Pixel-identical.

## Not done

* The press / hover / release click handling of the menu and the inventory stays twice: the inventory has slots
  that sink while held and right-click to use, the menu buttons only; a shared helper would need both cases.
* The GL-state reset at the end of three HUD parts (two lines each) stays.
