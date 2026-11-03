# Make `make paths` faster: profile, then draft the fixes

Status: solved 2026-11-03. Was: draft 2026-11-03, needs nothing first. From the user. An analysis plan: its result is one or more new drafts
with the actual improvements, not code changes.

## Problem

`make paths` (makefile, target `paths`) takes very long. It runs `levelcheck --script` over the 54 campaign levels,
then plays each written scenario (`tests/out/paths/lvl*.txt`, about 6000 lines in total, up to ~200 per level) in a
real `bin/game` under Xvfb through `tools/run_scenarios.sh` and the load gate (`tools/load_gate.sh`). The game runs in
god mode, with real-time waits (`wait 4000ms`, `waittraps`, `walk to X` at walking speed).

## Task

Measure where the time goes, then write one draft per improvement worth doing (`docs/plan/<slug>.draft.<model>.md`,
tagged by size, docs/plans.md). Don't implement the improvements here.

1. Wall time of the whole run, split into: build, `levelcheck --script`, and the scenario phase.
2. Per level: wall time, game ticks played, and the time spent in startup (asset load, level load) vs. playing.
   Sort the levels by time; find what the slow ones share (long waits, trap rhythms, many steps, failures/timeouts).
3. Parallelism: how many games run at once (`load_gate.sh jobs`, `GATE_LOAD_S`, `GATE_LOAD_MB`, memory), how long
   games wait at the gate, CPU use over the run. Is the machine busy or idle most of the time?
4. Per tick cost inside one game: profile a slow level (`perf`, or the game's own timers) with rendering on.
   Does the scenario runner draw every frame, and does it need to? Is the game capped to real time (vsync, frame
   limiter) or does it run ticks as fast as it can?
5. Startup cost: what a game loads before the first tick (models, textures, sounds, shaders) and whether a path run
   needs it all.

Ideas to weigh (check them against the numbers, don't assume):

* Run ticks faster than real time in scenarios: a fixed-step loop without sleeping, `wait Nms` as N ticks.
* Skip or cut rendering in path runs (draw no frames, or one frame in N; screenshots need none here).
* Several levels per game process (one startup), or a lighter startup when no window output is needed.
* Shorter scripts from `levelcheck --script`: fewer or tighter waits where the checker knows the exact timing.
* Better scheduling: the slowest levels first, a higher job cap if CPU and memory allow.
* Only the levels whose file or the checker/movement code changed (an opt-in quick mode).

## Result

* The new drafts, each with the measured gain it aims for and how to check it (before/after wall time of
  `make paths`). Each keeps `make paths` checking the same thing: the checker's movement model against the real
  physics, on every campaign level.
* A short note here (Findings section) with the numbers, then `tools/plan.py move paths-speed solved` (a tooling
  plan: no play-test needed).

## Findings (2026-11-03)

Measured on the 16-thread laptop (Ryzen 7 5850U, 8 cores with SMT, 15 W), `make paths` at c0af1920, with the
instrumented runner that became `SCENARIO_TIMES` and `tools/profile_game.sh`
([performance-testing.md](../../performance-testing.md)).

1. Phases: build 1.8 s (incremental), `levelcheck --script` 0.05 s, scenarios 1395 s (23 min). Only the scenario
   phase matters.
2. Per level: 54 to 400 s (median 230 s), 3878 to 36966 ticks, 1.03 M ticks in total. 9.7 to 15.9 ms a tick in the
   parallel run, slower than the 16 ms of real time only by a little. The time follows the tick count; the slow levels
   are the long ones (lvl46, 40, 42, 41, 23). Startup is 2.2 s a game alone, under 1% of the run.
3. Parallelism: the gate's cap of 9 games was reached (held back 38 s in total, waits at the gate 1391 s summed),
   CPU 66% user, 28% idle (the 4 reserved cores and some), run queue 14.8 on the 9 gated cores: each game keeps more
   than one thread busy (the game, llvmpipe, Xvfb). lvl46 alone: 123 s; in the full run: 400 s. Nine games at once
   gave only ~2.6x the throughput of one: a higher cap would not help on this machine.
4. Per tick: the game does not sleep between ticks (virtual clock, `tickDelayMs` 0) and fills one pixel in blind
   frames, but `Draw()` still runs: `AnimatedModel::Show` 63%, `Font::print` 11% of lvl46's profile, ~half of all
   time inside llvmpipe (vertex work, which the scissor does not cut). With the gameplay draw skipped in blind frames
   (a hack on a copy): lvl46 alone 2.6 s instead of 110-123 s, all 54 `result.txt` byte for byte the same (ticks
   included), the whole phase 52 s at 6 games and 52 s at 12.
5. Startup: PNG decode 36%, MD3 meshes 19%, VBO upload 7% of ~2.2 s (1.3 GB RSS). Once the draw is gone, it is ~85%
   of each game.

The ideas weighed:

* Faster than real time: already so; the draw is what made a tick cost ~10 ms. Draft
  [scenario-skip-world-draw](scenario-skip-world-draw.md): 1395 s -> ~52 s.
* Several levels per process, lighter startup: worth it after the draw fix. Draft
  [scenario-light-startup](scenario-light-startup.md): ~52 s -> under 25 s.
* Shorter scripts (2014 `wait`/`waittraps` lines, a 4 s stamina wait before every jump): no draft. Without the draw
  a tick costs ~0.015 ms; all the waits together are a few seconds.
* Scheduling (slowest first, a higher job cap): no draft. A higher cap gave nothing (6 and 12 games: the same 52 s).
* An opt-in quick mode (changed levels only): no draft; the full run will be under a minute.

Not about speed: 27 of the 54 levels fail `make paths` (walks blocked, a missed `killboss`), on 21335b18 (before the
sub-cell hitbox commits) too, checked on lvl8 and lvl21.
