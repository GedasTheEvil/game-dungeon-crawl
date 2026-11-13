# Serket's favour and Blessing of Isis: 10% to resist poison or a curse

Status: solved 2026-11-13 (play-tested by the user). Was: implemented 2026-11-12, waiting for the user's play-test. Was: draft 2026-11-12, size S, needs nothing first.

## Idea

From the user. Both god skills get one more bonus, always on once learnt:

* Serket's favour: 10% chance that a poison does not land.
* Blessing of Isis: 10% chance that a curse does not land.

On top of what they already do (half as long, the longer shrine ward, the cure on learning,
[learn-skill-cures](learn-skill-cures.feature.md)). From any source, like the shrine wards.

## Plan

* Constants next to `CURSE_MS_ISIS` (or the skill code): `SERKET_RESIST_PERCENT = 10`, `ISIS_RESIST_PERCENT = 10`.
* Stacking (user): the skill's 10% adds to the resist chance items and blessings give, 100% the upper limit, one
  roll over the sum.
* Poison: `PlayerStats::PoisonResistPercent` (`src/level/player_stats.cpp`) becomes amulet + resistance potion +
  `SERKET_RESIST_PERCENT` with the skill, `std::min(100, ...)` as today. `Player::Poison` (`src/sim/player.cpp`)
  keeps its one `TakePoison` roll. The resisted line names a source: the potion running → "You resist the poison"
  (as today), else an amulet with a poison ward → "Your amulet wards off the poison" (as today), else (only the
  skill) "Serket's favour spares you the venom". The Shelter of Serket stays a full block before it (already 100%).
  The potion's toast that shows `PoisonResistPercent()` then includes the skill's 10%: fine, it is the real chance.
* Curse: a new `PlayerStats::CurseResistPercent()` = `ISIS_RESIST_PERCENT` with the skill, plus any item or
  blessing curse-resist chance (today none: the Protection of Isis is a full block, already 100%; the trap ward
  stops a chest trap or tomb curse as a whole, before the curse, and stays a roll of its own), `std::min(100, ...)`,
  so a later source stacks. `PlayerStats::LayCurse`: after the `IsisWarded` check, roll `CurseResistPercent()`
  (only when > 0); on a hit: good status line "Isis's blessing turns the curse away", return false (no
  `run.cursed++`, no field note). `LayCurse` has no `Rng` yet: add an `Rng&` parameter, pass the sim's rng from the
  callers (`src/sim/player.cpp`, `src/sim/dungeon_chest_traps.cpp`, `src/test/scenario.cpp`; the scenario `curse`
  command must still lay it every time: give it a resist-free path or a seed, implementer's choice, but `curse` in
  play scripts and scenarios must stay deterministic).
* Heka screen texts (`src/level/skills.cpp`): "Poisons last half as long, 10% resisted", "Curses last half as long,
  10% resisted" (fit the card; shorten if it does not).
* Field notes of both skills (`src/level/journal.cpp`): add the 10%.
* Glossary, the "skill names" row: Serket's favour ... and 10% of poisons don't land; Blessing of Isis ... and 10% of
  curses don't land.
* Unit test (`tests/unit/`, name the plan): with the skill and a fixed rng, over many tries about 10% resisted
  (or a seed known to roll a resist and one known not to); without the skill never resisted. Both poison and curse.
  Stacking: `PoisonResistPercent()` = amulet + potion + 10 with the skill, capped at 100 (a 95% amulet + skill
  → 100).

## Play-test

* `bin/game --play tests/play/skill-god-resist.txt` (god mode, both skills learnt, a hall with scorpions and a cobra
  that poison and mummies and a giant bat that curse; the script's level is `tests/levels/skill_god_resist`).
* Get hit many times: now and then "Serket's favour spares you the venom" and no poison card.
* Same for curses: now and then "Isis's blessing turns the curse away" and no curse card.
* The Heka cards read "..., 10% resisted"; the field notes of both skills tell the 10%.
