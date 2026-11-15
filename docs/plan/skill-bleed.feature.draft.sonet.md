# Skill Bleed: cutting hits may make a foe bleed

Status: draft 2026-11-15, size S, needs first [bleeding](bleeding.feature.md).

## Idea

A Heka skill like Stun (plan [skill-stun](solved/skill-stun.md)), for cutting weapons: a weapon hit that does not
kill may make the monster bleed.

## Plan

* `SkillId::Bleed`, a row in `SKILLS` (`src/level/skills.cpp`): "Bleed", "Cutting hits may make a foe bleed". Like
  Stun, the screen gives no exact percent.
* Chance: 20% x the weapon's cut (slash) share, the way Stun uses 25% x the blunt share (`STUN_CHANCE_PERCENT`,
  `Dungeon::stunRoll`): a sword (85% cut) 17%, a mace never. Then the bleeding plan's rule: the monster's cut
  resistance scales the chance, bloodless kinds never bleed. A `BLEED_CHANCE_PERCENT` constant.
* Tier: weak. A boss: like Stun, half the chance.
* Ranged hits count by the same rule: they are mostly stab or smash, so they rarely or never cut.
* Stacks with stun, burning and the venom amulet's poison: each rolls on its own.
* Journal / field note row and a glossary row like Stun's (`FieldNote::Stun`, `journal_text.cpp`).
* Scenario `stun` command's twin `bleed` if it helps the test (`scenario_script.cpp`).

## Play-test

* `tests/play/skill-bleed.txt`: the skill learned, a sword, a few monsters. Hit them: some start to bleed (icon over
  the health bar, drops), their HP drops on its own.
* Switch to a mace: no bleeding.
