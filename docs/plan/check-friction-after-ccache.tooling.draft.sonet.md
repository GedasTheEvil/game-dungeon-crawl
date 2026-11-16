# Measure make check with friction_report after ccache (not before 2026-11-21)

Status: draft 2026-11-16, size S, not before 2026-11-21 (ccache landed by [check-build-header-rebuild](solved/check-build-header-rebuild.tooling.md); a few days of runs needed).

## Problem

The last point of check-build-header-rebuild: did ccache lower the build stage's median and p90 (before: median 31 s,
p90 600 s, max 1126 s, 22 runs)?

## Plan

* Run `tools/friction_report.py` and compare the build stage with the numbers above.
* Report the ccache hit rate (`ccache -s`). Note the result here, then move the plan to solved.
