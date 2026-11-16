# Skill Bleed: cutting hits may make a foe bleed

Status: implemented 2026-11-15, waiting for the user's play-test. Was: draft 2026-11-15, size S, needs nothing first.

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
* Tiers up (play-test round 1): a monster that bled weak before the hit may also bleed medium, at half the weak
  chance; one that bled medium may bleed strong, at half the medium chance (`BLEED_TIER_UP_PERCENT`). Each eligible
  tier rolls on its own, so a hit climbs one tier at most. Bleeding only; poison stays as it is.
* Ranged hits count by the same rule: they are mostly stab or smash, so they rarely or never cut.
* Stacks with stun, burning and the venom amulet's poison: each rolls on its own.
* Journal / field note row and a glossary row like Stun's (`FieldNote::Stun`, `journal_text.cpp`).
* Scenario `stun` command's twin `bleed` if it helps the test (`scenario_script.cpp`).

## Play-test

* `tests/play/skill-bleed.txt`: the skill learned, a sword, a few monsters. Hit them: some start to bleed (icon over
  the health bar, drops), their HP drops on its own. Keep cutting one that bleeds: more drops
  (medium), then more (strong); the Cut Test sword's 1 HP hits keep it alive long enough.
* Switch to a mace: no bleeding.

## Play-test feedback

### Round 1 (2026-11-16)

- [x] the skill gives bleed tiers (bleeding only, not poison). Weak: the current chance, no requirement
- [x] medium: half the weak chance, only if the monster already bleeds weak
- [x] strong: half the medium chance, only if the monster already bleeds medium
