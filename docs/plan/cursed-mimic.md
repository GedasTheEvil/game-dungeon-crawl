# Cursed mimic

Status: implemented 2026-10-20, waits for the user's play-test. Draft 2026-10-10, refined 2026-10-10. From the user.

Play-test: `bin/game --play tests/play/cursed-mimic.txt` (a cursed mimic past a death trap: open it, jump the trap
back), or `bin/game --level 22` (also 24, 26, 28, 30).

Names: the game now shows Egyptian names ([egyptian-names](solved/egyptian-names.md)): Anubis guard = Anubis living statue, Anubis boss = Anubis, boss scarab = Khepri, vampire bat = Bat of Shezmu, mimic = Chest of Set, worm = Sand worm, plant = Thorn acacia. Code ids stay.

## Idea

A variant of the mimic for the higher levels. Its bite can curse the player with **insanity** (map, journal,
inventory and quick drink locked, club in hand; see the curses draft). It is the only source of insanity for now,
the harshest curse.

## Sketch

* New `MonsterCursedMimic` after `MonsterSobek`, built like the giant kin (giant cobra, giant rat): the mimic's model
  and clips (`monsters/mimic`), its own texture, a bit bigger, stronger.
* Look: the chest has to give a hint for an attentive player, e.g. dark wood, tarnished gold, faint purple glow in
  the keyhole or seams. Still has to read as a chest from afar.
* Behaviour: a chest (ambush) until woken, like the mimic (on the interact key or a hit, once
  [interact-prompt](solved/interact-prompt.md)). Unlike the mimic it then jumps:
  it hops
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

## Done (2026-10-20)

* `MonsterCursedMimic` = 29 (after the ushabti, the newest kind; the sketch's "after Sobek" predates the later
  kinds), name "Cursed chest of Set", glyph `x`, the numbers of the table, speed 20 (a bit slower than the player's
  walk: they can still walk away; the giant rat's 24 outruns them). Threat 4.
* `Locomotion::AmbushHop`: lurks like `Ambush` (wakes on interact or a hit), then moves like `WalkJump` (leaps pits
  and traps). `ambushes()` / `leaps()` (`monster_kinds.h`) replace the `== Ambush` / `== WalkJump` checks. Not
  rooted once awake (the walker's bite reach). Clips: `AMBUSH_HOP_CLIPS` (`_jump` for the leaps).
* Killed: its spawn tile is slain, its chest drops where it died (`dropChest`). Loot `RollCursedMimicLoot`: half the
  time a lesser or minor amulet (as a chest's), else the mimic's loot.
* Curse: `{Insanity, 25%}`; a unit test checks no other kind lays insanity.
* Levelgen: from difficulty 8, 30% of the generated mimics are cursed (`CURSED_MIMIC_*`, `level_gen.cpp`).
* Campaign: one each in levels 22, 24, 26, 28, 30, beside a treasure, off the path; plain mimics stay in 21, 22, 27,
  28. All levels pass the checker with no warnings.
* Model: `mimic.py --cursed` (`mimic_cursed*.md3`, `mimic_cursed.png`): the hop as its move clip (10 frames: crouch,
  spring with the lid agape, the lid snapping shut as it lands), a 10-frame leap (`_jump`); attack, die and idle as
  the mimic's. Texture recoloured by colour class: near-black wood, tarnished bronze gold, a deep purple lining,
  purple where the chest has lapis blue (panels, inlays, the keyhole plate). From afar a chest; the hint is the
  colour. `render_sheet.py` reads a model's `REVIEW_CLIPS` (`MIMIC_CURSED=1` for the review renders).
* Sounds: `tools/audio/mimic_sounds.py` also writes `mimic_cursed_{wake,att,die,jump}.wav` (the mimic's a fifth
  lower under a whisper, and a leap).
* Journal: its note "Some chests bite. Some bite the mind. ...".
* Tests: `tests/unit/cursed_mimic_test.cpp` (numbers, lurking, hop and leap, insanity, the chest where it died, the
  loot roll, levelgen), scenario `tests/scenarios/cursed_mimic.txt` (wakes, leaps the trap, the insanity icon, the
  map refused).

## Open (for the implementer, decided above)

* Hop clips and how the chest moves (hops only, no walk cycle), the texture look.
* The share of generated mimics that are cursed, and from which difficulty.
