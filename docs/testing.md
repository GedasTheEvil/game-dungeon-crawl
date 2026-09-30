# Scenario tests

Scripted game runs for agents and regression checks. The game reads a script, plays it on a
fixed-step virtual clock, saves screenshots and state, then exits with a status code.

```
make test                                      # every tests/scenarios/*.txt
make test SCENARIO=tests/scenarios/smoke.txt   # one script
JOBS=4 make test                               # at most 4 at a time (default: nproc - 4, at least 1)
HEADLESS=0 make test                           # real windows instead of Xvfb, one at a time
./game tests/scenarios/smoke.txt               # direct run, real window
SCENARIO_DRAW_ALL=1 ./game tests/scenarios/smoke.txt  # the same, watchable
```

`tools/run_scenarios.sh` runs the scripts in parallel, each under its own `xvfb-run` when it is installed.
Run from the repo root.

## Behaviour in test mode

- The start menu is skipped. User keyboard and mouse input is ignored. `key esc` opens the in-game menu; drive it
  with `mouse` / `click` (`tests/scenarios/menu.txt`, `credits.txt`).
- Sound uses the SDL dummy driver (silent).
- One tick = 16 ms of game time. Each tick runs due commands, then `Update()`, then `Draw()`.
  All `timer` objects read the virtual clock, so a script with the same seed gives the same
  frames on every run.
- Ticks run back to back, not every 16 ms of real time. Only frames with a screenshot are drawn in full; the
  others run `Draw()` with a 1x1 scissor (animations advance in `Draw()`), so the screenshots are the same.
  `SCENARIO_DRAW_ALL=1` (set by `HEADLESS=0`) draws every frame at the normal pace, to watch a run.
- Window size comes from `resolution` (default 1280x720).

## Commands

One command per line. `#` starts a comment.

| Command | Meaning |
|---|---|
| `resolution W H` | Window size. Only before `level`. |
| `seed N` | `srand` seed, applied at each `level`. Default 1. |
| `god` | The player takes no damage (`Player::TakeHit`). Death tiles still kill. |
| `level N` / `level path` / `level gen:SEED:D` | Load campaign level N (`levels/lvlN`, [levels.md](levels.md)), any map file, or a generated level with that seed and difficulty 1-10. The player starts fresh, like New Game. Required before gameplay commands. |
| `wait T` | Wait T ticks, or `500ms`, or `2s`. |
| `walk left\|right\|up\|down N` | Move until the player is N tiles away on that axis. `up`/`down` work on ladders only. The command fails if the player does not move for 30 ticks. |
| `jump`, `attack`, `interact` | Same as the key press (interact = pick up / riddle). |
| `camera M N` | Set the camera `rotM`/`rotN` (not clamped). |
| `toon on\|off` | Toon shading (F1): cel-banded lights and ink outlines. |
| `screenshot name` | Save the next frame as `NNN_name.png`. |
| `key C` | Key press, as typed: one character or `enter`, `esc`, `space`, `tab`, `backspace` (`key i` opens the inventory). |
| `riddles PATH` | Load the riddles from one file or a directory instead of `riddles/` ([riddles.md](riddles.md)). |
| `give TYPE ID [N]` | Add N (default 1) items to the inventory. TYPE: `melee` (0 club, 1 sword, 2 spear), `ranged` (0 bow), `potion` (0 small health, 1 large health, 2 might, 3 armor, 4 life, 5 small stamina, 6 large stamina). |
| `xp N` | Gain N XP, like killing monsters. Each level up adds max HP and refills HP and stamina (level 2 at 1000). |
| `savegame FILE` / `loadgame FILE` | Save / load the game. A bare file name is in the output directory; a path is taken as is. Never load from `saves/`: those are real games, not committed, overwritten by play. |
| `chest TYPE ID [N]` | Open N (default 1) treasure chests holding that item: the item plus the random bonus loot, like a pickup. |
| `mouse X Y` | Move the mouse to X% Y% of the window, Y from the bottom (hover). |
| `press X Y` / `release X Y` | Move there, then left button down / up. `click X Y` does both in one tick. |
| `dump` | Write the state line (x, y, hp, stamina, level, screen, alive, won) to the result. |
| `killboss` | The boss in play dies, as if the player killed it (XP, boss gates). Fails if no boss is in play. |
| `expect F OP V` | Assert. F: `x y hp stamina level alive won might armor equip_type equip_id keys xp riddle bars boss minions` (`keys` = bit mask of the lock colours held, red 1, blue 2, green 4, gold 8; `xp` = total XP; `riddle` = 1 while the riddle screen is open; `bars` = living monsters showing their health bar; `boss` = HP of the boss in play, 0 if none; `minions` = living minions), or an item count written as type + id (`potion2`, `melee1`); add `.level` for the item level (`melee1.level`). OP: `== != < <= > >=`. |
| `quit` | End the script. The end of the file also ends it. |

A failed `walk` or `expect` is a soft failure: the script continues, but the exit code is 1.
Player death is a failure unless the script contains `expect alive == 0`.
The run stops after 10 minutes of game time.

## Output

`tests/out/<script-name>/` (deleted at the start of each run):

- `result.txt`: one line per command with the tick, OK/FAIL and details, then `PASS n/m` or `FAIL n/m`.
- `NNN_<name>.png`: screenshots.

Stdout has the summary and one `FAIL line L: ...` line per failure.

| Exit code | Meaning |
|---|---|
| 0 | All commands passed. |
| 1 | An assert failed, a walk was blocked, the player died, or the time limit was reached. |
| 2 | Parse error, no `level` command, or the level could not be loaded. |
| 3 | Crash (signal or exception). |

