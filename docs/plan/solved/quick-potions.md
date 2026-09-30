# Quick potions: hotkeys for healing and stamina

Status: implemented 2026-09-30 (see [Implementation](#implementation)); verified in play 2026-09-30. The
[HUD redesign](hud-redesign.md) shows the quick slots.

## What

Drink a potion in a fight without opening the inventory:

| Key | Action |
|---|---|
| `1`-`4` | equip club / sword / spear / bow (exists) |
| `H` | drink the best fitting **healing** potion (H for heal) |
| `0` | drink the best fitting **stamina** potion |

Keys chosen by the user (2026-09-30). `H` is not bound today (`src/input/input.h`). `0` is the inventory's slot key
of the small stamina potion (`HOTKEYS` in `src/ui/inventory.cpp`), so in the inventory it keeps selecting that slot;
in game it drinks (today it does nothing there: `Inventory::EquipHotkey` ignores potion keys). Mouse alternative:
none needed; the mouse buttons are attack, interact and jump. Add both to the options controls table (`CONTROLS` in
`src/ui/menu.cpp`).

## Best fit

Potions today (`Inventory::DrinkPotion`, `src/ui/inventory.cpp`): small health +25% of max HP, large health +50% of
max HP (`PlayerStats::Heal` takes a percent), small stamina +50% stamina, large stamina full stamina. (Life +5% max HP and full heal, might, armor: not quick potions,
they are rare and permanent; keep them inventory only.)

Rule: **drink the weakest potion that is not wasted; the stronger one only when it is needed.**

1. Missing = max - current (HP or stamina). Nothing missing: drink nothing, the status box says "You are at full
   health" (`H`) or "You are at full energy" (`0`).
2. Candidates: the potions of that kind the player has.
3. Pick the weakest candidate. Take the stronger one only if the weakest would leave the player in danger:
   health still below `QUICK_HEAL_DANGER` (e.g. 35% of max) after the small one while the large one exists.
   Example: 134 max HP, at 30% (40 HP). The small potion (+33) takes them to 73 HP (54%): out of danger, drink the
   small one; the large one is saved. At 5% (7 HP), the small one leaves 40 HP (30%): still in danger, drink the
   large one.
4. Stamina: the same, with the danger line at the cost of the next jump (`JUMP_STAMINA_COST`). The small potion
   (+50%) always clears it, so the large one is drunk only when no small one is left.

The rule lives in a pure function (`quickPotion(kind, current, max, counts) -> potion id or none`) so a unit test or
scenario can check the table of cases without a fight.

## HUD

The quick slots in the [HUD redesign](hud-redesign.md) show what `H` / `0` would drink right now (the choice
changes with the health), the count of that potion, and the key to press as a key cap (`H`, `0`) under the slot.
No potion of that kind left: the slot is shown empty (the tile without an icon or count), the key cap stays. After a
drink the slot flashes and the status box says "Healed 25 health", like drinking from the inventory.

## Rules to keep

* Drinking takes no extra time today (inventory use is instant). For quick potions consider a short cooldown (~1 s,
  a `Timer`) so mashing `H` does not drink the whole stack in one frame.
* Not while dead, not in menus, the map or the riddle screen (the same gate as the weapon hotkeys:
  `ScreenState::IsGameplayInteractionAllowed`).
* Allowed during an attack (unlike the weapon switch): drinking does not change the swing.

## Tests

* The pure function against a table: counts none / small only / large only / both, health at 10%, 30%, 60%, 100%.
* Scenario: `give potion 0 2`, `give potion 1 1`, take damage (a new `hurt N` command, or stand in a trap), `key h`,
  expect the small potion used (`expect potion0 == 1`) and HP up by 25; lower HP, `key h`, expect the large one used.
  The same for stamina with `key 0`. Screenshots of the HUD slots before and after.
* At full health / stamina: `key h` / `key 0` drinks nothing (counts unchanged), the status message shows.

## Decided

* No pinning (2026-09-30): the hotkeys only ever drink health (`H`) or stamina (`0`) potions, always the best
  fitting one by the rule above.
* Cooldown 1 s (`QUICK_DRINK_COOLDOWN_MS`) between quick drinks.
* No potion of that kind left: nothing drunk, the status box says "No healing potion left" / "No stamina potion left".

## Implementation

* `src/ui/quick_potion.{h,cpp}`: `quickPotion(kind, current, max, smallCount, largeCount)`, the potion strengths
  (`PotionEffect`, also used by `DrinkPotion`), `QUICK_HEAL_DANGER_PERCENT` (35), the cooldown.
* `Inventory::QuickDrink` / `QuickChoice` / `QuickDrinkMs`; `DrinkPotion` returns its message (the inventory
  toasts it, a quick drink puts it in the status box). Keys in `PlayerActionController::quickDrink`
  (`src/input/input.cpp`), in the same gate as the weapon hotkeys but also during a swing.
* Options controls table: "Drink healing / stamina potion: H / 0" (rows 3.5 high to fit 13).
* No unit test framework yet: the rule is checked by `tests/scenarios/quick_potions.txt`, with the new scenario
  command `hurt N`.
