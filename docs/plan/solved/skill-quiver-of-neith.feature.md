# Skill: Quiver of Neith

Status: solved 2026-11-05 (play-tested by the user). Was: implemented 2026-11-05, waiting for the user's play-test. Was: draft 2026-11-05, size M, needs first [heka-surplus](heka-surplus.feature.md) (the scrolling Heka list, the icon atlas, `SkillId` order). From the user.

## Idea

A Heka skill for the ranged style: every ranged weapon holds more shots, and spent shots come back 25% faster.
Neith, the huntress, whose sign is two crossed arrows. The amulets do the same job one at a time (Satet's Quiver for
the count, Bastet's Cat for the pace, [amulet-more-shots](amulet-more-shots.md),
[amulet-quick-shots](amulet-quick-shots.md)); the skill does both, smaller than the grand amulets, and stacks
with them.

* Name: "Quiver of Neith". Card: "Quiver of Neith: ranged weapons hold 40% more shots, and they come back 25% faster."
* Lore (`content/texts/skills.txt`, `[skill.quiver-of-neith]`): "Neith, the huntress of the crossed arrows, refills
  your quiver before you reach for it."
* Journal note on learning (as Sharp eye's `FieldNote`), humour welcome ("Neith counts my arrows. She counts better
  than I do.").

## Plan

* `SkillId::QuiverOfNeith` appended after the heka-surplus skills, `SKILL_COUNT` +1, row in `SKILLS`
  (`src/world/skills.cpp`), not sealed. Constants in `skills.h`: `QUIVER_SHOTS_PERCENT = 40`,
  `QUIVER_RECOVERY_PERCENT = 25`.
* Shots: `PlayerStats::Shots` (`src/entities/player_stats.cpp`) adds the skill's percent to `amulet.shotsPercent`
  (one sum, one rounding up, at least one). Self-bow 5 → 7, composite bow and sling 6 → 9, throwing stick 3 → 5,
  javelin 2 → 3. With a grand quiver amulet (100%): self-bow 5 → 12.
* Pace: `PlayerStats::ShotRecoveryPace` adds `QUIVER_RECOVERY_PERCENT` to `amulet.shotRecoveryPercent`: the wait
  after the last shot and each shot's return, 2.5 s → 2 s. With a grand cat: speed 225%.
* `PlayerStats` needs to know the skill: pass it the same way the heka-surplus stat skills reach it (a skill is a
  state, recomputed on load / reset, never granted once).
* Learning it mid-fight: the shots already spent stay spent; the new max shows on the HUD pips at once.
* Icon: a quiver with two crossed arrows in `tools/textures/skill_icons.py`, regenerate `skill_icons.png`.
* Glossary row "quiver of neith" (names `SkillId::QuiverOfNeith`, the constants); update the "shots" row.

## Tests

* Unit (`tests/unit/skills_test.cpp`): `Shots` per ranged weapon with and without the skill, with the skill plus a
  quiver amulet; `ShotRecoveryPace` with the skill, with the skill plus a cat amulet; melee weapons stay at 0 shots.

## Play-test

* `bin/game --play tests/play/skill-quiver-of-neith.txt`: the archery level, a self-bow and a javelin given, the skill learnt.
* Shoot the bow empty: 7 shots, not 5. The javelin throws 3.
* After the last shot the first comes back after about 2 s, then one every 2 s.
* The Heka screen shows the card with its icon and lore.
