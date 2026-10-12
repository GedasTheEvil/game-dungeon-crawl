# Bats under a high ceiling

Status: draft 2026-10-12, ready (needs nothing first). From play-testing.

## Bug

A bat roosting in a corridor two cells high (open cell under the bat's cell) wakes up and swoops, but stays in its
own (upper) row. It never comes down to the player's row: the player cannot hit it, it cannot bite the player.
Stalemate.

## Why (today's code)

* A flyer lives on one row: `Monster::row`, the spawn row. `Monster::Fly` (`src/entities/monster_ai.cpp`) only moves
  `x` and `flight.lift` (height above that row's floor, `BAT_LOW_LIFT` .. `BAT_CEILING`).
* It wakes on `sameRow(py)` (0.8 rows tolerance): a jumping player gets close enough, so the bat swoops, at its own
  row's head height, one row too high.
* The bite needs `sameRow(py)` too, the player's melee needs the bat in reach: neither happens.
* Wall probe (`dungeon_monsters.cpp`, `mon.Fly(...)`) checks only the bat's row.

## Idea

The flyer AI dives down to the player's row when the cells between are open.

* Waking: also see a player below, in the same column span, through open cells (no solid tile between the rows,
  within `BAT_SIGHT` horizontally). Limit: 1–2 rows down (implementer's choice; 2 rows covers the reported map).
* Swoop: the target row is the player's row. Change `row` (or a flight row offset) once the bat drops below its
  row's floor, so `sameRow`, the bite, the player's melee and arrows, the wall probe and the draw all use the
  lower row. Drawing must stay smooth across the row change (lift in world units vs. row base).
* Return: fly back up to the spawn row and roost there (`SpawnRow()`), climbing through the same open cells.
* Lost player (climbed away, out of sight): return as today.
* Never dive through solid tiles, platforms or into water (check what tiles a flyer may pass).
* Boss (Bat of Shezmu, `kin = MonsterBat`) and its `Summon::Drop` minions use the same flyer AI, so they get it too.
  Per the boss rule, it may see/dive farther (e.g. one more row).

Alternative (cheaper, implementer may add on top): a level checker warning for a bat whose cell has an open cell
below in the same column, and fix such levels. Does not fix boss rooms or summons, so the AI fix is the main path.

## Tests

* Scenario in `tests/scenarios/`: two-cell-high corridor, bat roosting in the upper cell, player below. Expect: bat
  wakes, dives, bites the player; player kills it with melee; a surviving bat returns to its roost.
* Regression: a one-cell corridor bat behaves as before.

## Open points

* How many rows a bat may dive (1 or 2; boss more?).
* Whether a bat also follows a player who goes *up* (bat below, player above), or only down from the roost.
* Whether to add the level checker warning as well.
