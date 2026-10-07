# Attack timers and weapon hotkeys per class

Status: implemented 2026-10-07, not play tested. Draft 2026-10-07, split from [egyptian-weapons.draft.md](egyptian-weapons.draft.md): needs no new weapons,
works with today's club, sword, spear and bow, and comes before them.

## Attack timers

Every weapon has two timers instead of one attack time:

* **Frame delay**: from the attack key until the hit lands (melee) or the shot leaves (ranged): the wind-up, the
  draw. The risk: a slow wind-up lets the monster strike first.
* **Recovery**: from the hit or shot until the next attack can start. The opening: no attack meanwhile.

Today both exist, only not named (`WeaponMotion`, `src/entities/item.h`): `motion.hitMs` is the frame delay (the
bow's is `BOW_DRAW_MS`, 450, `src/core/gameplay_config.h`), and `motion.attackMs` counts from the key press, so
recovery is `attackMs - hitMs`. Replace `attackMs` with a recovery value in every `ITEM_DEFS` row
(`src/state/assets.cpp`), so one timer can be tuned without the other. `swingMs` stays the animation's return to rest
and runs inside the recovery.

Moving or sprinting does not cancel a recovery: a swing or shot is a commitment.

Today's weapons keep their timing, only split:

| Weapon | Frame delay ms | Recovery ms |
|---|---|---|
| Club | 300 | 600 |
| Sword | 180 | 370 |
| Spear | 200 | 550 |
| Bow | 450 | 550 |

## Hotkeys per class

Replaces the fixed keys of [weapon-hotkeys.md](solved/weapon-hotkeys.md) (`1` club, `2` sword, `3` spear, `4` bow):

* `1` equips the next owned melee weapon, `2` the next owned ranged one, in inventory order, wrapping around.
* The class's weapon not equipped yet: the key equips its first owned weapon of that class.
* None of that class owned, or during a swing or draw (as today): the key does nothing.
* `3` and `4` are free again. Inventory slot hotkeys stay as they are.

With one ranged weapon today, `2` just equips the bow; the cycling pays off with the
[Egyptian weapons](egyptian-weapons.draft.md).

## Work

* `WeaponMotion` and `ITEM_DEFS`: recovery instead of `attackMs`; the attack gate in `Player` uses frame delay plus
  recovery.
* `Inventory::EquipHotkey` (`src/ui/inventory.cpp`), `PlayerActionController::equipHotkey` (`src/input/input.cpp`),
  the options controls table.
* Tests: `tests/scenarios/weapon_hotkeys.txt` for the cycling; a scenario that sprints during a recovery and checks the
  next attack waits.

## Done

* `WeaponMotion::recoveryMs` replaces `attackMs`; `AttackMs()` (frame delay plus recovery) is the gate between
  attacks. `hitMs` stays the frame delay's name.
* Bindings `equip_melee` (`1`) and `equip_ranged` (`2`) replace `equip_club` .. `equip_bow`; `Inventory::EquipNext`
  cycles. The HUD weapon slot shows `1-2`, the weapons field note names the two keys.
* Scenarios equip with the new `equip WEAPON` command instead of the number keys; `expect attacking` reads whether a
  swing or draw is under way.
* Tests: `weapon_hotkeys.txt` (cycling, first of the class, free keys), `attack_recovery.txt` (sprinting through the
  club's recovery; the next attack waits until 900 ms).
