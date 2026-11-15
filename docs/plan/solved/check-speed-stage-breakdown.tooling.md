# make check: which stage eats the median

Status: solved 2026-11-15. Was: draft 2026-11-15, size S, needs nothing first.

## Problem

The [recheck](check-speed.tooling.md) of 2026-11-15 (`tools/friction_report.py --since 2026-11-07`): `make check`
median 124 s (before: 120 s), p90 439 s (600 s), 150 runs, 6.7 h. `make tidy` alone: median 178 s, p90 521 s. `make
unit`: median 39 s, p90 180 s (a shard run was 8 s on an idle machine). Agents' runs stay slow, likely from tidy (a
header change tidies many files) and from builds after a header change, plus waiting for the gate while other tasks run.

## Plan

* Time each stage of `make check` (build, plan lint, format, tidy, unit) in a log line per run, with the number of
  changed files and the gate wait; read it from `friction_report.py`.
* Fix the biggest stage: for tidy, a cache per file hash or tidy only the files that include a changed header
  once; for the build, check whether a header change forces a full rebuild.
* Measure again a week later.

## Done 2026-11-15

* Point 1: `tools/check_stages.sh` (run by `make check`) times build, plan, format, tidy and unit, and appends one line per
  run to `check-stages.log` in the git common dir (all worktrees share it): `files=` changed since the merge base,
  `tidied=` sources clang-tidy ran on, `wait=` seconds the jobs sat at the load gate (summed over jobs; `tools/load_gate.sh`
  writes it to `GATE_WAIT_FILE`), per-stage seconds, `total=`, and `FAIL:<stage>` for a failed run. `tools/friction_report.py`
  prints a per-stage table (median, p90, mean, share) and a split by changed-file count; `CHECK_LOG` overrides the path.
* Point 2: measured on a change of `src/level/gameplay_config.h` (16 cores, idle machine, clean tree). A full build is
  226 s. A header change rebuilds every includer: that is `make`'s dependency tracking and needed (no ccache), so the
  build stays. `make tidy` on that header change tidied 128 sources: 528 s (7 m 13 s on a second cold run). Every run of
  `make check` redid all 128, also after an unrelated fix. Fix: `tools/tidy.sh` caches a clean source, keyed by a hash of
  the source, every repo header in its `.d` file, `.clang-tidy`, flags and clang-tidy version (`TIDY_CACHE=0` turns it
  off). Same header change, warm cache: 8.9 s (was 426 to 528 s); a changed leaf header (`core/logger.h`) re-tidied only its
  26 includers (101 s) and the unchanged tree after it took 8.7 s. A first run after a header change still pays in full;
  the fix loop (several `make check` after one header edit), and the runs after a merge, do not.
* First logged run: build 192 s, plan 2, format 2, tidy 5, unit 8, total 210 s, wait 626 job-s (the build's jobs queueing for
  a free slot at the gate: a 9-slot parallel build, so large; not time lost on a busy machine).
* Point 3: a week-later measurement cannot be done now: [check-speed-week-later](../check-speed-week-later.tooling.draft.sonet.md).
