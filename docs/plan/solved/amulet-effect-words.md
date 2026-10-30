# Amulet effect words: guard vs. power

Status: draft 2026-10-30 (user), needs nothing first.

## Why

The guard amulets (they cut the damage the player takes) and the damage amulets (they raise the damage the player
deals) read almost the same:

| amulet | name band now | effect line now |
|---|---|---|
| Shen Ring (stab guard) | `Pierce II` | `Stab damage -16%` |
| Neith's Arrowhead (piercing) | `Pierce II` | `Stab damage +20%` |
| Djed Pillar (smash guard) | `Blunt II` | `Smash damage -16%` |
| Seth's Mace (crushing) | `Crush II` | `Smash damage +20%` |
| Knot of Isis (cut guard) | `Slash II` | `Cut damage -16%` |
| Montu's Khopesh (cleaving) | `Cleave II` | `Cut damage +20%` |
| Sun Disc (fire guard) | `Fire II` | `Fire damage -40%` |

Whose damage: the line does not say. The stab guard and the piercing amulet even share the name band (`Pierce`).
The bands still use the hard words [simple-damage-words](simple-damage-words.md) dropped (blunt, slash,
pierce, cleave, crush).

## Change

Only text the player reads (`AMULET_TYPES`, `src/world/items.cpp`: `shortNoun` and `effect`). A guard amulet
says "guard", a damage amulet says "power". Not "armor" (the user's first idea): the Armor stat is flat and does
not stop fire, a guard is a percentage and does; the same word would mix them up. Not "protection": longer, and a
harder word for the children. "Guard" is already in the amulets' labels ("amulet of stab guard").

| amulet | name band | effect line |
|---|---|---|
| smash / cut / stab guard | `Smash Guard II` | `Smash guard: you take 16% less` |
| fire guard | `Fire Guard II` | `Fire guard: you take 40% less` |
| crushing / cleaving / piercing | `Smash Power II` | `Smash power: you deal 20% more` |

* "Power" for the damage amulets: plain, and not "Strength" (the Might amulet's word).
* Too wide for the slot's name band (`inventory.cpp`, `NAME_BAND_H` band) or the card's effect line: shorten to
  `Stab Grd. II` / `Stab Pow. II`, or the effect to `Stab hits on you -16%` / `Your stab hits +20%`. Check with a
  scenario screenshot of the inventory (the longest: `Smash Guard IV`, `Smash Power IV`, grand tier).
* `labelNoun` ("amulet of stab guard", for the editor, tools and scenario commands) stays: not player text.
* Trap, poison, armor, health, strength, regeneration, venom: no change.

## Tests

* `tests/unit/amulet_extras_test.cpp` checks the old effect lines: update them.
* Add a check that no two amulet types share a `shortNoun`.
* Grep `tests/` and `content/texts/` for the old lines (`Stab damage`, `Smash damage`, `Cut damage`,
  `Fire damage -`).
