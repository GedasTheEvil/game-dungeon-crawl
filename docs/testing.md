# Scenario tests

Scripted game runs for agents and regression checks. The game reads a script, plays it on a
fixed-step virtual clock, saves screenshots and state, then exits with a status code.

```
make test                                      # every tests/scenarios/*.txt
make test SCENARIO=tests/scenarios/smoke.txt   # one script
HEADLESS=0 make test                           # real window instead of Xvfb
./game tests/scenarios/smoke.txt               # direct run, real window
```

`tools/run_scenarios.sh` uses `xvfb-run` when it is installed. Run from the repo root.

## Behaviour in test mode

- The menu is skipped. User keyboard and mouse input is ignored.
- Sound uses the SDL dummy driver (silent).
- One tick = 16 ms of game time. Each tick runs due commands, then `Update()`, then `Draw()`.
  All `timer` objects read the virtual clock, so a script with the same seed gives the same
  frames on every run.
- Window size comes from `resolution` (default 1280x720).

## Commands

One command per line. `#` starts a comment.

| Command | Meaning |
|---|---|
| `resolution W H` | Window size. Only before `level`. |
| `seed N` | `srand` seed, applied at each `level`. Default 1. |
| `god` | The player takes no damage (`stats::GetHit`). Death tiles still kill. |
| `level N` / `level path` / `level gen:SEED:D` | Load campaign level N (`Levels/lvlN`, [levels.md](levels.md)), any map file, or a generated level with that seed and difficulty 1-10. The player starts fresh, like New Game. Required before gameplay commands. |
| `wait T` | Wait T ticks, or `500ms`, or `2s`. |
| `walk left\|right\|up\|down N` | Move until the player is N tiles away on that axis. `up`/`down` work on ladders only. The command fails if the player does not move for 30 ticks. |
| `jump`, `attack`, `interact` | Same as the key press (interact = pick up / riddle). |
| `camera M N` | Set the camera `rotM`/`rotN` (not clamped). |
| `screenshot name` | Save the next frame as `NNN_name.png`. |
| `key C` | Key press, as typed: one character or `enter`, `esc`, `space`, `tab` (`key i` opens the inventory). |
| `give TYPE ID [N]` | Add N (default 1) items to the inventory. TYPE: `melee` (0 club, 1 sword, 2 spear), `ranged` (0 bow), `potion` (0 small health, 1 large health, 2 might, 3 armor, 4 life, 5 small stamina, 6 large stamina). |
| `savegame FILE` / `loadgame FILE` | Save / load the game. A bare file name is in the output directory; a path is taken as is (`Saves/save0.sav`). |
| `chest TYPE ID [N]` | Open N (default 1) treasure chests holding that item: the item plus the random bonus loot, like a pickup. |
| `mouse X Y` | Move the mouse to X% Y% of the window, Y from the bottom (hover). |
| `press X Y` / `release X Y` | Move there, then left button down / up. `click X Y` does both in one tick. |
| `dump` | Write the state line (x, y, hp, stamina, level, screen, alive, won) to the result. |
| `expect F OP V` | Assert. F: `x y hp stamina level alive won might armor equip_type equip_id keys` (`keys` = bit mask of the lock colours held, red 1, blue 2, green 4, gold 8), or an item count written as type + id (`potion2`, `melee1`); add `.level` for the item level (`melee1.level`). OP: `== != < <= > >=`. |
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

## Out of scope (v1)

Menu interaction and answering riddles.
