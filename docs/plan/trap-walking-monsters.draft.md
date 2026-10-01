# Trap-walking monsters

Status: draft 2026-10-01.

## Idea

A new per-type trait, **courage**, separate from `Locomotion`:

* **Coward** (the default, most monsters): afraid of traps, never sets foot on one. A `Walk` monster stops at the
  trap's edge, a `WalkJump` monster (giant rat) leaps over it. Leaping is how a coward avoids the trap, so the jumpers
  are cowards too and nothing changes for them.
* **Reckless**: walks straight through traps. The traps hurt it the same way they hurt the player, but less, by a
  per-type **trap resistance**.

Locomotion says what a monster *can* do (walk, leap, fly), courage says whether it *wants* to step on a trap.

Reckless monsters:

* **Anubis** boss ([boss-rooms.draft.md](boss-rooms.draft.md)): the player cannot shake him off behind a row of traps.
  He follows through spikes, death traps and rock falls.
* **Mummy** ([mummy-minion-monster.draft.md](mummy-minion-monster.draft.md)).

## Trap damage

* A per-type multiplier, not armour: traps ignore armour, so this is a separate trap resistance.
  For example a `trapDamagePct` field in `MonsterType` (100: full damage, 0: immune).
* Anubis: 10%. A rock that crushes him does 100 instead of `ROCK_CRUSH_DAMAGE` 1000.
* Mummy: 50%. A crushing rock (500) can kill it.
* A monster killed by a trap gives **no XP**: the player must not farm kills with traps.
* Same rules as the player's (`Dungeon::updateTraps` in `dungeon_base.cpp`): spike / death trap hits every
  `TRAP_HURT_INTERVAL_MS` with the streak ramp, rock crush / graze by distance from the cell centre
  (`Dungeon::updateRocks` in `dungeon_mechanisms.cpp`). Each monster needs its own hurt timer and streak.

## What to change

* `enum class Courage : unsigned char { Coward, Reckless }` in `MonsterType`, a column in `MONSTER_DEFS` /
  `BOSS_DEFS`. For a reckless monster the movement code treats spike, death trap and rock-fall cells as open floor.
* Spike pits (no floor below) stay a stop: a walker would fall in.
* Rock fall: a monster stepping into the cell starts the rumble too, like the player. `updateRocks` checks the
  monsters under the rock as well as the player.
* Damage numbers / blood on trap hits, so the player sees the trap hurt the monster.
* Level checker: `monsterThreat` may need a bump for reckless monsters (the player cannot use traps as a wall).

## Open questions

* Any other reckless monsters besides the mummy and Anubis?
* A reckless jumper (none yet): walk through traps, or still leap over them to take no damage? Decide when one
  exists.
* Does the rock reset after it falls on a monster, or stay `Fallen` as for the player?
