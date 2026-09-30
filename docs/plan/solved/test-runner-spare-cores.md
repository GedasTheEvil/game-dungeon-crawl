# Test runner and tidy: leave CPU cores free

Status: solved (2026-09-30). Four cores stay free (`nproc - 4`, 12 of 16), not two as first planned. `nice` not added.

## Problem

`make test` runs `tools/run_scenarios.sh`, which starts `JOBS` scenarios at once, default `$(nproc)` (16 here). Each
one is a `./game` under its own Xvfb, so all cores are busy and the desktop becomes slow while the tests run.

## Idea

Default to `nproc - 2` jobs (14 of 16), at least 1. The runner already keeps at most `jobs` processes running
(`wait -n` loop), so only the default changes:

```bash
jobs=${JOBS:-$(($(nproc) - 2))}
[ "$jobs" -lt 1 ] && jobs=1
```

* `JOBS=16 make test` still uses all cores.
* Update the header comment of `tools/run_scenarios.sh` and the `make test` notes in [docs/testing.md](../../testing.md).
* Same for `make tidy`: the makefile has `TIDY_JOBS?=$(shell nproc)` (clang-tidy via `xargs -P`). Change it to
  `nproc - 2`, at least 1; `TIDY_JOBS=16 make tidy` still uses all cores.
* Optional: run the jobs under `nice` so the desktop stays responsive even at full width.
