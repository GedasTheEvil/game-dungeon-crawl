# Skill: stun

Status: draft 2026-10-10. Ready: [skills](solved/skills.md) is in. From the user;
refined 2026-10-22 (decided by the user).

The skill is on the Heka screen already, sealed: implementing it clears its `sealed` in `SKILLS`
(`src/world/skills.cpp`) and checks its card text there.

## Idea

A new mechanic: with the skill, a hit with a blunt weapon can stun the monster, which stops it moving and attacking
for a short time.

* Blunt weapons only: the weapon's `DamageMix` decides. Default: chance = 25% x the blunt share (club, mace high;
  khopesh none; user, 2026-10-22, not a flat chance on blunt-main weapons).
* Stun: default 1.5 s. The monster stops in place, its clip pauses or plays a stagger; little stars or a daze ring
  over it. A stunned flyer drops to the floor for the stun (low weapons reach it), then takes off
  again (user, 2026-10-22).
* Bosses: half the time, and not again within 5 s (no stun-lock).
* Lurking monsters cannot be stunned until awake.
* The journal shows "Stunned" in the status line the first time; field note with humour.

## Open (warn when reporting)

* Chance and duration need play-testing; the sim (`monster-balance`) should run with it.

Decided: a stunned monster takes normal damage, no bonus (user, 2026-10-22).

## Tests

* Unit / sim: a blunt hit stuns at the chance (seeded), the monster does not move or attack while stunned; a slash
  weapon never stuns; boss stun is shorter and has a cooldown.
