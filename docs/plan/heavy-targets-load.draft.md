# Heavy make targets: lighter on the system

Status: draft 2026-10-08, from the user: the full scenario suite (`make test`) still makes the system lag badly.

## Goal

* Cut the suite's resource use by 20% compared to the last runs (decided 2026-10-08): the baseline is the last full
  run's summary, peak 11 games at once, min idle 1.1/16 cores, min free 755 MB. Target: at most 9 games at once (20%
  fewer), and CPU and memory in use at the peak 20% below that run's. Measure on a fresh baseline run first, then
  after the change.
* Start no new game process while the swap has less than 4 GB free: sleep and sample again instead.
* Scope (decided 2026-10-08): every heavy process, not only the scenario suite. `make test`, `make paths` (the same
  runner), `make tidy` / `tidy-fix` (`xargs -P $(TIDY_JOBS)`, nproc - 4 clang-tidy processes, makefile), and any other
  parallel target (the build, if run with `-j`). One shared gate for all of them: the same job cap, the same idle-core
  and free-memory floor, the same swap check, ideally one helper script both the runner and the makefile call.

## Today

`tools/run_scenarios.sh` is load-aware already: a new game starts only while `RESERVE_CORES` cores (default 4) sit
idle and `MIN_FREE_MB` (default 4096) of memory is available (`MemAvailable`), at most `JOBS` at a time (default
`nproc - RESERVE_CORES`). One always runs. Games run niced, one llvmpipe thread each (`LP_NUM_THREADS=1`), under one
Xvfb per run. The last full run reported a peak of 11/12 games at once, held back 105 s, min idle 1.1/16 cores,
min free 755 MB: the memory floor was crossed while games were running.

## Ideas

* Fewer at once: default `JOBS` 20% lower (`(nproc - RESERVE_CORES) * 4 / 5`), or reserve more cores.
* Swap gate: read `SwapFree` from `/proc/meminfo`; below 4096 MB, sleep and sample again before each start
  (configurable, e.g. `MIN_SWAP_FREE_MB`). Rule out "one always runs" for this gate only while games are running.
* Memory per game: check what a game holds (all textures and models load at start; a scenario on one level needs
  few) and whether `MemAvailable` drops below the floor between samples because the new game is still loading
  (the 1 s wait after a start may be too short).
* Measure before and after: the runner's summary line (peak, held back, min idle, min free) plus the wall time;
  "20% less" as the peak games and memory, with the wall time allowed to grow.

