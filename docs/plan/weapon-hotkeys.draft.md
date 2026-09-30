# Weapon hotkeys

Status: idea, not refined.

## Idea

Number keys equip a weapon directly, without opening the inventory:

| Key | Weapon |
|-----|--------|
| `1` | club   |
| `2` | sword  |
| `3` | spear  |
| `4` | bow    |

## Code pointers

* Key constants: `src/input/input.h` (`KEY_ATTACK`, `KEY_INVENTORY`, ...); handling in `src/input/input.cpp`.
* Equipped weapon: `Inventory::equippedSlot`, `Equipped()`, `EquippedType()` in `src/ui/inventory.h`.

## Open questions

* No weapon of that kind in the inventory: ignore the key, or show a short message?
* Several weapons of the same kind (different levels): pick the best one, or cycle on repeated presses?
* Switch during an attack or bow draw: block it, or cancel the attack?
* Show the hotkeys in the inventory or the HUD?
