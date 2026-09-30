# Quick potions: hotkeys for healing and stamina

Status: idea, not started. After the [HUD redesign](hud-redesign.draft.md): the HUD shows the quick slots.

## What

Drink a potion in a fight without opening the inventory:

| Key | Action |
|---|---|
| `1`-`4` | equip club / sword / spear / bow (exists) |
| `Q` | drink the best fitting **healing** potion |
| `R` | drink the best fitting **stamina** potion |

`Q` and `R` sit next to `W A S D` and `E` (interact), so the left hand never leaves the movement keys. Neither is
bound today (`src/input/input.h`). Mouse alternative: none needed; the mouse buttons are attack, interact and jump.
Add both to the options controls table (`CONTROLS` in `src/ui/menu.cpp`).

## Best fit

Potions today (`Inventory::DrinkPotion`, `src/ui/inventory.cpp`): small health +25 HP, large health +50 HP, small
stamina +50% stamina, large stamina full stamina. (Life +5% max HP and full heal, might, armor: not quick potions,
they are rare and permanent; keep them inventory only.)

Rule: **drink the weakest potion that is not wasted; the stronger one only when it is needed.**

1. Missing = max - current (HP or stamina). Nothing missing: do nothing, show "Health is full" like the inventory.
2. Candidates: the potions of that kind the player has.
3. Pick the weakest candidate. Take the stronger one only if the weakest would leave the player in danger:
   health still below `QUICK_HEAL_DANGER` (e.g. 35% of max) after the small one while the large one exists.
   Example: 134 max HP, at 30% (40 HP). The small potion takes them to 65 HP (49%): out of danger, drink the small
   one; the large one is saved. At 10% (13 HP), the small one leaves 38 HP (28%): still in danger, drink the large one.
4. Stamina: the same, with the danger line at the cost of the next jump or a short sprint (`JUMP_STAMINA_COST`).

The rule lives in a pure function (`quickPotion(kind, current, max, counts) -> potion id or none`) so a unit test or
scenario can check the table of cases without a fight.

## HUD

The quick slots in the [HUD redesign](hud-redesign.draft.md) show what `Q` / `R` would drink right now (the choice
changes with the health), the count of that potion, and the key cap. Nothing to drink: the slot is dimmed. After a
drink the slot flashes and the status box says "Healed 25 health", like drinking from the inventory.

## Rules to keep

* Drinking takes no extra time today (inventory use is instant). For quick potions consider a short cooldown (~1 s,
  a `Timer`) so mashing `Q` does not drink the whole stack in one frame.
* Not while dead, not in menus, the map or the riddle screen (the same gate as the weapon hotkeys:
  `ScreenState::IsGameplayInteractionAllowed`).
* Allowed during an attack (unlike the weapon switch): drinking does not change the swing.

## Tests

* The pure function against a table: counts none / small only / large only / both, health at 10%, 30%, 60%, 100%.
* Scenario: `give potion 0 2`, `give potion 1 1`, take damage (a new `hurt N` command, or stand in a trap), `key q`,
  expect the small potion used (`expect potion0 == 1`) and HP up by 25; lower HP, `key q`, expect the large one used.
  The same for stamina with `key r`. Screenshots of the HUD slots before and after.

## Open questions

* `Q` / `R`, or `H` (heal) / `G`? Or the number row after the weapons (`5`, `6`)? The number row is already the
  inventory's slot keys, where `5`-`0` select potions, so letters seem clearer.
* Should the player be able to pin a potion for the hotkey in the inventory (overriding the best fit)?
