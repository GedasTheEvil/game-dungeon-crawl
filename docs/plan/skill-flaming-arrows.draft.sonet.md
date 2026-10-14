# Skill: flaming arrows

Status: draft 2026-10-14. Needs first: [fire-damage](fire-damage.draft.opus.md) (fire type, burning, the brazier fire
arrows). Sibling of [skill-power-hit](skill-power-hit.draft.sonet.md): either can go in first. From the user.

## Idea

A Heka skill (`SkillId`, `SKILLS` in `src/world/skills.cpp`, a new card, not sealed). With it, bow shots can fly
burning for stamina.

* Bow only (arrows); sling, javelin and throwing stick stay as they are.
* A shot with the skill on deals **+20% bonus damage, as fire**: the arrow's own hit stays, a fire share of 20% of it
  comes on top. The fire share goes through fire resistances (a mummy takes double) and ignites the target
  (burning, per fire-damage).
* Cost: 10% of max stamina per shot (cheaper than power hit's 15%: the bonus is smaller).
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

* Numbers (20%, 10% stamina) need the sim (`monster-balance`).

## Tests

* Unit: with the skill on, a bow hit deals +20% as fire and costs 10% stamina; a mummy takes the fire share double;
  the target burns; low stamina = normal arrow, no cost; toggle off = normal; a brazier-lit arrow costs nothing.
* Scenario with a screenshot: the Heka screen with 9 cards.
