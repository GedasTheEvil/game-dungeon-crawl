# Draft map: symbols, zoom, markers

Status: implemented 2026-11-01, waiting for a play-test (`bin/game --play tests/play/map-symbols-and-zoom.txt`, then
`M`). Draft 2026-10-30, from the project audit of 2026-10-30.

## Problems

* `src/ui/map_view.cpp` has no symbol for teleporters or the boss gate; they fall back to `GRAPHITE`
  ([tile-table](solved/tile-table.md) left them out).
* `DraftMap` only draws (`src/ui/map_view.h`): no zoom or pan, no legend, no player markers, no maps of earlier levels.

## Plan

* Pencil symbols for the teleporter / ring platform, the boss gate, the portal, sphinx (riddle) gates, name gates;
  a small legend on the page.
* Zoom (mouse wheel, +/-) and pan (drag, arrows) when the level is larger than the page.
* Optional, implementer's choice: up to a few player markers (click to place, a pencil cross) saved per level.
* Earlier levels' maps: out of scope unless cheap.

## Done

* Symbols (`MapMark`, `markOf` in `src/ui/map_view.cpp`; one switch draws them): the entrance and the exit get the
  portal in the sphinx's arch, in their plasma's colour as in the game (entrance red, exit blue; the user, 2026-11-01),
  a sphinx gate a door with a blue question mark in it (the user), an empty sphinx a plain arch; a teleporter two
  columns with teal plasma between them; a ring platform three teal rings over a platform; a sealed name gate the columns with a gold cartouche between them; the boss gate an obsidian portcullis
  with a red seal while closed.
* Legend: a papyrus slip left of the sheet, one row per mark on the explored cells (drawn with the first such tile,
  so a key or gate row is in the colour found first), then "You" (the red ring). Nothing unexplored is named.
* Zoom and pan (`MapZoom`, `src/ui/map_zoom.h`, unit tests `tests/unit/map_zoom_test.cpp`): 1x is the old fit of the
  explored part; the wheel zooms round the mouse, `+` / `=` and `-` round the centre, up to 9 canvas units a cell; a
  left drag and the arrows / move keys pan within the 1x view. Clip planes keep the zoomed sketch on the sheet. A new
  level starts at 1x; the zoom is not saved.
* Player markers: left out (optional). They would need a save field per level and a way to remove them; the left
  button is the drag now, the right one is free for them.
* Earlier levels' maps: left out (out of scope; the explored cells of a level are not kept after it).
* Scenario `tests/scenarios/map_symbols.txt` (level `tests/levels/map_symbols`: every symbol along one walk) takes the
  symbols, zoomed, panned and zoomed-out screenshots at 4:3.
