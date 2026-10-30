# Draft map: symbols, zoom, markers

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

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
