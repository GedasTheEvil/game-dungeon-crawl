# Amulets

Status: draft 2026-10-06. Split off [resistance-potion.draft.md](resistance-potion.draft.md).

## Idea

* A new item kind. **One amulet worn at a time**; each gives one bonus for as long as it is on.
* Found in rare chests or dropped by a boss.

Bonuses so far:

| Amulet | Bonus | Kind |
|---|---|---|
| Poison | 50% chance to resist a poisoned hit (the poison, not the hit's damage) | chance |
| Traps | less trap damage | reduction |
| Pierce (blunt, slash alike) | 50% off one damage type (dictated as "5%": confirm) | reduction |
| Regeneration | slow HP regeneration | over time |

* **Chance vs reduction:** a chance for effects that are on or off (poison), a % off for damage.
* **Typed instead of "physical":** a cut of all monster damage is stronger than every other amulet. One amulet per
  damage type instead (blunt, slash, pierce). Needs the monsters to deal typed damage:
  [monster-attack-damage-types.draft.md](monster-attack-damage-types.draft.md).

## Open

* Numbers: the share per amulet. Traps can be seen and walked around, so a trap amulet needs a big cut (75%?) or
  immunity to be worth the slot.
* Regeneration vs the potion economy: slow (1 HP every few s), only out of combat, or only up to 50% HP? Does poison
  pause it?
* Swapping: free in the inventory (put on the poison amulet before the scorpion room), or only out of combat?
* Campaign: which amulet in which level / boss; each found once, no duplicates. Count them in `./levelcheck` (does a
  poison amulet count toward the antidote rule?).
* UI: an amulet group or slot in the inventory ([inventory-overhaul.draft.md](inventory-overhaul.draft.md)), the
  worn amulet on the HUD, the details panel text. Save / load of the worn one.
* Models and icons.
* More bonuses: slower sprint drain, a faster escape from the crocodile's hold, more damage of one type.
