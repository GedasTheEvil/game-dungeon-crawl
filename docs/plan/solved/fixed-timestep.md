# Stage 9b: held-key movement on the fixed tick

Status: draft 2026-09-30; decided 2026-10-01: `WALK_SPEED` 1.0 tiles/s. Implemented 2026-10-01, see [Done](#done),
verified in play 2026-10-01. Stage 9 of the
[code structure review](code-structure-review.md); the random streams part is done
([random-streams.md](random-streams.md)).

## The problem, in numbers

Walking moves `PLAYER_MOVE_STEP` (0.025 tiles) once per key event (`input.cpp`), so its speed is the operating
system's key repeat. On this machine (X: `xset q`): repeat delay 600 ms, rate 25/s. So in play:

* the first step, then nothing for 0.6 s, then 25 steps a second: **0.625 tiles/s** (sprint x3: 1.9 tiles/s);
* another machine with another repeat rate walks at another speed (GNOME's own setting here is 30 ms, 33/s);
* scenario tests step once per 16 ms tick: **1.5 tiles/s**, 2.4 times the play speed, with no pause. Timings a
  scenario proves (a monster reached in N seconds, a jump over a pit with the walk key held) are not the ones the
  player gets.

Jumps add `JUMP_FORWARD_SPEED` per jump tick on top of the walk key, so their reach depends on the repeat rate too;
the level checker (`movement.h`) cannot model it.

## Proposal

1. Track the walk keys as held (`glutKeyboardUpFunc` / `glutSpecialUpFunc`, already wired for Shift) and move once
   per game tick (`Update`, 16 ms) while held, `WALK_SPEED` tiles per second: no repeat delay, the same speed on
   every machine.
2. `WALK_SPEED`: **1.0 tiles/s** (decided 2026-10-01): between today's play (0.625) and the scenarios (1.5); a
   40-cell corridor takes 40 s instead of ~64 s. Sprint stays x3. The options weighed:
   * 1.0 tiles/s: chosen.
   * 0.625 tiles/s: today's feel without the start pause; scenario walks slow down 2.4x (their time limits and
     waits need a look).
   * 1.5 tiles/s: the scenarios' speed; play gets 2.4x faster, monster fights and jumps feel different.
3. Scenario `walk` steps at the same `WALK_SPEED`, so tests prove the play speed.
4. The jump carries the player forward at the walk speed while the key is held, and the checker's
   `movement.h` can compute the reach (`Jump::ARC.drift` plus the walk), with a `static_assert` that a one-cell gap
   clears from the middle of a cell.
5. Re-run everything that depends on timing: all scenarios, `make paths`, and a playthrough of a few levels for the
   feel (jumps over pits, rock falls, monster chases). The player gets faster than today, so monster speeds change
   relative to the player: do this before [monster-strength.draft.md](../monster-strength.draft.md) tunes them.

## Also in this stage (small)

* `GameClock::now()` is real time, `Update` runs on a 16 ms GLUT timer without a measured dt: a slow frame slows the
  game. A fixed-dt accumulator (run as many 16 ms updates as the clock says) is the usual fix; it matters little at
  this game's load, so it can wait for the walk change.

## Done

* `gameplay_config.h`: `UPDATE_TICK_MS` 16 (the game's timer and `Scenario::TICK_MS`), `WALK_SPEED` 1.0,
  `PLAYER_MOVE_STEP` = one tick of it (0.016), climbing up 0.9 of it.
* `input.cpp`: the walk keys set a held flag (`setWalkHeld`), `keyReleased` / `specialKeyReleased` clear it on every
  screen. `Update` calls `stepHeldWalk` once a tick. The key repeat only re-sends the press.
* Scenario `walk` holds the key the same way and releases it at the end.
* `movement.h`: `Jump::WALKING_REACH` (drift 0.97 + walk 0.72 = 1.69 tiles), `static_assert` > 1.5.
* Rock fall: at 1.0 tiles/s walking on no longer cleared the graze (0.95 tiles in 950 ms, 1.1 needed; it never did
  at the old play speed either, only at the scenarios' 1.5). `ROCK_WARN_MS` 650 -> 900, with a `static_assert`.
* Scenarios: 11 failed after the change. Float steps stopping at 4.9999 instead of 5 (monster_anim, monster_blood,
  toon, mechanisms, player_hud, ladder_jump_off): walks go a bit further. More time in spike hitboxes (spikes,
  fall_trap): expected HP updated. The bosses reach the player earlier (anubis_boss, vampire): shorter wait, `god`
  earlier. All 63 pass, `make paths` 15/15, `levelcheck` no warnings.
* Not done: the fixed-dt accumulator (see above, can wait). The real game's `Update` timer is still 16 ms without
  a measured dt.
