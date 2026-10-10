# Cursed mimic

Status: draft 2026-10-10, refined 2026-10-10 (decided, not implemented). From the user. Needs [curses](curses.draft.md).

## Idea

A variant of the mimic for the higher levels. Its bite can curse the player with **insanity** (map, journal,
inventory and quick drink locked, club in hand; see the curses draft). It is the only source of insanity for now,
the harshest curse.

## Sketch

* New `MonsterCursedMimic` after `MonsterSobek`, built like the giant kin (giant cobra, giant rat): the mimic's model
  and clips (`monsters/mimic`), its own texture, a bit bigger, stronger.
* Look: the chest has to give a hint for an attentive player, e.g. dark wood, tarnished gold, faint purple glow in
  the keyhole or seams. Still has to read as a chest from afar.
* Behaviour: a chest (ambush) until the player comes near, like the mimic. Unlike the mimic it then jumps: it hops
  after the player and leaps pits and traps, as the walk-jumpers do (giant rat, giant scarab, `Leap`). Needs an
  ambush-then-jump locomotion (or `Ambush` with a jump flag) and hop clips on the mimic model (lid snapping as it
  lands). Its speed is a starting value: about the giant rat's.
* Curse: insanity, 25% per landed bite. Insanity has one grade, so no grade roll.
* Loot: like the mimic (`mimicLoot`), maybe one grade better since it is rarer and riskier.
* Journal note, e.g. "Some chests bite. Some bite the mind."
* Boss rule: no boss has `kin = MonsterMimic`, so none needed.
* Placement: hand-placed in levels 21-30 (as the giant cobra), next to plain mimics (both appear). `level_gen.cpp`
  swaps a share of its generated mimics for cursed ones from a higher difficulty on (own roll on `mimicRng`).
* Glyph: `x` (free; `m` is the unknown monster).
* `levelcheck`: threat 4. Insanity locks the map, which the checker does not model. A jumping mimic can now reach
  the player across pits; the checker's threat and reach rules for walk-jumpers apply.
* Glossary row: cursed mimic.
* Tests: unit (kind numbers, curse roll), a scenario (wakes, bites, insanity icon, map key refused).

## Numbers (starting values)

| | Mimic | Cursed mimic |
|---|---|---|
| HP | 40 | 120 |
| Bite | 10 / 800 ms | 20 / 900 ms |
| XP | 1500 | 3500 |
| Scale | 8 | 10 |
| Speed | 0 | about the giant rat's |
| Threat | 2 | 4 |

## Decided (2026-10-10)

* Insanity chance 25% per landed bite.
* It jumps (see Behaviour).
* Plain and cursed mimics both appear on levels 21-30.
* Glyph `x`.

## Open (for the implementer)

* Hop clips and how the chest moves (hops only, no walk cycle), the texture look.
* The share of generated mimics that are cursed, and from which difficulty.
