# Scenarios: cut the startup that dominates once the world draw is gone

Status: solved 2026-11-03. Was: draft 2026-11-03. Needs first: [scenario-skip-world-draw](scenario-skip-world-draw.md). From
[paths-speed](paths-speed.md) (measured there).

## Problem

Without the world draw, a path level plays in ~0.4 s, but every game first spends ~2.2 s (alone; 4 s CPU, 1.3 GB
RSS) loading assets before the first tick. The 54 path levels then took 52 s at 6 parallel games and also 52 s at 12:
startup and memory bandwidth bound, not tick bound.

Startup profile (`perf`, a one-tick scenario): PNG decode (`stbi_load`, mostly zlib) 36%, MD3 meshes
(`CharacterModel::Parse`, `Md3Mesh::Load`) 19%, `AnimatedModel::Compile` (VBO upload) 7%, `GameState::Load` the rest.

## Options (the implementer picks, measures, keeps what pays)

* Several scripts per game process: `bin/game a.txt b.txt ...` runs them one after another, resetting the game state
  between them (the level, the player, the seed, the output dir) without reloading assets. `tools/run_scenarios.sh`
  hands each game a batch. Watch: state that leaks from one script into the next (globals, the clock, the journal,
  `Scenario` runner fields); each script must give the same `result.txt` as when run alone.
* Skip what a blind script never uses: no texture decode or upload when the script takes no screenshot (keep the
  sizes, hitboxes and anything the simulation reads from model files). Careful: `ModelInfo` and clip lengths come from
  the MD3s, the simulation may need them.
* Faster PNG loading (decoded texture cache, or a lighter decoder), if the above are not enough.

## Check

* `make paths` scenario phase wall time before and after (`SCENARIO_TIMES`, docs/performance-testing.md). Goal: under
  25 s from ~52 s. Every level's `result.txt` the same as before.
* `make test` still passes, screenshots unchanged.

## Result (2026-11-03)

Option 1, as forks: `bin/game a.txt b.txt ...` (`Scenario::loadBatch`) loads the assets once, then runs each script
in a `fork()` of the loaded game, one after another. A fork starts from the exact state a lone run's first tick sees,
so no reset code and nothing leaks between scripts. The parent closes the audio device first (`Audio::CloseDevice`):
its thread does not live in a fork and must hold no lock there. A fork never touches GL, so it never draws: a script
that comes to a frame a lone run draws (a screen other than the gameplay's; the menus' `Draw()` keeps UI state the
next key reads) exits 4 there, and `tools/run_scenarios.sh` runs it again alone after the batches. Only scripts
without a `screenshot` go into batches (headless only), as many batches as the job cap, longest scripts spread first.
Options 2 and 3 (skip the texture decode, faster PNG) not needed.

* `make paths` scenario phase: ~56 s -> ~15 s (whole target 60 s -> 14.7 s). All 54 `result.txt` byte for byte the
  same (27 still fail: [paths-failures](../paths-failures.md)).
* The 147 solved scenarios with their `screenshot` lines cut, each alone vs through the batches: all 147
  `result.txt` the same (120 pass either way). Before the exit-4 rule `keyboard_menus` differed.
* `make test`: passes, 20.7 s as before (its 3 scenarios take screenshots, run alone); its 12 screenshots identical.
