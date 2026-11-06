# make check runs faster

Status: draft 2026-11-06, size M, needs nothing first.

## Problem

`make check` is the costliest step agents take. From the week 2026-10-30 to 11-06
(`python3 -I tools/friction_report.py --since 2026-10-30`): 225 runs in 68 sessions, median 120 s, p90 600 s,
12.3 h in total.

Stage times on a clean tree (16 cores, nothing to rebuild, measured 2026-11-06):

| Stage | Time |
|---|---|
| `make` (nothing to do) | 2.4 s |
| `plan.py links`, `lint` | 0.2 s |
| `plan_test.py` | 2.1 s |
| `make format` | 2.3 s |
| `make format-check` | **25.7 s** |
| `make tidy` (no changed file) | short; ~100 s median with changed files (friction report) |
| `make unit` | **24 s** (861 tests, 22.9 s in the tests themselves) |

So about 55 s is spent before any real work, every run.

* `format-check` (`tools/check_format.sh`) runs clang-format twice per file, one file after another, on every
  file in `FORMAT_FILES`. It runs right after `make format`, so "not formatted" can never show there. Only the
  "unstable" check (a file that changes on a second pass) can.
* `bin/unit` runs all 861 doctest cases one after another. The slowest are the zone-table and level-generation
  tests (`a zone level's traps follow the zone table` 2.3 s, `terrain and monsters` 1.8 s, `boss level` 1.6 s, the
  model clip and kind-height tests ~1.3 s each). They generate or load whole levels and models.
* Rebuild fan-out: `core/gameplay_config.h` is included by 60 files, `world/dungeon.h` by 37, `world/level.h` by 33.
  Changing one tuning number rebuilds a quarter of the tree.

## Plan

Measure first, change in this order, and record the time after each step in this plan:

1. `format-check` in `make check`: check only the "unstable" case, on the files `make format` touched or that git
   shows as changed (`git diff --name-only HEAD` plus untracked), run in parallel (`xargs -P` under the load gate,
   `tools/load_gate.sh`). The standalone `make format-check` keeps checking every file. Target: < 3 s on a
   clean tree.
2. `make unit` in parallel: split the doctest run into N shards (`bin/unit --test-case=...` lists, or doctest's
   `--order-by=name` with `--first`/`--last` windows), each run through the load gate, all output collected, and
   failure if any shard fails. Keep the single-process run as `make unit-serial` for debugging. Target: < 8 s.
3. Speed up the slowest unit tests where that keeps what they check: cache a generated level or loaded model
   across test cases in a file (a function-local static), and check fewer seeds where many add nothing.
4. Only if the numbers still say so: split `core/gameplay_config.h` by topic (combat, movement, traps, UI, ...),
   so a tuning change rebuilds only its users. Measure with `touch` + `make` before and after.
5. Run `friction_report.py` a week later and record the new median and p90 here.

Do not drop any check. The gain must come from doing the same work faster or skipping work that cannot fail.

## Open points

* Shard count: start with the load gate's job cap.
* Whether `make check` should skip `unit` when no `src/`, `tests/unit/` or `content/` file changed. Not by default:
  agents rely on a green check.
