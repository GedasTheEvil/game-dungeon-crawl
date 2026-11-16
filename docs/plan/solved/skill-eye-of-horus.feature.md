# Skill: Eye of Horus (Sharp eye renamed, the minimum range shrinks)

Status: implemented 2026-11-16, waiting for the user's play-test. Was: draft 2026-11-16, size S, needs nothing first.

## Idea

The user: the Sharp eye skill becomes "Eye of Horus". Next to its range bonus (ranged reach 130%,
`SHARP_EYE_RANGE_PERCENT`) it takes the minimum range of ranged weapons away
([ranged-weaker-than-melee](solved/ranged-weaker-than-melee.md)):

* Rank I (the skill learnt): the minimum range shrinks to the player's hitbox. A shot is refused as too close
  (`ShotRefusal::TooClose`) only while the player's and the monster's hitboxes intersect.
* Rank II (a second heka, [skill-upgrades](skill-upgrades.feature.md)): no minimum range at all, a shot is
  never refused as too close.

## Plan

* Rename the skill everywhere the player reads it: card name in `src/level/skills.cpp` (`SkillId::SharpEye` may stay
  or become `EyeOfHorus`, implementer's call), card text ("Ranged weapons reach 30% farther; shoot up close"),
  journal field note title and text (`content/texts/journal.txt`, `FieldNote::SharpEye`), glossary row, Where is X rows.
* Name clash: the trap-ward amulet is already named "Eye of Horus" (`AmuletType::TrapWard`, item texts, journal
  lines 213, 306, 492). Rename the amulet (e.g. "Wedjat" or "Eye of Ra"); the skill keeps "Eye of Horus".
* Rank I: in `Dungeon::ShotRefused` (`src/sim/dungeon_arrows.cpp`) with the skill, the too-close test is "the
  monster's hitbox overlaps the player's" instead of `weaponMinRange`. A helper next to `weaponMinRange`
  (`src/level/items.h`) that takes the `Heka`, as `weaponReach` does, keeps the rule in one place.
* Rank II: if skill-upgrades is implemented first, its Sharp eye row gets "no minimum range" on top of its reach; if
  this draft lands first, skill-upgrades adds rank II here (the skill-upgrades implementer reads this draft).
* The headless sim's kite (backs off to the bow's minimum range, [monster-balance](solved/monster-balance.md))
  reads the same helper, so a sim player with the skill stops backing off.
* Unit tests: refusal with and without the skill, hitboxes touching vs. a gap of 0.1 tile, rank II never refuses.
* Scenario: a monster next to the player (gap below the bow's minimum range, no overlap): refused without the skill,
  shoots with it.

## Implementation notes

* `SkillId::EyeOfHorus`; the amulet is now the "Wedjat" (its riddle already took "wedjat" as an answer).
* `TooCloseRule` / `tooCloseRule(heka)` and `weaponMinRange(weapon, heka)` (`level/items.h`): rank I measures from the
  player's front edge (`Player::HalfWidth`) to the monster's near edge, too close below 0 (an overlap; touching is
  not); rank II never refuses. The balance sim's kite reads `weaponMinRange(weapon, heka)`.
* Rank II: done with [skill-upgrades](skill-upgrades.feature.md) (160%, no minimum range).

## Play-test

* `bin/game --play tests/play/skill-eye-of-horus.txt`. Walk right up to the plant (col 13) and shoot: "Too close to
  shoot". Learn Eye of Horus (K): from the same spot the bow fires.
* Walk into the plant: "too close" again. Spend the second heka on Rank II: the bow fires even then.
* The card and journal say "Eye of Horus"; the trap-ward amulet is now the "Wedjat".
