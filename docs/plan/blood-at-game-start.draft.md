# Blood splash at game start

Status: draft 2026-10-05. Low priority: fix once the other work is done.

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
