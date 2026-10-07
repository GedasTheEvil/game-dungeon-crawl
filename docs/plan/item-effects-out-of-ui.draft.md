# Item effects out of the inventory screen

Status: draft 2026-10-07. From the [architecture review](solved/architecture-review.md), refactor 1.

## Problem

`Inventory::DrinkPotion` and `QuickDrink` / `QuickDrinkMs` (`src/ui/inventory.cpp:201-315`) are game rules in a
screen: they apply potion gains to `PlayerStats`, run the quick-drink cooldown, play the sound and build the toast
text. The HUD and the hotkeys reach them through the inventory. Amulets ([amulets](amulets.draft.md)) and the
[resistance potion](resistance-potion.draft.md) would add more rules there.

## Idea

* Move the effect of using an item into the level library next to `ItemBag` / `potionGain` (`src/world/item_bag.cpp`):
  a function that takes the bag, the player stats and the kind, applies the effect and returns what happened (gain,
  cured poison, ...). No `Game()`, no sound.
* The screen and the hotkeys call it, then play the sound and show the toast from the result (or via `WorldEvents`).
* Unit tests for each potion's effect and the quick-drink cooldown.
* `PlayerStats` is in `src/entities/` and uses `GameClock` (`core/timer`, render lib): see
  [sim-unit-tests](sim-unit-tests.draft.md) step 1, which this needs first or together.

## Open

* Result struct vs `WorldEvents` for the sound and toast.
