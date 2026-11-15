# Scenario command monsterhp: set the nearest monster's HP

Status: solved 2026-11-15. Was: draft 2026-11-15, size S, needs nothing first.

## Problem

Multi-hit scenarios with a real weapon kill the monster too soon, and a test of a low-HP state (a flee, a boss phase,
the health bar's colours) has to hit its way down first. The test weapons ([test-weapons](test-weapons.tooling.md))
fix the first with a fake weapon; this fixes both with the real weapon and its real damage numbers.

## Plan

* `monsterhp N` (`src/level/scenario_script.cpp`, next to `hurtboss`): the living monster nearest the player gets
  N HP (N >= 1; above its max HP is allowed, the max grows with it so the health bar stays right). Fails if no monster
  lives. `bosshp N` does the same for the boss in play (fails without one), so boss phases that hang on HP update.
* Setting HP is no hit: no hit reaction, no XP, no journal note, no aggro.
* `expect nearest` and `expect boss` already read it back.
* Docs: the command rows in `docs/testing.md`.
* A scenario: `monsterhp 1000`, five club swings, `expect nearest` at 1000 minus five club hits.
