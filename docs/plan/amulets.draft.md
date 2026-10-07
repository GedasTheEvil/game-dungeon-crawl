# Amulets

Status: draft 2026-10-06. Split off [resistance-potion.draft.md](resistance-potion.draft.md).

## Idea

* A new item kind. **One amulet worn at a time**; each gives one bonus for as long as it is on.
* Where found: lesser and minor from chests (a random chance), normal and grand only from bosses (random amulet).
* Duplicates allowed: the player can have more than one amulet of the same type and tier.
* Later: [amulet upgrades](amulet-upgrades.draft.md) (combine or boost), [amulet of venom](venom-amulet.draft.md)
  (poison monsters on hit).

### UI (decided 2026-10-07)

* Its own Amulets tab in the inventory ([inventory-overhaul.md](solved/inventory-overhaul.md)), the details panel
  text per amulet.
* The worn amulet's icon on the HUD.
* The worn amulet and the carried ones are saved with the game.

### Tiers (decided 2026-10-07)

Most amulets come in four tiers, weakest first: **lesser, minor, normal, grand**. All tiers of one amulet share one
model and differ only in the bonus (and the name). Example: the amulet of strength is an animal tooth (wolf, jackal or
similar) on a string. Names: "Lesser amulet of strength", "Amulet of minor strength", "Amulet of strength",
"Grand amulet of strength".

| Amulet | Lesser | Minor | Normal | Grand | Notes |
|---|---|---|---|---|---|
| Strength | +1 | +2 | +4 | +6 | adds to might (`PlayerStats::Might`, added to every hit) |
| Armour | +1 | +2 | +4 | +6 | adds to armour (`PlayerStats::Armor`), same numbers as strength |
| Health | +5% | +10% | +20% | +30% | more max HP; HP keeps its share of the max when put on or taken off |
| Regeneration | - | - | 1 HP/s | 2 HP/s | normal and grand tiers only; out of combat only |
| Poison | 10% | 25% | 50% | 80% | the chance to resist a poisoned hit (the poison, not the hit's damage) |
| Traps | 25% | 50% | 75% | immune | less trap damage |
| Pierce (blunt, slash alike) | 8% | 16% | 28% | 40% | less damage of one type |

* **Health amulet:** putting it on or taking it off changes the max HP but keeps the HP at the same share of it: no
  heal, no loss (full stays full, half stays half). Unlike a max HP potion (`PlayerStats::AddMaxHP` heals fully).
* **Regeneration:** the simplest rule: a flat number of HP per second, only out of combat. No cap, no % of max HP.
  **Out of combat = no monster is chasing you:** as soon as a monster has noticed you and moves towards you,
  regeneration stops (the fear of the monster). It starts again when none is chasing. A monster that cannot chase or
  attack you (rooted, stuck, lost you, out of reach) does not count: you are safe. Poisoned: no regeneration.
  Code note: `Monster::alerted` stays set once a monster has acted (it drives the health bar), so "chasing now" needs
  its own per-tick state.
* **Chance vs reduction:** a chance for effects that are on or off (poison), a % off for damage.
* **Typed instead of "physical":** a cut of all monster damage is stronger than every other amulet. One amulet per
  damage type instead (blunt, slash, pierce). Needs the monsters to deal typed damage:
  [monster-attack-damage-types.md](solved/monster-attack-damage-types.md).

## Open

* Chest chance per tier and depth (minor deeper than lesser?); does every boss drop one, or a chance? Which amulet
  types can drop where.
* Swapping: free in the inventory (put on the poison amulet before the scorpion room), or only out of combat?
* `./levelcheck`: count the amulets a level can give? Does a poison amulet count toward the antidote rule (it is only a
  chance)?
* Models and icons: one model per amulet type (strength: an animal tooth on a string), tiers share it; the others open.
* More bonuses: slower sprint drain, a faster escape from the crocodile's hold, more damage of one type.
