# make check: a header change rebuilds everything (ccache or fewer includes)

Status: draft 2026-11-16, size M, needs nothing first.

## Problem

[check-speed-week-later](solved/check-speed-week-later.tooling.md): after the tidy cache and tidy fan-out fixes the
build is the biggest stage of `make check` (51% of the mean total; median 31 s but p90 600 s, max 1126 s, 22 runs after
the fan-out fix). A run after a header change rebuilds every includer (a full build is about 226 s idle; more under
load at the gate), and there is no compiler cache (`ccache` is not installed).

## Plan

* Ask the user to install `ccache` (`apt install ccache`); then use it as the compiler launcher in `makefile` (opt
  out with an env var), so a rebuild after a branch switch, a merge or a reverted header edit hits the cache.
* Measure: a `gameplay_config.h` touch, cold and then reverted; check the cache hit rate and the wall time.
* If a header still fans out too far, list the top headers by includer count (`tools/where.py` or the `.d` files) and
  split or forward-declare the worst one.
* Measure again with `tools/friction_report.py` after a few days (build median, p90).
