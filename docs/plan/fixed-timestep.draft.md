# Stage 9b: held-key movement on the fixed tick

Status: draft 2026-09-30, waits for the user's decision (it changes how walking feels). Stage 9 of the
[code structure review](code-structure-review.draft.md); the random streams part is done
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
2. Choose `WALK_SPEED`. Options:
   * **1.0 tiles/s** (recommended to try first): between today's play (0.625) and the scenarios (1.5); a 40-cell
     corridor takes 40 s instead of ~64 s.
   * 0.625 tiles/s: today's feel without the start pause; scenario walks slow down 2.4x (their time limits and
     waits need a look).
   * 1.5 tiles/s: the scenarios' speed; play gets 2.4x faster, monster fights and jumps feel different.
3. Scenario `walk` steps at the same `WALK_SPEED`, so tests prove the play speed.
4. The jump carries the player forward at the walk speed while the key is held, and the checker's
   `movement.h` can compute the reach (`Jump::ARC.drift` plus the walk), with a `static_assert` that a one-cell gap
   clears from the middle of a cell.
5. Re-run everything that depends on timing: all scenarios, `make paths`, and a playthrough of a few levels for the
   feel (jumps over pits, rock falls, monster chases).

## Also in this stage (small)

* `GameClock::now()` is real time, `Update` runs on a 16 ms GLUT timer without a measured dt: a slow frame slows the
  game. A fixed-dt accumulator (run as many 16 ms updates as the clock says) is the usual fix; it matters little at
  this game's load, so it can wait for the walk change.
