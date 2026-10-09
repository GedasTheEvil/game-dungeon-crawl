# Inventory keys with tabs

Status: implemented 2026-10-09, not play-tested yet. Draft 2026-10-06, refined 2026-10-09. Split off
[inventory-overhaul.md](solved/inventory-overhaul.md), which keeps today's keys.

## Problem

The number row (`HOTKEYS`, `1234567890-=`, `src/ui/inventory.cpp`) picks the open tab's first 12 slots, each of
those slots shows its key in the corner. The grid shows 8 (4 x 2, `VISIBLE_ROWS`) and scrolls, so a key can pick a slot
out of view, and the slots past the 12th have none. A stopgap since the [Egyptian weapons](solved/egyptian-weapons.md).

## Decided (2026-10-09)

* **Number keys do nothing in the inventory.** Picking is by mouse and WASD; E / Enter / Space uses, U upgrades.
* **No key labels on the slots.** `HOTKEYS`, `HOTKEY_COUNT` and the corner label go.
* **Tab / Shift+Tab switch tabs**, next and previous, wrapping around, skipping disabled tabs (`TabEnabled`).
* The in-game weapon and potion hotkeys stay as they are.
* The help text and the inventory footer (if they name the number keys) follow; `docs/ui.md` too.
* Tests: a scenario (Tab cycles, a disabled tab skipped, a number key leaves the selection alone).

## Implemented (2026-10-09)

* Tab opens the next enabled tab, wrapping round (`Inventory::NextTab`). **No Shift+Tab:** freeglut never delivers
  it. X sends ISO_Left_Tab, which has no ASCII text, so the keyboard callback only sees the Shift special key (checked
  with a small GLUT program under Xvfb + `xdotool key shift+Tab`). With three groups at most, Tab alone goes round fast.
* Number keys fall through to nothing; `HOTKEYS`, `HOTKEY_COUNT` and the slot labels are gone; the footer reads
  "Arrows: browse    Tab: next group".
* Scenario `tests/scenarios/inventory_keys.txt`; new `expect` fields `tab` and `selected` ([testing](../testing.md)).
