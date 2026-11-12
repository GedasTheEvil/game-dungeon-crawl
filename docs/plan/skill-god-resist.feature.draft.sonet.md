# Serket's favour and Blessing of Isis: 10% to resist poison or a curse

Status: draft 2026-11-12, size S, needs nothing first.

## Idea

From the user. Both god skills get one more bonus, always on once learnt:

* Serket's favour: 10% chance that a poison does not land.
* Blessing of Isis: 10% chance that a curse does not land.

On top of what they already do (half as long, the longer shrine ward, the cure on learning,
[learn-skill-cures](solved/learn-skill-cures.feature.md)). From any source, like the shrine wards.

## Plan

* Constants next to `CURSE_MS_ISIS` (or the skill code): `SERKET_RESIST_PERCENT = 10`, `ISIS_RESIST_PERCENT = 10`.
* Poison, `Player::Poison` (`src/sim/player.cpp`): after the Shelter of Serket check, before `TakePoison`, with the
  skill a separate `rng.percent(SERKET_RESIST_PERCENT)` roll; on a hit: good status line "Serket's favour spares you
  the venom", return. Separate roll (not added to `PoisonResistPercent`) so the line names the skill and the amulet /
  potion lines stay right.
* Curse, `PlayerStats::LayCurse` (`src/level/player_stats.cpp`): after the `IsisWarded` check, with the skill a
  `ISIS_RESIST_PERCENT` roll; on a hit: good status line "Isis's blessing turns the curse away", return false
  (no `run.cursed++`, no field note). `LayCurse` has no `Rng` yet: add an `Rng&` parameter, pass the sim's rng from
  the callers (`src/sim/player.cpp`, `src/sim/dungeon_chest_traps.cpp`, `src/test/scenario.cpp`; the scenario
  `curse` command must still lay it every time: give it a ward-free path or a seed, implementer's choice, but
  `curse` in play scripts and scenarios must stay deterministic).
* Heka screen texts (`src/level/skills.cpp`): "Poisons last half as long, 10% resisted", "Curses last half as long,
  10% resisted" (fit the card; shorten if it does not).
* Field notes of both skills (`src/level/journal.cpp`): add the 10%.
* Glossary, the "skill names" row: Serket's favour ... and 10% of poisons don't land; Blessing of Isis ... and 10% of
  curses don't land.
* Unit test (`tests/unit/`, name the plan): with the skill and a fixed rng, over many tries about 10% resisted
  (or a seed known to roll a resist and one known not to); without the skill never resisted. Both poison and curse.

## Play-test

* `bin/game --play tests/play/skill-god-resist.txt` (setup: god, both skills learnt, a room with poisoning and
  cursing monsters).
* Get hit many times: now and then "Serket's favour spares you the venom" and no poison card.
* Same for curses: now and then "Isis's blessing turns the curse away" and no curse card.
