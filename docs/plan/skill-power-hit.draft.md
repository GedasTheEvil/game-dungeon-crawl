# Skill: power hit

Status: draft 2026-10-10. Needs [skills](skills.draft.md) first. From the user; not fully refined.

## Idea

With the skill, attacks deal more damage but cost stamina. With too little stamina, the attack is a normal one.

* Default: +50% damage, costs 15% of max stamina per swing or shot. Melee and ranged alike (implementer's choice:
  melee only).
* Stamina also pays for sprint and jump (`PlayerStats`): an always-on power hit would starve them. Default: a
  **toggle key** (default `Q`), shown on the HUD; off = today's attacks. Alternative (implementer's choice): hold the
  attack button to wind up a power hit.
* Feedback: a heavier swing sound, a bigger hit spark, a slight camera shake on hit.
* Too little stamina: a normal attack, no cost, and the HUD toggle blinks once.

## Open (warn when reporting)

* Toggle vs hold vs always-on (user said "attacks deal more damage", which may mean always-on).
* Numbers need the sim (`monster-balance`).

## Tests

* Unit / sim: power hit deals +50% and costs stamina; with low stamina a normal hit, no cost; toggle off = normal.
