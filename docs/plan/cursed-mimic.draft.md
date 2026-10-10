# Cursed mimic

Status: draft 2026-10-10 (idea, not decided). From the user. Needs [curses](curses.draft.md).

## Idea

A variant of the mimic for the higher levels. Its bite can curse the player with **insanity** (map, journal,
inventory and quick drink locked, club in hand; see the curses draft). It is the only source of insanity for now,
the harshest curse.

## Sketch

* New `MonsterCursedMimic` after `MonsterSobek`, built like the giant kin (giant cobra, giant rat): the mimic's model
  and clips (`monsters/mimic`), its own texture, a bit bigger, stronger.
* Look: the chest has to give a hint for an attentive player, e.g. dark wood, tarnished gold, faint purple glow in
  the keyhole or seams. Still has to read as a chest from afar.
* Behaviour: as the mimic (`Locomotion::Ambush`, rooted, wakes when the player comes near).
* Curse: insanity, chance per landed bite (start at 20%?), then the grade roll from the curses draft (insanity has
  one grade, so no roll).
* Loot: like the mimic (`mimicLoot`), maybe one grade better since it is rarer and riskier.
* Journal note, e.g. "Some chests bite. Some bite the mind."
* Boss rule: no boss has `kin = MonsterMimic`, so none needed.
* Placement: hand-placed in levels 21-30 (as the giant cobra), and `level_gen.cpp` swaps a share of its generated
  mimics for cursed ones from a higher difficulty on (own roll on `mimicRng`).
* `levelcheck`: threat a bit above the mimic's (2). Insanity locks the map, which the checker does not model; no
  change expected.
* Glossary row: cursed mimic.
* Tests: unit (kind numbers, curse roll), a scenario (wakes, bites, insanity icon, map key refused).

## Numbers (starting values)

| | Mimic | Cursed mimic |
|---|---|---|
| HP | 40 | 120 |
| Bite | 10 / 800 ms | 20 / 900 ms |
| XP | 1500 | 3500 |
| Scale | 8 | 10 |
| Threat | 2 | 4 |

## Open

1. Curse chance per bite (20%?), and whether every cursed mimic is placed by hand or also generated.
2. Glyph: `M` is the mimic; pick a free one.
3. Should it also replace plain mimics in 21-30, or both appear?
