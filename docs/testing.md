# Tests

Two kinds: unit tests of the GL-free library code, and scenario tests of the running game. `make test` runs both
(the unit tests first).

## Unit tests

[doctest](https://github.com/doctest/doctest) (one header, `external/doctest/doctest.h`). `tests/unit/*_test.cpp`
link `build/liblevel.a` and `build/libbase.a` only: no window, no GL. `make unit` builds and runs `build/unit`; `./build/unit -tc="*gate*"`
runs the matching test cases. A new file in `tests/unit/` is picked up by the makefile. Test the rules there first;
a scenario is for what needs the game running (drawing, timing, input).

## Scenario tests

Scripted game runs for agents and regression checks. The game reads a script, plays it on a
fixed-step virtual clock, saves screenshots and state, then exits with a status code.

```
make test                                      # every tests/scenarios/*.txt
make test SCENARIO=tests/scenarios/smoke.txt   # one script
JOBS=4 make test                               # at most 4 at a time (default: (nproc - RESERVE_CORES) * 4 / 5, at least 1)
RESERVE_CORES=6 MIN_FREE_MB=4096 make test     # start a new one only while 6 cores idle and 4 GB free (default 4, 4096)
MIN_SWAP_FREE_MB=2048 make test                # and 2 GB of swap free (default 4096)
HEADLESS=0 make test                           # real windows instead of Xvfb, one at a time
./game tests/scenarios/smoke.txt               # direct run, real window
SCENARIO_DRAW_ALL=1 ./game tests/scenarios/smoke.txt  # the same, watchable
```

`tools/run_scenarios.sh` runs the scripts in parallel, each under its own `xvfb-run` when it is installed.
Each game starts through the load gate `tools/load_gate.sh`, the one `make tidy` and the build use too: before a
start it samples CPU idle (1 s), available memory and free swap, and waits while any is short or `JOBS` jobs run.
The jobs of all runs at once count together. A game started less than `GATE_LOAD_S` seconds ago (default 15) still
counts as `GATE_LOAD_MB` (default 1500) more memory to come: it loads its models and textures for several seconds.
Gated jobs run on the last `GATE_CPUS` cores (default: the job cap), so the first cores stay free for the desktop.
Each game runs niced with one llvmpipe thread (`LP_NUM_THREADS=1`). The last lines report peak parallel runs,
seconds held back, minimum idle cores, minimum free memory (and at the start), minimum free swap and the wall time.
Run from the repo root.

## Behaviour in test mode

- The start menu is skipped. User keyboard and mouse input is ignored. `key esc` opens the in-game menu; drive it
  with `mouse` / `click` (`tests/scenarios/menu.txt`, `credits.txt`).
- Sound uses the SDL dummy driver (silent).
- One tick = 16 ms of game time. Each tick runs due commands, then `Update()`, then `Draw()`.
  All `timer` objects read the virtual clock, so a script with the same seed gives the same
  frames on every run.
- Ticks run back to back, not every 16 ms of real time. Only frames with a screenshot are drawn in full; the
  others run `Draw()` with a 1x1 scissor (the UI screens advance their own animations there, such as the
  inventory's turntable; the game world moves in `Update()` only), so the screenshots are the same.
  `SCENARIO_DRAW_ALL=1` (set by `HEADLESS=0`) draws every frame at the normal pace, to watch a run.
- Window size comes from `resolution` (default 1280x720).

## Commands

One command per line. `#` starts a comment.

A new command is a row in `commandDefs()` (`src/test/scenario_script.cpp`: name, argument parser, usage), its run in
`runInstant` (`src/test/scenario.cpp`, `-Wswitch` asks for it), an example line in
`tests/unit/scenario_script_test.cpp` and a row in the table below. A new `expect` field is a row in `fieldDefs()` and
its getter in `src/test/scenario_fields.cpp`. The unit tests fail until this page lists both.

| Command | Meaning |
|---|---|
| `resolution W H` | Window size. Only before `level`. |
| `seed N` | Seed of the game's random streams (`GameRandom`: loot, the riddle deck, clip start frames), applied at each `level`. Default 1. Each blood effect has its own stream. |
| `god` | The player takes no damage (`Player::TakeHit`). Death tiles still kill. |
| `level N` / `level path` / `level gen:SEED:D` | Load campaign level N (`levels/lvlN`, [levels.md](levels.md)), any map file, or a generated level with that seed and difficulty 1-10. The player starts fresh, like New Game. Required before gameplay commands. |
| `wait T` | Wait T ticks, or `500ms`, or `2s`. |
| `walk left\|right\|up\|down N` | Hold the walk key until the player is N tiles away on that axis: the play speed (`WALK_SPEED`, one step a tick, `stepHeldWalk`). `up`/`down` work on ladders only. The command fails if the player does not move for 30 ticks. Float steps can stop a hair short (4.9999 for 5): walk a bit past a cell edge you need to be in. |
| `walk to X` | Walk along the row until the player's map x reaches X, whichever way it is. Fails like `walk` when blocked. `levelcheck --script` uses it, so its steps do not add up errors. |
| `hold left\|right\|up\|down T` | Hold the walk key for T (ticks, `500ms`, `2s`), whether the player moves or not. Never fails: for walking into a wall. |
| `sprint on\|off` | Shift down / up (sprint). It stays down until `sprint off`. |
| `jump`, `attack`, `interact` | Same as the key press (interact = pick up / riddle). |
| `camera M N` | Set the camera `rotM`/`rotN` (not clamped). |
| `motion on\|off` | Options > Display > Motion effects (sprint blur, FOV kick, vignette). On by default; scenarios never read or write `saves/settings.ini` ([settings.md](settings.md)): they run with the default settings and key bindings. |
| `toon on\|off` | Toon shading (F1): cel-banded lights and ink outlines. |
| `hitboxes on\|off` | Debug outlines (F3): monster hitboxes red, the player's green, the equipped weapon's reach yellow. Only after `level` (a level load keeps it). |
| `screenshot name` | Save the next frame as `NNN_name.png`. |
| `key C` | Key press, as typed: one character or `enter`, `esc`, `space`, `tab`, `backspace` (`key i` opens the inventory), or a special key by its name in [settings.md](settings.md) (`key left`, `key f12`, `key f1`). |
| `hurt N` | The player loses N HP straight away (no armour, ignores `god`), to test healing and the HUD. |
| `poison weak\|medium\|strong` | The player is poisoned with that tier, as by a poisoned bite: it (re)starts that tier's timer ([poison](plan/solved/poison-and-antidote.md)). Works in `god` mode too, only the damage is not taken. |
| `poisonmonster weak\|medium\|strong` | The living monster nearest the player is poisoned with that tier, as by the player: its kill by the poison gives the XP. It may shrug it off (`MonsterKind::poisonResistPercent`, [monster poison](plan/solved/monster-poison.md)); `expect nearest_poison` tells. |
| `prop COL NAME` | Put the prop NAME (`DECOR_NAMES` in `src/world/decor.h`: `cat`, `osiris`, `thoth_ibis_standing`, ...) on column COL of the player's row, over whatever the level's scatter picked there. For props the scatter would only place by chance. |
| `riddles PATH` | Load the riddles from one file or a directory instead of `riddles/` ([riddles.md](riddles.md)). |
| `give TYPE ID [N]` | Add N (default 1) items to the inventory. TYPE: `melee` (0 club, 1 short sword, 2 spear, 3 dagger, 4 khopesh, 5 epsilon axe, 6 duckbill axe, 7 mace), `ranged` (0 self-bow, 1 composite bow, 2 sling, 3 throwing stick, 4 javelin), `potion` (0 small health, 1 large health, 2 might, 3 armor, 4 life, 5 small stamina, 6 large stamina, 7 antidote), `amulet` (its place among the amulets: 0-3 strength lesser .. grand, 4-7 armor, 8-11 health, 12-15 poison warding, 16-19 trap warding, 20-23 blunt, 24-27 slash, 28-31 pierce warding, 32-33 regeneration normal, grand, 34-37 venom; [tools/editor/readme.md](../tools/editor/readme.md)). |
| `select ITEM` | In the open inventory, select the item and open its tab, like a click on its slot. ITEM: its label with `_` for spaces (`club`, `short_sword`, `spear`, `self-bow`, `small_health`, `large_health`, `might`, `armor`, `life`, `small_stamina`, `large_stamina`, `antidote`). Fails when the inventory is not open. Scenarios pick items with it, not by slot position. |
| `equip WEAPON` | Take the weapon in hand, like a click on its slot in the inventory: `club`, `dagger`, `short_sword`, `khopesh`, `epsilon_axe`, `duckbill_axe`, `mace`, `spear`, `self-bow`, `composite_bow`, `sling`, `throwing_stick`, `javelin`. Fails when the weapon is not held. Scenarios equip with it, not with the number keys (they cycle per class). |
| `wear AMULET` / `wear off` | Put the amulet on, like a click on its slot and the Wear button (one at a time: the worn one comes off), or take the worn one off. AMULET: its label with `_` for spaces (`lesser_amulet_of_strength`, `amulet_of_health`, `grand_amulet_of_trap_warding`). Fails when it is not held. |
| `xp N` | Gain N XP, like killing monsters. Each level up adds max HP and refills HP and stamina (level 2 at 1000). |
| `savegame FILE` / `loadgame FILE` | Save / load the game. A bare file name is in the output directory; a path is taken as is. Never load from `saves/`: those are real games, not committed, overwritten by play. |
| `chest TYPE ID [N]` | Open N (default 1) treasure chests holding that item: the item plus the random bonus loot, like a pickup. |
| `mouse X Y` | Move the mouse to X% Y% of the window, Y from the bottom (hover). |
| `press X Y` / `release X Y` / `click X Y` | Move there, then left button down / up; `click` does both in one tick. |
| `dump` | Write the state line (x, y, hp, stamina, level, screen, alive, won; screen is `menu`, `inventory`, `riddle`, `map`, `journal` or `gameplay`) to the result. |
| `killboss` | The boss in play dies, as if the player killed it (XP, boss gates). Fails if no boss is in play. |
| `hurtboss N` | The boss in play takes an N HP hit, as from the player. Fails if no boss is in play. |
| `poisonboss weak\|medium\|strong` | The boss in play is poisoned with that tier, as by `poisonmonster`. Fails if no boss is in play. |
| `expect F OP V` | Assert. F: `x y hp stamina level alive won might armor equip_type equip_id keys poison xp riddle bars boss minions attacking decor_tier nearest nearest_poison coffins chests journal journal_solved journal_notes journal_tried maxhp worn safe` (`maxhp` = max HP with the worn amulet; `worn` = the worn amulet's id (as in `give amulet`), -1 if none; `safe` = 1 while no monster can chase or attack the player (the regeneration amulet heals); `keys` = bit mask of the lock colours held, red 1, blue 2, green 4, gold 8; `poison` = bit mask of the poison tiers running, weak 1, medium 2, strong 4; `xp` = total XP; `riddle` = 1 while the riddle screen is open; `bars` = living monsters showing their health bar; `boss` = HP of the boss in play, 0 if none; `minions` = living minions; `attacking` = 1 while a swing or a bow draw is under way; `decor_tier` = the highest decoration tier (`decor.h`: 0 cave .. 3 temple) among the level's props, decals and surfaces; `nearest` = HP of the living monster nearest the player, 0 if none; `nearest_poison` = its poison tiers running, as `poison`; `coffins` = coffins standing on the level; `chests` = treasure chests not opened yet; `journal` = riddles written down in the journal, `journal_solved` = those of them solved, `journal_notes` = field notes written, `journal_tried` = damage types whose effect on a creature is written down, its poison resistance counted as one more, summed over the creatures), or an item count written as type + id (`potion2`, `melee1`); add `.level` for the item level (`melee1.level`). OP: `== != < <= > >=`. |
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

