# Scenarios: no world draw in frames without a screenshot

Status: solved 2026-11-03. Was: draft 2026-11-03, needs nothing first. From [paths-speed](paths-speed.md) (measured there).

## Problem

`Scenario::tick` (`src/test/scenario.cpp`) runs `Draw()` every tick. In frames without a screenshot ("blind") it
cuts the fill to one pixel with a scissor, but the vertex work stays: llvmpipe still transforms every animated model
(`AnimatedModel::Show` was 63% of a path run's profile, `Font::print` 11%). The game logic is cheap.

Measured on lvl46's path script (26312 ticks, alone, `LP_NUM_THREADS=1`): 110-123 s with the draw, 2.6 s without
(of which 2.2 s are startup). The results (`result.txt`) of all 54 path levels were byte for byte the same with and
without the draw. The whole `make paths` scenario phase went from 1395 s to 52 s.

## Change

* In blind frames on the gameplay screen (`Screen::Gameplay`, also `Screen::Death`), skip `Draw()` altogether (no
  swap either). Keep drawing the other screens (menu, inventory, riddle, map, journal, skills): they advance their own
  animations in `Draw` (the inventory's turntable), per the comment in `Scenario::tick`.
* `SCENARIO_DRAW_ALL=1` (HEADLESS=0) still draws every frame in full.
* Frames with a pending screenshot draw as today.
* Update the comment in `Scenario::tick` and the runner notes in docs/testing.md.

## Check

1. The world draw keeps state from frame to frame in a few places (MotionFx easing, HUD bars, particles?). Run
   `make test` before and after, then compare every screenshot in `tests/out/*/` pixel by pixel (Python and PIL, or
   ImageMagick `compare -metric AE`). If any differ, draw the world in blind frames only for the scripts that take a
   screenshot, and skip it in the others (the path scripts take none). Report which scenarios differed.
2. `make paths`: the same pass/fail per level as before (identical result files), and the
   wall time of the scenario phase, before and after (`SCENARIO_TIMES`, docs/performance-testing.md). Goal: under
   60 s, from ~1400 s.
3. `make test` wall time before and after (a secondary gain).

## Result (2026-11-03)

* Check 1: skipping the draw in every script changed 2 screenshots of tomb_curse (`004_opened`, `005_jars`, ~6400
  pixels each; deterministic, so real state in Draw). Fallback applied: scripts with a `screenshot` keep the blind draw
  (`Runner::hasScreenshots`); with it all 12 screenshots of `make test` are pixel-identical to before.
* Check 2: `make paths` result files identical (27 of 54 fail before and after, see paths-failures). Scenario phase
  1278 s (whole `make paths` 21m18) -> ~55 s (whole 1m00 with the build).
* Check 3: `make test` 21.1 s -> 20.4 s (only 3 scenarios run, all with screenshots: no gain expected).
