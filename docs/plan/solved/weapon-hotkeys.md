# Weapon hotkeys

Status: done.

Number keys equip a weapon directly, without opening the inventory:

| Key | Weapon |
|-----|--------|
| `1` | club   |
| `2` | sword  |
| `3` | spear  |
| `4` | bow    |

The keys are the inventory slot hotkeys (`HOTKEYS` in `src/ui/inventory.cpp`), so the inventory slots already show
them. The options controls table lists them too.

## Decisions

* No weapon of that kind yet: the key does nothing.
* One slot per weapon kind (copies raise its level), so there is nothing to pick or cycle.
* During a swing or a bow draw (`Player::attackStartMs >= 0`) the key does nothing.
* Potion keys (`5`-`0`, `-`) do nothing in game; in the inventory the number keys still only select a slot.

## Code

* `Inventory::EquipHotkey` (`src/ui/inventory.cpp`), called from `keyPressed` via
  `PlayerActionController::equipHotkey` (`src/input/input.cpp`).
* Test: `tests/scenarios/weapon_hotkeys.txt`.
