# make check speed: recheck a week later

Status: draft 2026-11-06, size S, needs nothing first. On hold: waits for a week of runs, until 2026-11-13.

## Problem

[check-speed](solved/check-speed.tooling.md) made `make check` faster on 2026-11-06 (format-check on changed files,
unit test shards, a parallel build). Before: 225 runs a week, median 120 s, p90 600 s, 12.3 h in total
(`python3 -I tools/friction_report.py --since 2026-10-30`). Whether agents' runs got faster is not measured yet.

## Plan

* On or after 2026-11-13: `python3 -I tools/friction_report.py --since 2026-11-07`. Write the `make check` and
  `make unit` rows (runs, median, p90, total) into check-speed's "Done" section, next to the old numbers.
* If the median did not drop, find the stage that eats the time (tidy with changed files, a full rebuild) and write
  a draft for it with `tools/plan.py new`. 
