# Skill: stun

Status: draft 2026-10-10. Needs [skills](skills.draft.md) first. From the user; not fully refined.

## Idea

A new mechanic: with the skill, a hit with a blunt weapon can stun the monster, which stops it moving and attacking
for a short time.

* Blunt weapons only: the weapon's `DamageMix` decides. Default: chance = 25% x the blunt share (club, mace high;
  khopesh none). Implementer's choice: a flat chance on blunt-main weapons (`mainType`) instead.
* Stun: default 1.5 s. The monster stops in place, its clip pauses or plays a stagger; little stars or a daze ring
  over it. A stunned flyer drops to the floor or hovers? Default: hovers in place.
* Bosses: half the time, and not again within 5 s (no stun-lock).
* Lurking monsters cannot be stunned until awake.
* The journal shows "Stunned" in the status line the first time; field note with humour.

## Open (warn when reporting)

* Chance and duration need play-testing; the sim (`monster-balance`) should run with it.
* Does a stunned monster take extra damage? Default: no.

## Tests

* Unit / sim: a blunt hit stuns at the chance (seeded), the monster does not move or attack while stunned; a slash
  weapon never stuns; boss stun is shorter and has a cooldown.
