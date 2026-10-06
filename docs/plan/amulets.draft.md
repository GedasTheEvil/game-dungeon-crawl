# Amulets

Status: draft 2026-10-06. Split off [resistance-potion.draft.md](resistance-potion.draft.md).

## Idea

* A new item kind. **One amulet worn at a time**; each gives one bonus for as long as it is on.
* Found in rare chests or dropped by a boss.

Bonuses so far:

| Amulet | Bonus | Kind |
|---|---|---|
| Poison | a chance to resist a poisoned hit (the poison, not the hit's damage) | chance |
| Traps | less trap damage | reduction |
| Pierce (blunt, slash alike) | a share off one damage type | reduction |
| Regeneration | slow HP regeneration | over time |

* **Chance vs reduction:** a chance for effects that are on or off (poison), a % off for damage.
* **Typed instead of "physical":** a cut of all monster damage is stronger than every other amulet. One amulet per
  damage type instead (blunt, slash, pierce). Needs the monsters to deal typed damage:
  [monster-attack-damage-types.md](solved/monster-attack-damage-types.md).

## Open

* Numbers: left for refinement (not now). Traps can be seen and walked around, so a trap amulet needs a big cut or
  immunity to be worth the slot.
* Regeneration vs the potion economy: slow, only out of combat, or only up to part of the HP? Does poison
  pause it?
* Swapping: free in the inventory (put on the poison amulet before the scorpion room), or only out of combat?
* Campaign: which amulet in which level / boss; each found once, no duplicates. Count them in `./levelcheck` (does a
  poison amulet count toward the antidote rule?).
* UI: an amulet group or slot in the inventory ([inventory-overhaul.md](solved/inventory-overhaul.md)), the
  worn amulet on the HUD, the details panel text. Save / load of the worn one.
* Models and icons.
* More bonuses: slower sprint drain, a faster escape from the crocodile's hold, more damage of one type.
