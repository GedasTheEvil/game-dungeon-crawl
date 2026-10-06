# Inventory keys with tabs

Status: draft 2026-10-06. Split off [inventory-overhaul.draft.md](inventory-overhaul.draft.md), which keeps today's
keys.

## Problem

The number row picks one of 12 inventory slots (`HOTKEYS`, `1234567890-=`, `src/ui/inventory.cpp`) and each slot shows
its key in the corner. With tabs and more items than keys, that no longer maps one to one.

## Open

* What the number keys pick in the inventory: a slot in the current tab (1-8 for the visible rows), or nothing.
* The slot key labels: kept, changed to match, or dropped.
* Keys that switch tabs: Q / E, Tab / Shift+Tab; disabled tabs skipped.
* The in-game weapon and potion hotkeys stay as they are.
