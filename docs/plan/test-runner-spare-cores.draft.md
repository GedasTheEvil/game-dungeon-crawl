# Test runner: leave CPU cores free

Status: idea (2026-09-30). Not started.

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
* Update the header comment of `tools/run_scenarios.sh` and the `make test` notes in [docs/testing.md](../testing.md).
* Optional: do the same for `TIDY_JOBS` in the makefile (`make tidy` runs `xargs -P $(nproc)` clang-tidy).
* Optional: run the jobs under `nice` so the desktop stays responsive even at full width.
