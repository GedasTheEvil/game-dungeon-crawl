# Editor zoom: 1x, 2x, 3x map view with panning and a minimap

Status: draft, ready. Needs nothing first. From the user. Tooling only: no game change, so once implemented and its
checks pass, it goes straight to `docs/plan/solved/` (docs/plans.md, Solved).

## Today

`tools/editor/editor.cpp` draws the whole 40 x 47 map in `MAP_AREA` on a 160 x 100 canvas, `CELL = 1.8` canvas units
per cell. At 1280 x 800 a cell is about 14 px: hard to see and to hit.

## Idea

* Zoom 1x (today), 2x, 3x. Keys `+` / `-` (and `1` / `2` / `3` when no text field has focus), `Ctrl` + mouse wheel
  zooms around the cell under the mouse. The level stays 40 x 47; only the view changes.
* Zoomed, `MAP_AREA` shows a window of the map:
  * pan with the mouse wheel (up / down, `Shift` + wheel left / right), the arrow keys, or a middle-button drag;
  * the view is clamped to the map's edges.
* **Minimap**, zoomed only: the whole map at a small scale in a corner of `MAP_PANEL` (the corner away from the mouse,
  or a fixed one; the implementer's choice), with a frame for the visible window. Click or drag in it to move the
  window. It draws the structure only (or all layers dimmed), no icons.
* The cell under the mouse, paint, drag paint, right click pick, the check mode's gold path dots and the hover line
  above the grid work at every zoom: one function maps mouse to cell and cell to rect with the zoom and offset
  (`cellRect`, the mouse to cell code near `editor.cpp:277`). Draw the map clipped to `MAP_AREA` (`glScissor`).
* Icons scale with the cell (they are textures already). Text drawn in cells, if any, too.
* The zoom and offset are view state only: not saved in the level.

## Code

* `editor.cpp`: `zoom` (1..3) and `viewCol` / `viewRow` (bottom left cell shown) in `Editor`; `cellRect` and the
  mouse to cell lookup use them. freeglut reports the wheel as buttons 3 / 4 in `glutMouseFunc` (or
  `glutMouseWheelFunc`).
* `tools/editor/readme.md`: a `Zoom` section (keys, wheel, minimap), the screen layout line on the left side.

## Tests

The editor has no tests today; the mouse to cell mapping can get a unit test if it is a free function
(`tests/unit/`). Check by hand: `make editor`, then run it bounded and silent
(`xvfb-run -a timeout -k 5 10 ...`) once to see it starts; the user tries the zoom.
