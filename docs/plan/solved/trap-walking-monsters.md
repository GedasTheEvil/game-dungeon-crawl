# Trap-walking monsters

Status: solved 2026-10-01, verified by scenarios (see [Done](#done)).

## Idea

A new per-type trait, **courage**, separate from `Locomotion`:

* **Coward** (the default, most monsters): afraid of traps, never sets foot on one. A `Walk` monster stops at the
  trap's edge, a `WalkJump` monster (giant rat) leaps over it. Leaping is how a coward avoids the trap, so the jumpers
  are cowards too and nothing changes for them.
* **Reckless**: walks straight through traps. The traps hurt it the same way they hurt the player, but less, by a
  per-type **trap resistance**.

Locomotion says what a monster *can* do (walk, leap, fly), courage says whether it *wants* to step on a trap.

Reckless monsters:

* **Anubis** boss ([boss-rooms.md](boss-rooms.md)): the player cannot shake him off behind a row of traps.
  He follows through spikes, death traps and rock falls.
* **Mummy** ([mummy-minion-monster.md](mummy-minion-monster.md)).
* **Anubis** (the plain one), added 2026-10-01: 50%, like the mummy.

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
* Rock fall: a reckless monster stepping into the cell starts the rumble too, like the player. `updateRocks` checks
  the monsters under the rock as well as the player. The rock falls once: after that the cell stays `Fallen`, whoever
  set it off.
* Cowards avoid an armed or falling rock-fall cell like spikes: today `Dungeon::walkerBlocked` stops walkers only at
  `Spike` / `Death`, so they walk into `RockFall` cells. A `Fallen` cell is harmless and open to all. Jumpers leap
  over an armed one (`leapLanding` must not land on it).
* Damage numbers / blood on trap hits, so the player sees the trap hurt the monster.
* Level checker: `monsterThreat` may need a bump for reckless monsters (the player cannot use traps as a wall).

## Open questions

* Any other reckless monsters besides the mummy and Anubis?
* A reckless jumper (none yet): walk through traps, or still leap over them to take no damage? Decide when one
  exists.

## Done

* `Courage` and `trapDamagePct` in `MonsterType`, set from two defaulted columns of `MONSTER_DEFS`: the mummy and
  the Anubis are reckless at 50%, the Anubis boss at 10%. Everything else is a coward at 100%.
* `Dungeon::walkerBlocked(col, row, reckless)`: cowards stop at spikes, death traps and armed or falling rock falls. A
  `Fallen` cell is open to everyone. The reckless walk onto all of them, but they still stop at a pit. Jumpers leap
  over an armed rock fall, because `leapLanding` uses the cowards' rule.
* The spike/death-trap timer and ramp moved into a shared `TrapHurt` (`src/entities/trap_hurt.h`). The player has one
  and each monster has its own. `Dungeon::updateTraps` hurts every monster on the ground (no flyers, no leaper in
  mid-air) whose centre is in a trap's hitbox, using the same `inTrap` test as the player.
* Rock falls: a walker whose centre is in an armed cell starts the rumble (`startRockFall`). The landing crushes or
  grazes the monsters under it the same way it does the player.
* `Monster::TrapHit` scales the damage by `trapDamagePct`, keeping the hundredths for the next hit. It hurts through
  `takeHit(dmg, false)`, which gives blood but no XP. There are no damage numbers in the game, so the blood is the
  only sign of the hit.
* A walker sets off the rumble at the cell's edge, so at the mummy's speed it is only grazed (25 HP), never crushed,
  like a player who walks on. The 500 crush needs it to be in the middle of the cell when the rock lands.
* Not done: the checker's `monsterThreat` was not raised for reckless monsters. All campaign levels still pass with
  no warnings.
* Scenarios: `coward_rock.txt` (a rat stops at an armed rock fall, a giant rat leaps over it, neither sets it off),
  `reckless_spikes.txt` (the mummy stands in spikes to strike, dies of the ramp, no XP), and `reckless_rock.txt`
  (the mummy sets off a rock fall and is grazed for 25), and `reckless_anubis.txt` (the Anubis walks through
  spikes). Each check fails if its rule is removed. All 68 scenarios pass.
