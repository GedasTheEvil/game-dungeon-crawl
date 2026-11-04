# Monster head bump: jumping monsters hit low ceilings too

Status: draft 2026-11-03. Needs first: [head-bump](head-bump.md),
[subcell-monsters](solved/subcell-monsters.md). From the user.

A monster's jump (a walk-jumper's leap, a pounce: `Monster::Jump`, `Monster::UpdateJump`) that meets a low ceiling
stops its rise like the player's ([head-bump](head-bump.md)) and hurts it the same way. A trick for the
player: wait under a low ceiling and let a pouncer leap into it.

* The rise stops under the ceiling, the arc turns into a fall; where it lands follows (a pounce falls short of the
  player). Its height comes from its kind ([subcell-monsters](solved/subcell-monsters.md)).
* Damage: the head bump's rule on the monster's max HP (the larger of 5 HP and 5% of it), straight off the HP.
  Only a real bump, by the same velocity threshold as the player's.
* 50% chance of a stun for 1 to 2 s: `Monster::StunHit` (plan skill-stun), so a boss gets its shorter stun and its
  cooldown as today. Not while lurking.
* The knock (`WorldSound::HeadBump`, from the monster's place) and its hurt sound.
* Monsters do not plan around it: their paths and pounce choice ignore low ceilings, or the trick never works.
  Exception: a monster that already bumped under this ceiling in the last few seconds does not pounce there again
  (pick a short memory), so a fight is not a free stun loop.
* Bosses (AGENTS.md, Boss abilities): a boss jumps and bumps as its kin. Not an ability, so no stronger version.
* Flyers do not bump: a low ceiling lowers their flight band ([subcell-monsters](solved/subcell-monsters.md)).

## Tests

* Unit: a leaper under a low ceiling stops rising and takes the damage; a stun follows about half the bumps; a boss's
  stun is shorter and holds its cooldown; a second pounce under the same ceiling right after does not happen.
* Scenario on `tests/levels/subcells`: a giant rat pounces at the player under a low ceiling.
* Glossary: extend "head bump" to monsters.
