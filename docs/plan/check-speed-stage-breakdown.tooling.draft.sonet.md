# make check: which stage eats the median

Status: draft 2026-11-15, size S, needs nothing first.

## Problem

The [recheck](solved/check-speed.tooling.md) of 2026-11-15 (`tools/friction_report.py --since 2026-11-07`): `make check`
median 124 s (before: 120 s), p90 439 s (600 s), 150 runs, 6.7 h. `make tidy` alone: median 178 s, p90 521 s. `make
unit`: median 39 s, p90 180 s (a shard run was 8 s on an idle machine). Agents' runs stay slow, likely from tidy (a
header change tidies many files) and from builds after a header change, plus waiting for the gate while other tasks run.

## Plan

* Time each stage of `make check` (build, plan lint, format, tidy, unit) in a log line per run, with the number of
  changed files and the gate wait; read it from `friction_report.py`.
* Fix the biggest stage: for tidy, a cache per file hash or tidy only the files that include a changed header
  once; for the build, check whether a header change forces a full rebuild.
* Measure again a week later.
