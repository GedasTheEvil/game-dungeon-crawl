# Skill: sharp eye

Status: draft 2026-10-10. Needs [skills](skills.draft.md) first. From the user.

## Idea

Ranged weapons reach farther: +30% range (`WeaponDef::range`) for the bows, the sling, the javelin and the throwing
stick. The bow's aim range (3 tiles today, see `monster-balance`) grows with it.

* Optional (implementer's choice): the aim shows the hit chance or a monster's health bar from farther.
* A longer reach than a cobra's spit is fine: the player who picks it trades a combat skill for a safer style.
* Card text: "Sharp eye: ranged weapons reach 30% farther." Journal note, humour welcome ("Horus lent me an eye. I
  will give it back. Probably.").

## Tests

* Unit: `weaponReach` / ranged range with and without the skill.
