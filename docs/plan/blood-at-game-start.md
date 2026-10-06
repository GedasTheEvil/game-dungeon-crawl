# Blood splash at game start

Status: implemented 2026-10-06, to be confirmed in play. Draft 2026-10-05.

## Bug

When a game starts, the player bleeds: a blood splash plays, but no damage is taken (health stays full). Only
the animation is wrong.

## Where to look

* The blood splash is a `ParticleSystem` (`src/graphics/particles.h`); Options > Display > Blood switches it off.
* Likely a splash left over or fired on load: a particle system that starts with life left, a hit event from the
  previous game or the menu, or a damage check that fires for 0 damage on the first tick.
* Check New Game, Load and a level change (exit gate) separately: which of them shows it?

## Check

A scenario (`tests/scenarios/`) that starts a game and takes screenshots over the first second: no blood.

## Done

* Cause: the player's `ParticleSystem blood{100}` was built at full life, so it splashed on the first ticks of the
  first game. Monsters stop theirs on spawn (`blood->Stop()`); the player never did.
* Fix: the player's blood starts stopped (`blood{0}`), and `Player::Reanimate` (New Game, Load) stops it, so a
  splash from the last game does not carry over. A level change keeps a running splash: that one is real.
* Check: `tests/scenarios/blood_at_start.txt` (the player stands behind the entrance statue: before the fix red dots
  showed below its plinth at 480 ms).
