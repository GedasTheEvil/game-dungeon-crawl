# Skill: regeneration (rank I 1 HP / 5 s, rank II 0.25% max HP / s)

Status: draft 2026-11-24, size S, needs nothing first.

## Idea

A new Heka skill that regenerates HP. Safe rule as every regeneration (the user, plan
[regen-safe-rule](regen-safe-rule.feature.draft.sonet.md)): no HP lost in the last 5 s, not burning, poisoned or
bleeding; a monster chasing does not stop it.
Rank I: 1 HP every 5 s. Rank II: 0.25% of max HP every second (at 100 max HP: 1 HP / 4 s; grows with max HP).
Stacks with the regeneration amulet and the garnet rings: every source adds, none replaces another.

## Plan

* New `SkillId` (append at the end of the enum, `SKILL_COUNT` +1), upgradable (`rankTwo`). Name: implementer's
  choice, an Egyptian healing deity, e.g. "Touch of Sekhmet"; check `level/skills.cpp` and the glossary for clashes.
  Hint line, lore text in `content/texts/` like the other skills, field note on learning if the others have one.
* Constants in `level/skills.h`: `REGEN_SKILL_MS_PER_HP = 5000` (rank I), `REGEN_SKILL_PERMILLE_2 = 2.5` as an integer
  rate (e.g. per-mille-per-10 s or HP·ms carry; avoid floats in the sim if the code base avoids them).
* `PlayerStats::Regenerate` (`level/player_stats.cpp`): a third carry beside `regen_carry_ms` and
  `ring_regen_carry`, own field (`skill_regen_carry`), the `healing` gate every source uses (no poison, no bleeding, not
  `RegenBlocked` (burning, 5 s after a loss), alive, below max); no `PlayerSafe` gate (if regen-safe-rule is not done
  yet, still don't gate the skill on it). Rank II replaces rank I's rate (it
  does not add rank I on top). Reset the carry where the others reset (`player_stats.cpp:267`, `WearRings` does not
  touch it).
* Stacking: the amulet's, the rings' and the skill's HP add up in the same tick; no cap other than max HP.
* Unit test (`tests/unit/`): rank I gives 1 HP after 5 s, 0 for 5 s after a loss or while poisoned; rank II at 400 max HP
  gives 1 HP a second; with the amulet worn the gains add.
* Glossary row, Where is X row (skills' effects beyond a flag), journal/Heka screen text if the screen lists effects.

## Play-test

* `tests/play/skill-regeneration.txt`: `god` off, give the heka, learn the skill, take damage (e.g. a trap or
  `hp` command if any), a slow monster near.
* Rank I: HP climbs about 1 every 5 s, also while chased; a hit stops it for 5 s.
* Rank II: faster (about 1 HP / 4 s at 100 max HP, more with a higher max HP).
* With the regeneration amulet worn: faster than either alone.
