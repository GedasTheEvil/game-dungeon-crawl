# Scenarios: cut the startup that dominates once the world draw is gone

Status: draft 2026-11-03. Needs first: [scenario-skip-world-draw](solved/scenario-skip-world-draw.md). From
[paths-speed](solved/paths-speed.md) (measured there).

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
