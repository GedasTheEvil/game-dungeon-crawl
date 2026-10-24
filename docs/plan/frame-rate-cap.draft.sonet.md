# Frame rate cap

Status: draft 2026-10-24, from the user (with [model-frame-interpolation](model-frame-interpolation.draft.opus.md):
"if it can work for both, that's OK as well"). Needs nothing first.

## Goal

An option for how often the whole game is drawn, to save power and heat (laptops, fans). Today
`Idle()` (`src/input/input.cpp`) asks for a redraw all the time and nothing sets vsync, so the rate is uncapped or
the driver's default.

## How

* Options > Display, "Frame rate": `Monitor` (vsync, default), `30`, `60`, `Unlimited`. No 24: the camera judders
  at that rate; a stop-motion look for the models is the animation rate option of model-frame-interpolation.
* `Settings::Display` (`src/state/settings_ini.h`), saved in saves/settings.ini; add the row to docs/settings.md.
  Scenario runs keep the default and draw from their tick as now (`noopDisplay`, `src/core/game.cpp`): no change there.
* `Monitor`: swap interval 1 (`glXSwapIntervalEXT`, or `glXSwapIntervalMESA` / `glXSwapIntervalSGI`, whichever
  the driver has), no sleep. Other choices: swap interval 0. `30` / `60`: `Idle()` posts a redraw only when
  `1000 / rate` ms passed since the last one, and sleeps the rest (short sleeps, so input stays responsive).
* The game logic tick (`UPDATE_TICK_MS`) stays as it is: the cap changes drawing only.

## Test

* Unit: the "redraw due" check for 30 and 60 (pure function of last draw time and now).
* Play-test: `bin/game --level 1`, switch the option, watch the CPU load (`top`) and the smoothness.
