# make check: a header change rebuilds everything (ccache or fewer includes)

Status: solved 2026-11-16. Was: draft 2026-11-16, size M, needs nothing first.

## Problem

[check-speed-week-later](check-speed-week-later.tooling.md): after the tidy cache and tidy fan-out fixes the
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

## Result

* makefile: `CCACHE` (found with `command -v ccache`, empty when absent) in front of `$(CXX)` in the three compile
  rules; `make NO_CCACHE=1` opts out. Note in `docs/development.md`.
* Measured (idle-ish, 255 objects include gameplay_config.h):
  * header touched, cache empty: 189 s, 0/255 hits (fills the cache).
  * comment appended to the header: 142 s, 255/255 hits (ccache hashes the preprocessed text, comments drop out).
  * reverted: 143 s, 255/255 hits. The 142 s that remains is preprocessing 255 files, links and archives under the load gate.
  * A real semantic edit still misses all 255 (about 189 s).
* Top fan-out (315 translation units): gameplay_config.h 257, core/paths.h 248, level/level.h 247, damage.h,
  poison.h, rgb.h, tier_drain.h 239, decor.h 238, skills.h 228, curses.h 227, items.h 226, terrain.h 222. Too wide
  to split in this task: [check-header-fanout](../check-header-fanout.tooling.draft.sonet.md).
* Deferred: friction_report after a few days: [check-friction-after-ccache](../check-friction-after-ccache.tooling.draft.sonet.md) (not before 2026-11-21).
