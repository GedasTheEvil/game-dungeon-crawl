# Make `make paths` faster: profile, then draft the fixes

Status: draft 2026-11-03, needs nothing first. From the user. An analysis plan: its result is one or more new drafts
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
