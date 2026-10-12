# Bats under a high ceiling

Status: draft 2026-10-12, refined 2026-10-12, ready (needs nothing first). From play-testing.

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

The flyer AI follows the player to another row, down or up, when the cells between are open.

* Sight is a radius (user decision): the bat sees the player within `BAT_SIGHT` as a distance in tiles (horizontal
  and vertical), through open cells only (no solid tile on the way). So the rows it follows come from its sight:
  1.75 tiles today = one row up or down; a longer sight reaches farther. No separate row limit.
  The same radius decides when it loses the player (`sees` in `Fly`), as today on the row.
* Swoop: the target row is the player's row. Change `row` (or a flight row offset) once the bat drops below its
  row's floor, so `sameRow`, the bite, the player's melee and arrows, the wall probe and the draw all use the
  lower row. Drawing must stay smooth across the row change (lift in world units vs. row base).
* Return: fly back up to the spawn row and roost there (`SpawnRow()`), climbing through the same open cells.
* Up as well (user decision): a player above the bat (climbing a ladder, on a higher row of a tall shaft) is chased
  through open cells and bitten there. A player on the rungs cannot attack ([no-attack-on-ladder](no-attack-on-ladder.md)),
  so the bat gets free bites there; this is intended. The bite then needs the player's actual height (`py` on the
  ladder), not only the row.
* Lost player (out of sight, no open path): return as today.
* Never dive through solid tiles, platforms or into water (check what tiles a flyer may pass).
* Boss (Bat of Shezmu, `kin = MonsterBat`) and its `Summon::Drop` minions use the same flyer AI, so they get it too.
  They hunt along the whole row (no horizontal radius); vertically they get a fixed reach, farther than a bat's
  (about 3 rows, a gameplay constant).

Alternative (cheaper, implementer may add on top): a level checker warning for a bat whose cell has an open cell
below in the same column, and fix such levels. Does not fix boss rooms or summons, so the AI fix is the main path.

## Tests

* Scenario in `tests/scenarios/`: two-cell-high corridor, bat roosting in the upper cell, player below. Expect: bat
  wakes, dives, bites the player; player kills it with melee; a surviving bat returns to its roost.
* Scenario: player climbs a ladder past a bat's row; the bat follows up and bites the player on the rungs.
* Regression: a one-cell corridor bat behaves as before.

## Open points

* Whether to add the level checker warning as well.
