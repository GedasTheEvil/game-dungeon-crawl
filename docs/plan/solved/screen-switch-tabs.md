# Screen switch tabs

Status: done 2026-10-01, verified in play (`tests/scenarios/screen_tabs.txt`). Tiles at the top right,
centred on the title rule (the title rule is shorter on these screens); the inventory icon is a chest. Look:
[docs/ui.md](../../ui.md#screen-tabs).

## Idea

The in-game screens (inventory, map, the upcoming [monster journal](monster-journal.md)) get a row of icons
to jump between them with the mouse, without closing one and opening the next.

## Look

* A small tab strip in the same spot on every screen, e.g. top corner next to the title (y ~88–98), in the shared
  look ([docs/ui.md](../../ui.md)).
* One icon per screen: bag / chest (inventory), scroll (map), notebook (journal). The open one is the active tab
  (lapis), the others stone.
* Key cap under or beside each icon (I, M, J) so the strip also teaches the keys.
* Hover tooltip with the screen name.

## Behaviour

* Click fires on mouse up over the same tab (shared button rule).
* Keys switch directly too: M in the inventory opens the map, I in the map opens the inventory. Today the inventory
  eats every key except I and the map only reacts to M / Esc (`src/input/input.cpp`, `keyPressed`). Pressing the key
  of the open screen still closes it to the game.
* Esc still goes back to the game from any of them.
* Map canvas has the window's width (`beginSquareCanvas`), the others are 160 x 100: the strip must sit at the same
  screen position on both, so anchor it to the 160 x 100 area or give it its own fixed-height canvas like the HUD.

## Code

* One shared part in `src/ui/ui_draw.h`, e.g. `ui::drawScreenTabs(Screen active, hover)` + a hit test returning the
  clicked `Screen`, so each screen only calls it.
* Mouse routing: each screen's click handler asks the strip first.
* Scenario test: open inventory, click map tab, click journal tab, screenshot each.

## Decided

* Riddle and menu stay out (riddle is modal, menu is a different flow).
* Tab order: inventory, map, journal.
