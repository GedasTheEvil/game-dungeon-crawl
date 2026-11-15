# Run make check tidy and unit stages at the same time

Status: draft 2026-11-15, size S, needs nothing first.

## Problem

`tools/check_stages.sh` runs build, plan, format, tidy and unit one after another. Tidy and unit do not depend on
each other: both need only the build (unit) and the formatted sources (tidy). Unit is median 10 s, p90 31 s, up to
135 s (`.git/check-stages.log`); that time is added on top of tidy's today.

## Plan

* In `tools/check_stages.sh`, after the format stage, start the tidy and the unit stage together (two background
  `bash -c` runs, `wait` on both). Both already go through the load gate (`tools/load_gate.sh`), so the CPU cap holds.
* Each stage's output goes to its own temp file and is printed after both end, tidy first, then unit, so the
  output does not interleave.
* Each stage keeps its own time in the log line (`tidy=`, `unit=`); `total=` is the wall time. A failure of either
  is `FAIL:tidy` or `FAIL:unit`; both failed: `FAIL:tidy` (the first in the old order). Print both outputs either way.
* A failed earlier stage (build, plan, format) still skips both, as now.
* Update the comment above `check:` in the makefile and docs/development.md (Code quality tools: "in order").
* Check: `make check` on a change with a unit test and a tidy warning (both fail and both print), and a clean one.
