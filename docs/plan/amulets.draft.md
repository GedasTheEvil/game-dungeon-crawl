# Amulets

Status: draft 2026-10-06. Split off [resistance-potion.draft.md](resistance-potion.draft.md).

## Idea

* A new item kind. **One amulet worn at a time**; each gives one bonus for as long as it is on.
* Found in rare chests or dropped by a boss.

### Tiers (decided 2026-10-07)

Most amulets come in four tiers, weakest first: **lesser, minor, normal, grand**. All tiers of one amulet share one
model and differ only in the bonus (and the name). Example: the amulet of strength is an animal tooth (wolf, jackal or
similar) on a string. Names: "Lesser amulet of strength", "Amulet of minor strength", "Amulet of strength",
"Grand amulet of strength".

| Amulet | Lesser | Minor | Normal | Grand | Notes |
|---|---|---|---|---|---|
| Strength | +1 | +2 | +4 | +6 | adds to might (`PlayerStats::Might`, added to every hit) |
| Armour | tiered like strength | | | | adds to armour (`PlayerStats::Armor`); numbers open |
| Health | +5% | +10% | +20% | +30% | more max HP |
| Regeneration | - | - | 1 HP/s | 2 HP/s | normal and grand tiers only; out of combat only |
| Poison | | | | | a chance to resist a poisoned hit (the poison, not the hit's damage); tiers open |
| Traps | | | | | less trap damage; tiers open |
| Pierce (blunt, slash alike) | | | | | a share off one damage type; tiers open |

* **Regeneration:** the simplest rule: a flat number of HP per second, only out of combat. No cap, no % of max HP.
* **Chance vs reduction:** a chance for effects that are on or off (poison), a % off for damage.
* **Typed instead of "physical":** a cut of all monster damage is stronger than every other amulet. One amulet per
  damage type instead (blunt, slash, pierce). Needs the monsters to deal typed damage:
  [monster-attack-damage-types.md](solved/monster-attack-damage-types.md).

## Open

* Tier numbers for armour, poison, traps and the damage types. Traps can be seen and walked around, so a trap amulet
  needs a big cut or immunity to be worth the slot.
* "Out of combat" for regeneration: how long after the last hit given or taken? Does poison pause it?
* Health amulet: on taking it off, is the HP above the new max cut off?
* Which tiers show up where (lesser early, grand late or from bosses); models for the other amulets.
* Swapping: free in the inventory (put on the poison amulet before the scorpion room), or only out of combat?
* Campaign: which amulet in which level / boss; each found once, no duplicates. Count them in `./levelcheck` (does a
  poison amulet count toward the antidote rule?).
* UI: an amulet group or slot in the inventory ([inventory-overhaul.md](solved/inventory-overhaul.md)), the
  worn amulet on the HUD, the details panel text. Save / load of the worn one.
* Models and icons.
* More bonuses: slower sprint drain, a faster escape from the crocodile's hold, more damage of one type.
