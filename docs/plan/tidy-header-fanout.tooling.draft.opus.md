# Tidy a changed header once, not in every source that includes it

Status: draft 2026-11-15, size M, needs nothing first.

## Problem

`make tidy` (and so `make check`) tidies every source that includes a changed header, also through other headers
(`tools/tidy.sh changed`, the build's `.d` files). A header like `level/monster_kinds.h` reaches about 130 sources:
one such run took 534 s of a 689 s `make check` (`.git/check-stages.log`, 2026-11-15). Over the logged runs tidy is
61% of the mean `make check` time (median 22 s, p90 533 s; `tools/friction_report.py`). The cache does not help on
the first run: its key holds every included header, so one header edit misses all of them.

The warnings tidy finds are worth it (mostly `bugprone-unchecked-optional-access`, `bugprone-integer-division`), so
the fix is fewer sources per header, not fewer checks. A header's own warnings show in any one source that includes
it (`HeaderFilterRegex`), so one source per header finds them.

## Plan

* `tools/tidy.sh changed`: tidy the changed sources, plus, for each changed header that no changed source already
  includes, one source that includes it: its own `.cpp` (`foo.h` → `foo.cpp`) when there is one, else the one with
  the fewest includes (the fastest). Print which source stands in for which header.
* `TIDY_FANOUT=all` keeps today's behaviour (every including source).
* `make tidy-all` stays the full run. Decide where the full fan-out still runs, so a warning in a caller (a header
  change that makes a caller's code wrong, e.g. a return type that became `std::optional`) is still caught before
  long. Options: `make tidy-all` before a push from the main folder (Opus pushes after each commit), or a
  `TIDY_FANOUT=all` run when a worktree task merges into master. Recommended: before the push; it is cached, so only
  the sources the change reached run.
* Update `make tidy`'s comment in the makefile, tools/tidy.sh's header and docs/development.md (Code quality tools).
* Measure: a header-only edit to `level/monster_kinds.h`, `make check` before and after (`check-stages.log`'s tidy
  column, `tidied=`). Report both numbers in the commit.

## Risk

A caller's new warning is found later (at the full run) instead of in the same `make check`. The changed callers
are tidied anyway; only untouched callers wait.
