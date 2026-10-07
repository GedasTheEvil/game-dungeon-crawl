# Pressure plate with poison arrows

Status: draft 2026-10-07, from the user.

## Idea

A new trap: a pressure plate in the floor. A player who steps on it sets off poison arrows that fly out of the wall
at them.

* The arrows poison: medium tier (`PoisonTier::Medium`, [poison-and-antidote](solved/poison-and-antidote.md)).
* Jumping over the plate sets off nothing.
* Running (sprinting) across it does not help: the arrows still hit and poison.

## Open

* Where the arrows come from: the nearest wall on the plate's row, a wall cell set in the level, or a shooter tile
  of its own? How many arrows, how fast, how much damage on top of the poison.
* Can the player dodge after the trigger (jump the arrow, duck behind something), or is every step a sure hit?
* Once or every time: does the plate re-arm, and after how long?
* Monsters: do they set it off, and can the arrows hit them?
* Level format: a new tile type (`DungeonTileType`, a row in `TILES`, [data-tables](solved/data-tables.md)), its
  attribute and value (direction, wall cell).
* Level checker: a plate on the only path means certain poison, so it wants an antidote in reach, as on levels with
  poisoners (`level_check.cpp`).
* Look and sound: the plate model and its click, the arrow (the bow's arrow model?), the wall's dart holes, the
  journal note on the first one.
* Campaign: from which level on.
