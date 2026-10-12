# Skill: sharp eye

Play-test: `./game --play tests/play/skill-sharp-eye.txt`

Status: draft 2026-10-10, implemented 2026-10-12 (see [Implemented](#implemented-2026-10-12)), waiting for play-test. From the user.

The skill is on the Heka screen already, sealed: implementing it clears its `sealed` in `SKILLS`
(`src/world/skills.cpp`) and checks its card text there.

## Idea

Ranged weapons reach farther: +30% range (`WeaponDef::range`) for the bows, the sling, the javelin and the throwing
stick. The bow's aim range (3 tiles today, see `monster-balance`) grows with it.

* Optional (implementer's choice): the aim shows the hit chance or a monster's health bar from farther.
* A longer reach than a cobra's spit is fine: the player who picks it trades a combat skill for a safer style.
* Card text: "Sharp eye: ranged weapons reach 30% farther." Journal note, humour welcome ("Horus lent me an eye. I
  will give it back. Probably.").

## Tests

* Unit: `weaponReach` / ranged range with and without the skill.

## Implemented (2026-10-12)

* `weaponReach(kind, heka)` (`src/world/items.h`): a ranged weapon's aim range times `SHARP_EYE_RANGE_PERCENT` (130)
  with the skill; melee reach unchanged. Used by the shot's aim range (`updateAttack`, `src/state/game_loop.cpp`) and
  the hitbox debug view (`draw.cpp`). Self-bow 3.0 to 3.9 tiles, composite bow 4.0 to 5.2.
* `sealed` cleared; card text unchanged, the lore line now names the throwing stick too.
* Journal note: new `FieldNote::SharpEye` (appended, `FIELD_NOTE_COUNT` 20), written on learning (Heka screen).
* Not done (optional in the draft): hit chance or health bar shown from farther. The inventory's "Range" row still
  shows the base range.
* Tests: `tests/unit/skills_test.cpp` (every weapon's reach with and without the skill).
