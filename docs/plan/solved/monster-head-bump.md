# Monster head bump: jumping monsters hit low ceilings too

Status: solved 2026-11-04 (play-tested by the user). Was: implemented 2026-11-04, waits for the play-test (`bin/game --play tests/play/monster-head-bump.txt`). Was: draft 2026-11-03. Needed first: [head-bump](head-bump.md),
[subcell-monsters](subcell-monsters.md). From the user.

A monster's jump (a walk-jumper's leap, a pounce: `Monster::Jump`, `Monster::UpdateJump`) that meets a low ceiling
stops its rise like the player's ([head-bump](head-bump.md)) and hurts it the same way. A trick for the
player: wait under a low ceiling and let a pouncer leap into it.

* The rise stops under the ceiling, the arc turns into a fall; where it lands follows (a pounce falls short of the
  player). Its height comes from its kind ([subcell-monsters](subcell-monsters.md)).
* Damage: the head bump's rule on the monster's max HP (the larger of 5 HP and 5% of it), straight off the HP.
  Only a real bump, by the same velocity threshold as the player's.
* 50% chance of a stun for 1 to 2 s: `Monster::StunHit` (plan skill-stun), so a boss gets its shorter stun and its
  cooldown as today. Not while lurking.
* The knock (`WorldSound::HeadBump`, from the monster's place) and its hurt sound.
* Monsters do not plan around it: their paths and pounce choice ignore low ceilings, or the trick never works.
  Exception: a monster that already bumped under this ceiling in the last few seconds does not pounce there again
  (pick a short memory), so a fight is not a free stun loop.
* Bosses (AGENTS.md, Boss abilities): a boss jumps and bumps as its kin. Not an ability, so no stronger version.
* Flyers do not bump: a low ceiling lowers their flight band ([subcell-monsters](subcell-monsters.md)).

Implementer's choices:

* Only a shaped cell's sub-cells are a low ceiling for a monster (`Dungeon::leapRoom`), not a whole wall tile over the
  hall: else Babi (3 quarters tall, arc 0.3 tile) bumped in every one-tile room (the Babi arena test caught it).
* The bump caps the arc at the room over the head and the leap falls from there (`Monster::UpdateJump`, returns
  `LeapBump`); the leap's x still runs on, so a pounce lands where it would have (the player's gap rule holds). Hard:
  rise speed there above `HEAD_BUMP_MIN_RISE_FRACTION` (a third) of the take-off's, as the player's.
* The damage and the daze roll are `head_bump.h`, shared with `Player::HeadBump`. The stun begins at the landing
  (`StunHit` refuses a leaping monster). No kind has a hurt sound: the grunt is its attack sound, as the player's.
* The sound has no place (the sound bank has no positional audio): it plays whenever an active monster bumps.
* Memory: `MONSTER_HEAD_BUMP_MEMORY_MS` (6 s) after a bump, no pounce while the monster still stands under a low
  ceiling (`Dungeon::pounce`).
* Tests: `tests/unit/monster_head_bump_test.cpp` on `tests/levels/monster_head_bump`; scenario
  `tests/scenarios/solved/monster_head_bump.txt`.

## Tests

* Unit: a leaper under a low ceiling stops rising and takes the damage; a stun follows about half the bumps; a boss's
  stun is shorter and holds its cooldown; a second pounce under the same ceiling right after does not happen.
* Scenario on `tests/levels/subcells`: a giant rat pounces at the player under a low ceiling.
* Glossary: extend "head bump" to monsters.
