# Jump onto a ladder: up or down grabs the rungs

Status: draft 2026-11-04, needs nothing first. Edge-case fix for the player; leaping climbers (giant rat, Babi)
catch the rungs mid-leap too.

## Problem

The player jumps onto (into) a ladder cell and presses up: the camera shudders, no grab, no climb.

Cause (`src/world/dungeon_base.cpp`): the jump and the climb both drive the player every tick.

* `Dungeon::Move` climbs whenever the cell is a `Ladder` (`mapY += dirY`) and pulls `mapX` toward `LADDER_GRIP_X`.
  It does not look at `jump.jumping`.
* `UpdateMovementState` still runs the jump: `mapY += jump.velocity`, gravity, `Move(jump.dir_x * speed, 0)`
  pushes `mapX` away from the grip again. The jump ends only when `mapY <= jump.start_y`, then snaps `mapY` to it:
  the climbed height is thrown away.
* `game_loop.cpp`: `climbing = !jumping && PlayerOnLadder()`, so the model stays in the jump clip, no climb pose.

Up/down and gravity fight over `mapY` (the grip pull and `dir_x` over `mapX`) each tick: the camera shudders.
A plain fall onto a ladder is fine (the fall stops in a `Ladder` cell); only the jump has it.

## Fix

Up or down pressed during a jump, in a `Ladder` cell, with a grip allowed (not `lostGrip`, not dazed, not crouched,
not held): the hands catch the rungs. The jump ends there (`jumping = false`, `falling = false`, `velocity = 0`,
`fall_velocity = FALL_STEP`, `dir_x = 0`), `mapY` stays where it is (no snap to `start_y`), and the same press climbs
as on the ladder already. From the next tick `PlayerOnLadder()` and the climb pose hold as usual.

Place: `Dungeon::Move`, before the climb (or a small `grabLadder()` it calls). Keep it in `Dungeon`, not input.

Jump off a ladder while up is held: must not grab back at once. Grab only once the jump comes down
(`jump.velocity <= 0`), or only from a jump that did not start on this ladder. Implementer's choice; the first is
simpler and also covers a jump straight up a ladder (rise, then the hands catch it on the way down).

Optional: the catch snaps `mapX` to the grip (`floor(mapX) + LADDER_GRIP_X`) if `PlayerOnLadder()` would otherwise
fail its `LADDER_REACH` test for the first ticks (check in the unit test; the pull in `Move` may already be enough).

No sound needed (the climb has none).

## Monsters: a leaping climber catches the rungs too

Who: kinds that both leap (`leaps(locomotion)`) and climb (`climbs`): the giant rat and Babi. Babi is the baboon's
boss (the baboon does not leap); no boss has the giant rat as `kin`. Leapers that do not climb (giant scarab, Khepri,
the cursed chest) and climbers that do not leap: unchanged.

Today (`src/entities/monster_leap.cpp`, `monster_climb.cpp`, `src/world/dungeon_walkers.cpp`): a leap is a fixed
sideways arc on its row (`Monster::Jump`, `UpdateJump`, `leap.lift`); a climb starts only from the floor
(`StartClimb`: `climb.y = 0`), after `followPath` walks the monster to the ladder's middle. `ClimbMap` lets a leap land
in a ladder cell (`Stands` is true there), but never starts a climb in the air.

Change:

* Mid-leap, a climber whose next path step goes up (or down) the ladder in the cell it is passing over catches the
  rungs: the leap ends there (`leap.startMs = -1`, `leap.lift = 0`), `x` goes to the ladder's middle, and the climb
  starts at the arc's height (`climb.y` = `leap.lift` in tiles; `StartClimb` gets a start height, 0 by default).
  The hold at the ladder's middle in `followPath` is skipped for this case.
* `followPath` can choose such a leap: a leap whose arc crosses the ladder column where the path climbs, instead of
  a walk to the rungs. Keep it to cases where it is the way the path goes anyway (a gap, a trap, a pit, or a ladder
  with no floor under its lowest rung); no new jumps where a walk works.
* Only while alerted and following the player up or down (the climbers' rules, plan monster-climbers). The
  `CreatureMove::Climb` journal note as for any climb.
* Babi (boss): catches the rungs from farther along the arc and higher up (a longer leap to the ladder,
  `BOSS_CLIMB_SPEED_FACTOR` after the catch). Exact numbers: the implementer's choice.
* Draw: the climb clip and `ClimbDepth` take over at the catch, from the arc's height, with no pop down to the floor
  first (check the screenshot).

Check: whether any campaign level has a ladder whose lowest rung has no floor under it (only reachable by a jump).
If not, the monster side only matters next to gaps; say so in the report.

## Tests

Unit test (`tests/unit/`, as `screech_test.cpp` sets up a ladder): a player beside a ladder cell jumps into it,
MoveUp pressed after the apex:

* the jump ends that tick, `PlayerOnLadder()` true;
* holding up for N ticks raises `mapY` steadily (each tick >= the last, no drop back to `start_y`);
* jump off the ladder with up held: the jump rises first (no instant re-grab).

Unit test, monsters: a giant rat chasing the player up a ladder past a gap leaps, catches the rungs mid-arc (it is
climbing, `climb.y > 0`, `x` on the ladder's middle) and reaches the player's row; a giant scarab in the same spot
does not climb. Scenario with a screenshot of the catch (no pop to the floor).

Play-test script `tests/play/jump-onto-ladder.txt`: start next to a ladder, a floor or two of rungs above, and a
giant rat across a gap from the ladder's foot.
