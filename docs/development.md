# Development

## Requirements

* Make
* GCC
* OpenGL
* GLUT
* SDL
* SDL Mixer

PNG loading uses the bundled single-header [stb_image](https://github.com/nothings/stb) (`external/stb/stb_image.h`), no extra library needed.

## Build and run

Asset paths are relative, so run the programs from the directory given here.

* Game: `make`, then `./Play` (or `./game` from the repo root).
* Level editor: `make editor`, `make run-editor`. Runs from `dungeon-editor/`, see [dungeon-editor/readme.md](../dungeon-editor/readme.md).
* Model viewer: `make model-viewer`, `make run-model-viewer ARGS="models/monsters/anubis.md3"`.
* Level tools: `make level-tools`, then `./levelcheck levels/lvl*` (validate, rank by difficulty) and
  `./levelgen --seed 1 --difficulty 5 OUT` (random level). See [levels.md](levels.md).

## Project layout

* `src/` - game code: `core`, `graphics`, `entities`, `world`, `ui`, `input`, `state`, `test` (scenario runner).
* `dungeon-editor/` - level editor. `saved/` holds its work-in-progress levels (not tracked by git).
* `model-viewer/` - MD3 model viewer.
* `tools/` - level tools (`level/`), Blender model scripts (`blender/`), sound and texture generators (`audio/`, `textures/`), scenario runner script.
* `tests/` - scenario scripts (`scenarios/`), test levels (`levels/`), results (`out/`, not tracked).
* `external/` - third-party headers.

## Assets

* `models/<category>/` - Quake 3 MD3 models (`characters`, `monsters`, `items`, `props`, `traps`, `decorations`, `ladders`, `mechanisms`). Monsters use `<name>.md3` (move), `<name>_att.md3` (attack), `<name>_die.md3` (death), optional `<name>_idle.md3` (idle).
* `textures/<category>/`, `fonts/` - PNG textures (24-bit RGB; alpha is supported); same categories as `models/` plus `ui`, `dungeon`, `effects`. `textures/Shader.txt` and `ShaderD.txt` are toon-shading ramps.
* `sounds/` - WAV effects, OGG soundtrack (not tracked by git). `tools/audio/*.py` synthesize the jump, rat, bat, key, gate, lever and rock sounds.
* `levels/` - campaign levels, edited with the level editor.
* `saves/` - save games (not tracked by git).

Models are rebuilt procedurally with Blender Python scripts in `tools/blender/`; see [remodeling.md](remodeling.md).

## Tests

Scenario scripts in `tests/scenarios/` drive the game and take screenshots. `make test` runs all, `make test SCENARIO=path` runs one.
See [testing.md](testing.md).

## Code quality tools

* Format source files: `make format`
* Run static analysis (clang-tidy): `make tidy`

`clang-tidy` uses the project configuration from `.clang-tidy`.
