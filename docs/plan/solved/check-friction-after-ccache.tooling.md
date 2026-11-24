# Measure make check with friction_report after ccache (not before 2026-11-21)

Status: solved 2026-11-24. Was: draft 2026-11-16, size S, not before 2026-11-21 (ccache landed by [check-build-header-rebuild](check-build-header-rebuild.tooling.md); a few days of runs needed).

## Problem

The last point of check-build-header-rebuild: did ccache lower the build stage's median and p90 (before: median 31 s,
p90 600 s, max 1126 s, 22 runs)?

## Plan

* Run `tools/friction_report.py` and compare the build stage with the numbers above.
* Report the ccache hit rate (`ccache -s`). Note the result here, then move the plan to solved.

## Result

Measured 2026-11-24 from `.git/check-stages.log` (`friction_report.py --since 2026-11-16` splits the stage table by date but
prints no max; max and the before-split come from the log directly). ccache landed 2026-11-16 10:39 (commit b7eb3154).

| Build stage | Runs | Median s | p90 s | Max s |
|---|---|---|---|---|
| Before (plan numbers) | 22 | 31 | 600 | 1126 |
| Before (log, runs before the commit) | 62 | 6.5 | 485 | 1126 |
| After (log, from the commit) | 133 | 4 | 160 | 527 |
| After (log, from 2026-11-17) | 80 | 4 | 117 | 367 |

* The plan's "before" 22 runs came from an older window; the log only starts 2026-11-15, so its own before-split is 62 runs.
  Against both, median and p90 fell; p90 about 3.8x lower (600 to 160), max halved.
* Total `make check` mean 181 s (all time) to 128 s since 2026-11-16 (report, `--since`); build share of mean 46% to 47%
  (tidy and unit dominate what is left).
* ccache hit rate (`ccache -s`): 8482 / 17813 cacheable calls = 47.62% (direct 5849, preprocessed 2633), cache 0.3 / 5.0 GiB.
  Counters are cumulative since the last zero, so this includes the uncached early runs.
* Remaining slow build runs are header-fanout rebuilds (misses) and parallel-agent gate waits; follow-up is
  check-header-fanout, not this plan.

Points: run the report (done), compare build stage (done), ccache hit rate (done), note result and move to solved (done).
Nothing left out.
