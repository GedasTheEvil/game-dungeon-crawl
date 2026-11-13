# Each unit test case starts on its own virtual clock at 0

Status: draft 2026-11-13, size S, needs nothing first.

## Problem

The game clock is one global (`gVirtualClock`, `gVirtualMs` in `src/core/timer.cpp:5-6`). The unit tests switch it
to virtual mode and advance it, but nothing ever sets it back: each test case starts at whatever time the cases run
before it left, and on the real clock if none of them switched it to virtual. Since `make unit` runs the cases in
shards, in a seeded random order (`tools/unit_shards.sh`, plan check-speed), "the cases before" depend on the shard
count and on every new test.

The tests already work round it by hand, each its own way:

* `tests/unit/skills_test.cpp:58-69` and `tests/unit/water_blessing_test.cpp:157-167`: "The virtual clock is global
  and the spike traps' phase follows it (wall_spikes_test): give it back whole cycles": they advance the clock to the
  next multiple of 48 s on the way out.
* `tests/unit/wall_spikes_test.cpp:40`: `waitFor` waits up to a minute for a spike state "whatever the clock's phase
  (the virtual clock runs on from the tests before)".
* `tests/unit/ring_accessories_test.cpp:271`: `GameClock::enableVirtual(); // alone in its shard: advance moves nothing
  on the real clock`: a test that failed once the shards ran it first.
* `enableVirtual()` sprinkled per case or helper: `status_list_test.cpp:12, 51, 72, 85`, `player_stats_test.cpp:12`,
  `curses_test.cpp:12` (`freshStats`), `SimWorld`'s constructor (`tests/unit/sim_world.h:53`).

A test that forgets both a `SimWorld` and `enableVirtual` and reads a `Timer` (`PlayerStats` holds two,
`src/level/player_stats.h:47-48`) runs on wall time in one order and on virtual time in another: a flaky test that
passes or fails depending on what ran before it in its shard.

## Plan

1. `src/core/timer.{h,cpp}`: `GameClock::resetVirtual(int ms)`: virtual mode on, the time set to `ms`.
2. `tests/unit/main.cpp`: a doctest listener (`REGISTER_LISTENER`, `test_case_start`) that calls
   `GameClock::resetVirtual(START_MS)` before every test case. `START_MS`: a fixed value well above 0 (say 100000),
   so no "0 means unset" field (`breakMs <= 0`, `blazeReadyMs = 0`) mistakes a moment at start for "never".
3. Drop the workarounds listed above (the whole-cycle advances, the extra `enableVirtual` calls, the "whatever the
   phase" comment); keep `waitFor` if it still reads clearer.
4. `SimWorld` keeps its `enableVirtual()` only if the balance sim (`tools/balance/balance.cpp`, no doctest) needs it.

## Risk

Low. A test that relied on the time the cases before it left fails at once and every run, not now and then: fix it
to set what it needs.

## Verify

* `make check`.
* `bin/unit --order-by=rand --rand-seed=N` for a few seeds, and `tools/unit_shards.sh bin/unit 1`, `... 3`, `... 6`:
  all pass.
