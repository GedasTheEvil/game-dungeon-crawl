# The item bag belongs to the game state, not the inventory screen

Status: solved 2026-11-16. Was: draft 2026-11-16, size S, needs nothing first.

## Problem

The architecture review (2026-11-16, smell 6): the player's `ItemBag` was a member of the inventory screen
(`ui/inventory.h`). The save, the sim links, the input's player actions, the HUD and the scenario runner reached it
through `ui.inventory->Bag()`, so the run's state lived in a widget that exists only after `GameState::Load`.

## Plan

* `GameState::bag` owns it; `Inventory` takes an `ItemBag&`.
* Save, sim links, input, HUD and scenario read `Game().bag`; the screen's read-only pass-throughs used outside it
  (`EquippedKind`, `Level`) are gone.
* Not done: the gameplay actions still in the screen (quick drink, equip, wear, add item) and their cooldowns; a
  separate plan if wanted.
