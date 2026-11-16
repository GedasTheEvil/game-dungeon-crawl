# make check: measure the stage log a week later

Status: solved 2026-11-16. Was: draft 2026-11-15, size S, needs nothing first.

## Problem

Point 3 of [check-speed-stage-breakdown](check-speed-stage-breakdown.tooling.md): measure again a week after its tidy cache and stage log landed. Not before 2026-11-22, so the log holds a week of real runs.


## Plan

* `python3 -I tools/friction_report.py --since <the merge date>`: the per-stage table of `make check` (median, p90, share per stage, runs by changed files). Compare with the numbers in the solved plan.
* Compare the tidy stage: median and p90 against before (tidy median 178 s, p90 521 s; a gameplay_config.h change tidied 128 sources in 528 s).
* If a stage still dominates, write a draft for it; record the result in this plan.
 

## Result

Done 2026-11-16, on request, earlier than the "not before 2026-11-22" in the draft. Caveat: the stage log covers only
about 1 day (2026-11-15 16:33 to 2026-11-16 10:12), 55 runs (11 failed), not a week. Read the numbers as a first look.
The log also mixes load from parallel agent tasks (gate wait up to 8717 job-s).

`python3 -I tools/friction_report.py --since 2026-11-15`, `make check` per stage:

| Stage | Median s | p90 s | Share of mean total |
|---|---|---|---|
| build | 17 | 695 | 51% |
| plan | 4 | 8 | 1% |
| format | 4 | 6 | 1% |
| tidy | 28 | 534 | 39% |
| unit | 14 | 78 | 10% |
| total | 206 | 1027 | 100% |

Runs by changed files: 0-2 files 5 runs, median 299 s (a build); 3-9 files 20 runs, median 106 s; 10+ files 30 runs,
median 276 s, tidy 80 s (96 sources). `make check` as a whole (report table, 135 runs): median 27 s, p90 293 s (before:
median 124 s, p90 439 s; that older figure came from transcripts, a different mix of runs).

Tidy, before vs now (`make tidy` alone: before median 178 s, p90 521 s):

* All 55 logged runs: tidy stage median 28 s, p90 534 s. Not a clean comparison: the tidy fan-out fix
  ([tidy-header-fanout](tidy-header-fanout.tooling.md)) landed 2026-11-15 22:03, in the middle of the window.
* Before the fan-out fix (33 runs, cache only): tidy median 19 s, p90 575 s, max 781 s. A header change still tidied
  113 to 130 sources, cold 443 to 671 s (the cache helps only repeats: 8 to 52 s). The cache works, the first run after a
  header change did not get faster.
* After the fan-out fix (22 runs): tidy median 39 s, p90 353 s; later runs tidied 1 to 11 sources, 28 to 134 s (slow
  only from load at the gate). The cold header case is fixed by the fan-out, not by the cache.
* So tidy is no longer the big stage for a normal run. `make tidy-push` pays the full fan-out before a push.

Dominant stage now: build (51% of the mean, p90 695 s; 19 of 55 builds over 100 s). A header change rebuilds every
includer, there is no compiler cache. New draft:
[check-build-header-rebuild](../check-build-header-rebuild.tooling.draft.sonet.md). Measure again once the log holds a real
week.
