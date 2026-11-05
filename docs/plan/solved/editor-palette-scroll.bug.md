# Editor palette scrolls when its icons overflow the panel

Status: implemented 2026-11-05, waiting for the user's play-test. Was: draft 2026-11-05, size S, needs nothing first.

## Repro

1. `make run-editor`, pick the Decor layer.

## Expected

All palette icons stay inside the palette panel (`TILES_PANEL`, `tools/editor/editor.cpp`). When a layer has more
slots than the panel holds, the mouse wheel over the panel scrolls the rows, as the papyrus hint text already does
(`hintScroll`, `HINT_SCROLL_STEP`, `hintScrollMax`).

## Actual

The panel holds 3 rows of `PALETTE_COLUMNS` (9). The decor layer has 34 slots (`PLACED_UNSET`, `PLACED_BARE`,
`DECOR_COUNT` 32): its 4th row draws below the panel, over the brush panel (`BRUSH_PANEL`). The comment on
`PALETTE_COLUMNS` ("4 rows hold the decor layer's 30 slots") is stale. Other layers grow too (decals 20, looks,
tile types) and will overflow the same way.

## Notes

* `slotRect(slot)` lays rows from the panel's top down; add a row offset (scroll in rows, whole rows only), and only
  draw and hit-test (`hoveredSlot`, around line 420) the slots whose rect is inside `TILES_PANEL`.
* The wheel over `TILES_PANEL` scrolls one row per notch, clamped to `0..rows - visibleRows`. Reset the scroll on a
  layer switch (as `hintScroll = 0.f`), and scroll so the selected slot shows when the brush changes by key.
* Show that there is more: a small up / down arrow (or a dimmed edge) at the panel's top / bottom when rows are hidden.
  Keep the shared UI look ([ui.md](../ui.md)).
* A testable helper (visible rows, max scroll, slot at a point for a scroll) in a header so a unit test can pin it:
  with 34 slots, 9 columns and 3 visible rows, max scroll is 1 row and no visible rect leaves the panel.
* Update `tools/editor/readme.md` (the wheel over the palette).

## Fixed when

* A test that names the plan fails before the fix and passes after it.
* No layer's palette draws outside `TILES_PANEL`; every slot is reachable by scrolling.

## Play-test

* Play command: `make run-editor` (the editor, not `bin/game`).
* In it, Decor layer: 3 rows show, the wheel over the palette reveals the 4th row; nothing draws over the
  brush panel.
* Switch to Decals and Looks: the palette starts at the top row, wheel scroll works where they overflow.
