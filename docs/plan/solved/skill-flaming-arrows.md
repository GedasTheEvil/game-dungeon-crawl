# Skill: flaming arrows

Status: implemented 2026-10-19, play-tested and solved 2026-10-19 (`bin/game --play tests/play/skill-flaming-arrows.txt`). Was: draft 2026-10-14. Needs first: [fire-damage](fire-damage.md) (fire type, burning, the brazier fire
arrows). Sibling of [skill-power-hit](../skill-power-hit.draft.opus.md): either can go in first. From the user.

## Idea

A Heka skill (`SkillId`, `SKILLS` in `src/world/skills.cpp`, a new card, not sealed). With it, bow shots can fly
burning for stamina.

* Bow only (arrows); sling, javelin and throwing stick stay as they are.
* A shot with the skill on deals **+40% bonus damage, as fire**: the arrow's own hit stays, a fire share of 40% of it
  comes on top. The fire share goes through fire resistances (a mummy takes double) and ignites the target
  (burning, per fire-damage).
* Cost: 10% of max stamina per shot.
* Purpose (user): the answer to monsters an arrow barely scratches. **Whoever resists pierce is weak to fire**, as a
  rule with exceptions.
  Today (`src/world/monster_kinds.cpp`, fire column from fire-damage) plant, mummy and egg cluster already are; this
  plan makes bat (thin wing membranes) and mimic (dry wood) weak to fire too, with the reason in the comment. The
  scorpion stays normal (decided, user 2026-10-14): the exception, its comment says why (e.g. a desert hunter, used
  to heat). Their bosses follow through
  `bossResist`. Keep the rule in the resistance table's comment, and in the journal's field note on fire arrows.
* **Toggle**, as power hit (stamina also pays for sprint and jump). One key for both skills, default `Q`: on = every
  learned stamina skill that fits the attack applies (a bow shot: flaming arrows, and power hit if it covers ranged;
  then both bonuses, both costs). Whichever of the two plans goes in first brings the key and its HUD sign.
* Too little stamina: a normal arrow, no cost, the HUD toggle blinks once (as power hit).
* Look: reuse the brazier fire arrow's flight (the projectile `fire` flag, flame style `ARROW`, its light).
* A brazier-lit arrow (`fireArrows`) already burns: the skill neither adds to it nor costs stamina for it.

## Also

* Card text in `SKILLS` (one-line effect plus a flavour line; e.g. Ra or Sekhmet lends the flame).
* 9 skills: the Heka screen gets a fifth card row (`ROWS` in `src/ui/skills_layout.h`); check it still fits with a
  screenshot.
* Glossary row: flaming arrows.

## Open (warn when reporting)

* Numbers (40%, 10% stamina) need the sim (`monster-balance`).

## Tests

* Unit: every kind that resists pierce (resists or tough) is weak to fire, except a named list (the scorpion and
  its boss): a table check, so a new monster keeps the rule or joins the list on purpose.
* Unit: with the skill on, a bow hit deals +40% as fire and costs 10% stamina; a mummy takes the fire share double;
  the target burns; low stamina = normal arrow, no cost; toggle off = normal; a brazier-lit arrow costs nothing.
* Scenario with a screenshot: the Heka screen with 9 cards.

## Implemented

* Q is the shared stamina-skill key (`BindAction::StaminaSkills`, `Player::staminaSkills`, not saved). With no stamina
  skill learnt it only says so. Power hit brings its own bonus and cost to the same toggle when it lands.
* The fire share is computed in `Dungeon::Shoot`: damage x 1.4 and a mix of 71% (rest, scaled) and 29% fire, so the fire
  is 40% of the old hit up to rounding.
* The water lotus joins the scorpions in the table test's exception list (its fire is `RESISTS`: a wet stem), the
  draft only named the scorpions. Bosses keep a normal fire rate (`bossResist` caps weak at normal), the table test
  checks that.
* The Heka cards are 11 high now (five rows fit); the Controls tab got 12 rows a column for the new key.
