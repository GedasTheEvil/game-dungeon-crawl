# Trap-walking monsters

Status: draft 2026-10-01.

## Idea

Some monsters walk straight through traps instead of stopping at their edge (`Locomotion::Walk`) or leaping over them
(`WalkJump`). The traps hurt them the same way they hurt the player, but less, by a per-type **trap resistance**.

The first one is the **Anubis** boss ([boss-rooms.draft.md](boss-rooms.draft.md)): the player cannot shake him off
behind a row of traps. He follows through spikes, death traps and rock falls.

## Trap damage

* A per-type multiplier, not armour: traps ignore armour, so this is a separate trap resistance.
  For example a `trapDamagePct` field in `MonsterType` (100: full damage, 0: immune).
* Anubis: about 10%. A rock that crushes him does 100 instead of `ROCK_CRUSH_DAMAGE` 1000.
* Other trap walkers: about 50%, roughly the player's damage halved.
* Same rules as the player's (`Dungeon::updateTraps` in `dungeon_base.cpp`): spike / death trap hits every
  `TRAP_HURT_INTERVAL_MS` with the streak ramp, rock crush / graze by distance from the cell centre
  (`Dungeon::updateRocks` in `dungeon_mechanisms.cpp`). Each monster needs its own hurt timer and streak.

## What to change

* A new locomotion (for example `Locomotion::WalkThrough`) or a flag on `Walk`: path-finding treats spike, death trap
  and rock-fall cells as open floor.
* Spike pits (no floor below) stay a stop: a walker would fall in.
* Rock fall: a monster stepping into the cell starts the rumble too, like the player. `updateRocks` checks the
  monsters under the rock as well as the player.
* Damage numbers / blood on trap hits, so the player sees the trap hurt him.
* Level checker: `monsterThreat` may need a bump for trap walkers (the player cannot use traps as a wall).

## Open questions

* Which other monsters walk through traps (the mummy, [mummy-minion-monster.draft.md](mummy-minion-monster.draft.md))?
* Can a rock fall kill a trap walker (an ordinary one at 50%: 500 damage), and does that give XP?
* Does the rock reset after it falls on a monster, or stay `Fallen` as for the player?
