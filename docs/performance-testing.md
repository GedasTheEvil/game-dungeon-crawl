# Performance testing

How to find where the time goes in a scenario run (`make paths`, `make test`) or in one game. First used by
[paths-speed](plan/solved/paths-speed.md); its Findings section has the reference numbers.

## Setup

`perf` needs the kernel to allow it (once per boot, or in `/etc/sysctl.d/`):

```
kernel.perf_event_paranoid = 1
kernel.kptr_restrict = 0
```

Missing: `sudo apt install linux-tools-generic`. Every agent run of `bin/game` stays silent and bounded (AGENTS.md):
the tools below do that.

## Tools

* `SCENARIO_TIMES=file tools/run_scenarios.sh ...` (also through `make`, e.g. `SCENARIO_TIMES=/tmp/t.tsv make paths`):
  a header row, then one tab-separated line per scenario, slowest first: name, seconds waited at the load gate, seconds
  run (startup and play; a batched script's has no startup, its wait is its batch's), the last tick, PASS/FAIL. The run
  prints the sums, the mean ms per tick and the load gate's report
  (peak parallel games, seconds held back, minimum idle cores and free memory).
* `tools/profile_game.sh [-t s] [-n lines] [-o file.perf] scenario.txt`: one game under `perf record -g` (Xvfb,
  `--no-sound`, a time limit, one llvmpipe thread). Prints the wall time, the result line, the share per library and
  the top functions, self and with callees. Dig further with `perf report -i file.perf`; callers of one function:
  `perf report -i file.perf --children --stdio -G -S '<symbol>'`.

## Recipes

* Whole run by phase: time the build, `levelcheck --script` and the scenario phase separately (the `paths` target in
  the makefile is three commands; run them by hand with `date +%s.%N` around each).
* Startup alone: a scenario of `seed 1`, `god`, `level content/levels/lvl1`, `wait 1`. Its wall time is what every
  game pays before the first tick; subtract it from a level's run time to get the play time.
* Per tick cost: (run time - startup) / ticks. Compare a game alone with the same game in a full parallel run: the
  ratio is the contention cost.
* CPU over a run: `vmstat -n 1 >/tmp/vmstat.log &` during the run; average the `us`, `sy`, `id` and `r` (run queue)
  columns. A run queue above the gate's cores (`GATE_CPUS`, default the job cap) means each game uses more than one
  thread (the game, llvmpipe, Xvfb).
* A change that should not alter behaviour: diff every `tests/out/<name>/result.txt` before and after (tick numbers
  included). Copy the old results aside first, the next run overwrites them.
* An experiment on a clean copy without touching the tree: `git archive HEAD | tar -x -C /tmp/x`, change and build
  there (`make -j16 bin/game`), run its `bin/game` from `/tmp/x` (it reads `content/` and `media/` from its own root).
  Mind the env var tests: `VAR= cmd` sets VAR to the empty string, `std::getenv` then returns non-null.

## Where the time went (2026-11-03, 16-thread laptop, Ryzen 7 5850U)

* Drawing: software GL (llvmpipe under Xvfb) does the vertex work of every model even when the fill is scissored to
  one pixel; it was ~97% of a path level's run.
* Startup: ~2.2 s a game alone: PNG decode 36%, MD3 meshes 19%, VBO upload 7%. Batches pay it once per batch
  ([scenario-light-startup](plan/solved/scenario-light-startup.md)): the 54 path levels' scenario phase went from ~56 s to
  ~15 s.
* Parallel games: 9 at once gave only ~2.6x the throughput of one (SMT, a 15 W CPU, three threads per game).
