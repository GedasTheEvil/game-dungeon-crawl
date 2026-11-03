# `make paths`: half the campaign levels fail their path script

Status: implemented 2026-11-03, waits for the user's play-test (the monster slot fix changes the game). Was: draft 2026-11-03. Needs first: [scenario-skip-world-draw](solved/scenario-skip-world-draw.md) (a run
takes ~1 min instead of ~23). From the user; found by [paths-speed](solved/paths-speed.md).

## Problem

`make paths` plays `levelcheck --script`'s path through every campaign level in the game. At c0af1920, 27 of the 54
fail: lvl8, 20-23, 25-37, 39-47 (not 38). The first failure is almost always a walk that stops short:
`walk to X -> FAIL (blocked after N tiles ...)` or `walk up/down N -> FAIL (blocked after 0.000 tiles)` right after
it; one `killboss -> FAIL` (lvl41); a few end with `expect level == 2 -> FAIL`. Some show `stamina=0`/`20` at the
blocked walk.

The sub-cell hitbox commits (87210a40, 3aef02b6) did not cause it: lvl8 and lvl21 fail the same way at 21335b18.
When it started is unknown.

Either the checker's movement model (`tools/level/`) and the game's physics disagree (what `make paths` exists to
catch), or the script writer (`PathScript` in `tools/level/levelcheck.cpp`) emits steps the game cannot follow (walk
targets, the gate stop, the stamina waits, ladder grip), or the levels changed under the checker.

## Task

1. Run `make paths` (`SCENARIO_TIMES`, docs/performance-testing.md) and group the failures by cause from the
   `tests/out/lvl*/result.txt` lines around the first FAIL (a screenshot there helps: add `shot` to a copy of the
   script).
2. Bisect when it started (`git bisect` on a temporary worktree or `git archive` copies in `/tmp`, a few failing
   levels as the test), to tell a regression from an old gap.
3. Fix at the root, per cause: the checker's model, the script writer, or the game. Never edit a level to dodge a
   checker gap; a level is only fixed when it is really unplayable (AGENTS.md, Campaign levels).
4. Every level passes `make paths`, `bin/levelcheck content/levels/lvl*` stays free of warnings, `make check` passes.
   Report the causes found and what changed for each.

## Result

`make paths`: 54/54 (was 27/54), in ~20 s. `bin/levelcheck content/levels/lvl*`: no warnings. `make check` passes.
Play-test: `bin/game --play tests/play/paths-failures.txt` (level 32, god and peace: walk to Sobek, he must appear).

Not bisected: each cause has a known start, newer than the path scripts. Three causes, by the levels they broke:

1. **Monster effects in god mode (~22 levels).** God mode stops the damage only. The holds and effects added since
   go through: the giant scorpion's grab ("Caught in its claws", lvl21, 33-35, ...), the giant bat's screech daze
   (no climb, a ladder let go of: lvl23, 30, 42), the curses (Fatigue: no stamina regen, so no jump: lvl8), the
   lotus's sedative. The checker does not model monsters, and the curse tests rely on curses in god mode. Fix: a
   scenario command `peace` (`Dungeon::SetPeace`): the monsters stand down as after the win (`monstersStandDown()`
   where the code checked `won`: no attack, step, shot or summon). `levelcheck --script` writes it after `god`.
2. **The boss never appears (lvl32, lvl41: `killboss -> FAIL`). A game bug.** At most `MAX_MONSTERS` (32) monsters
   are in play; one spawns when its tile comes into view and keeps its slot until it dies. Level 32 has 36
   monsters, 41 has 49: a player who slips past them fills every slot, Sobek's tile then spawns nothing (2563 tries
   on lvl32) and the boss gate stays shut. Fix: `freeMonsterSlot`, with no free or dead slot, takes the slot of the
   farthest monster out of all sight (the view grown by the drawn margin), never the boss or a minion, and only one
   whose spawn tile still spawns it: it comes back once its tile is in view. Boss summons use it too. Unit test:
   world_rules_test "the boss still appears when every monster slot holds a living monster".
3. **Quicksand in the checker (lvl39).** The path jumped from 19 over quicksand into quicksand (17), then walked
   out. In the game the walk in quicksand slows to nothing before it crosses a cell (0.75 tiles at most): stuck.
   The game's design (`QUICKSAND_MAX_RUN`) is to walk into the first cell and jump out over the second. Fix in
   `level_check.cpp`: a jump never lands in quicksand, and no walk on from quicksand into more of it. All levels
   stay valid. Left open: the checker still lets a walk cross a single quicksand cell (also over 0.75 tiles); no
   path does today (a jump over it is cheaper).

The lvl8 `walk down 2.97 -> FAIL (blocked after 2.400 tiles)` seen before was a side effect of the monsters (it
passes now).
