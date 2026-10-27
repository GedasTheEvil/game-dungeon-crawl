# Skill: stun

Status: implemented 2026-10-27 ([Implemented](#implemented)), waiting for the user's play-test:
`bin/game --play tests/play/skill-stun.txt`. Was: draft 2026-10-10, from the user; refined 2026-10-22 (decided by the
user); needed [skills](solved/skills.md).

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

## Implemented

* `Monster::StunHit` / `Reeling`: the monster stands (`UpdateMonsters` skips it), its clip paused (`Animate`), no step,
  no bite, no shot; a bite's windup or a grab under way ends (a held player is free). A flyer falls as a dead one to
  its row's floor (`Fly`), its lift then eases back up as usual. A stun again on a stunned (non-boss) monster
  extends it to 1.5 s from the new hit.
* The roll (`Dungeon::stunRoll`, gameplay stream) is after every weapon hit that did not kill, melee and missiles alike:
  the sling stone (all blunt) and the throwing stick stun too, the draft only said "a blunt weapon". No toggle, no cost.
* Not stunned: a lurker (also not by the hit that wakes it), one rising, emerging, hidden or burrowing, in a leap, a
  charge (its own wall stun included) or a ball roll, or climbing. The draft only named lurkers; the others would
  break their move's state.
* The first stun shows "<name> is stunned"; the field note `Stun` comes after the amulets' (notes are saved by number).
* Look: three small stars circling the head (`drawStunStars`); the clip pauses (no stagger clip).
* Scenario `stun` (the boss in play, else the nearest monster); `tests/scenarios/skill-stun.txt` shows the stars and the
  bat on the floor.
* Bosses: no kin rule applies (not a monster ability).
