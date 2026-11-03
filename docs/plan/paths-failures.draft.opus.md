# `make paths`: half the campaign levels fail their path script

Status: draft 2026-11-03. Needs first: [scenario-skip-world-draw](solved/scenario-skip-world-draw.md) (a run
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
