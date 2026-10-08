# Heavy make targets: lighter on the system

Status: done 2026-10-08 (tooling, no gameplay change). From the user: the full scenario suite (`make test`) still
made the system lag badly.

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
* Measure every run (decided 2026-10-08), where it can be done: each heavy target ends with a summary of its peaks,
  as `run_scenarios.sh` prints today (peak jobs at once, min idle cores, min free memory), plus min free swap and the
  wall time. Tidy and the build get the same line. Then each run can be compared to the baseline without a separate
  measuring run.

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


## Done

* `tools/load_gate.sh`: one gate for the scenario games (`run_scenarios.sh`, so `make test` and `make paths`), each
  clang-tidy of `make tidy` / `tidy-fix`, and every compile and link (also under `make -j`). Slot files under
  `GATE_DIR`, held by flock while a job runs, so all runs at once share the cap. `JOBS` default
  `(nproc - RESERVE_CORES) * 4 / 5` (9 of 16), plus the idle-core, `MemAvailable` and `MIN_SWAP_FREE_MB` (4096)
  floors. One always runs. A game started less than `GATE_LOAD_S` (15 s) ago counts as `GATE_LOAD_MB` (1500) still to
  come. Jobs run on the last `GATE_CPUS` cores (default the job cap): the desktop keeps the first ones however much a
  job grows after its start. `TIDY_JOBS` is gone.
* Each heavy target ends with `== <label> load: peak, held back, min idle, min free (and at the start), min free swap,
  wall time`: `make test` / `paths` (scenarios), `make tidy` / `tidy-fix`, the build (`make`, `make unit`, before
  `make paths`).
* Memory per game, 2971 -> 1513 MB peak RSS: the models kept every frame's vertex data after compiling it into display
  lists (which hold their own copy), and grew it by push_back (up to 2x). Now the arrays are sized up front and
  `AnimatedModel::Compile` drops frames 1.. (the player keeps them for `Player::Fist`).

## Measured (full suite, 101 scenarios, independent 1 s sampler)

| | baseline (HEAD 4eb929e) | after |
|---|---|---|
| games at once, peak | 11 (10 on a rerun) | 9 |
| memory taken by the suite (start - min `MemAvailable`) | 18.8 GB, 17.5 GB | 11.5-12.3 GB (-35%) |
| min free swap | 6.2 GB, 3.4 GB | 7.6-9.7 GB, the gate never needed |
| suite CPU (game + Xvfb), peak | ~10-11 cores, uncapped | <= 9 cores, pinned |
| wall time | 433-505 s | 445-529 s |

System-wide CPU in use still peaks at 13-15 of 16 cores: the rest is the desktop (browser, IDE) on the free cores.
`make -j16` from clean: 57 s, peak 6 compiles. `make tidy`: 153 s (under a minute before, 12 at once uncapped).
