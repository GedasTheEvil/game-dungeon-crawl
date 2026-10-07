# Pressure plate with poison arrows

Status: draft 2026-10-07, from the user.

## Idea

A new trap: a pressure plate in the floor. A player who steps on it sets off poison arrows that fly out of the wall
at them.

* The arrows poison: medium tier (`PoisonTier::Medium`, [poison-and-antidote](solved/poison-and-antidote.md)).
* Jumping over the plate sets off nothing.
* Running (sprinting) across it does not help: the arrows still hit and poison.

## Weight (new mechanic)

Monsters get a weight, and the plate goes off only under a heavy one.

* Small monsters do not set it off: rats, scarabs, bats (bats fly anyway).
* Large monsters do, and the arrows hit them. Cowards (`Courage::Coward`) do not step on traps, so in practice the
  reckless ones: mummies, the Anubis guard, the bosses.
* Weight is a tier, a small whole number, in the monster row (`MonsterKind::weight`,
  `src/world/monster_kinds.cpp`):
  * 0: flyers (bats), they never touch the floor;
  * 1: small monsters (rats, scarabs);
  * 2-3: large monsters;
  * above 3 is fine and expected (bosses, later traps that need more weight).
* The plate goes off at weight 2 and above. The player sets it off too, so the player counts as at least 2.
* Other traps later may use the same tiers with their own threshold.
* Does the plate count as a trap for the cowards' fear (they stop at its edge), or is it hidden from them?
* Damage and poison on a monster: monsters cannot be poisoned yet. The venom amulet needs the same
  ([venom-amulet](venom-amulet.draft.md)); build monster poison once for both, or the arrows only hurt monsters.
* The player's luring a mummy over the plate: a feature to keep.

## Open

* Where the arrows come from: the nearest wall on the plate's row, a wall cell set in the level, or a shooter tile
  of its own? How many arrows, how fast, how much damage on top of the poison.
* Can the player dodge after the trigger (jump the arrow, duck behind something), or is every step a sure hit?
* Once or every time: does the plate re-arm, and after how long?
* Weights: the tier of each monster type; the player's own weight (2?).
* Level format: a new tile type (`DungeonTileType`, a row in `TILES`, [data-tables](solved/data-tables.md)), its
  attribute and value (direction, wall cell).
* Level checker: a plate on the only path means certain poison, so it wants an antidote in reach, as on levels with
  poisoners (`level_check.cpp`).
* Look and sound: the plate model and its click, the arrow (the bow's arrow model?), the wall's dart holes, the
  journal note on the first one.
* Campaign: from which level on.
