# Dungeon Crawl: A game for Linux, Windows and possibly OSX

My bachelors degree work in 2011.
This is a 2.5D side-scroller game.

## Requirements to compile:

* Make
* GCC
* OpenGL
* GLUT
* SDL
* SDL Mixer

PNG loading uses the bundled single-header [stb_image](https://github.com/nothings/stb) (`external/stb/stb_image.h`), no extra library needed.

## Build and run

* Game: `make`, then `./Play` (or `./game` from the repo root; asset paths are relative).
* Level editor: `make editor`, `make run-editor`.
* Model viewer: `make model-viewer`, `make run-model-viewer ARGS="Models/monsters/anubis.md3"`.

## Assets

* `Models/<category>/` - Quake 3 MD3 models (`characters`, `monsters`, `items`, `props`, `traps`, `decorations`, `ladders`). Monsters use `<name>.md3` (walk), `<name>_att.md3` (attack), `<name>_die.md3` (death).
* `Textures/<category>/`, `Fonts/` - PNG textures (24-bit RGB; alpha is supported); same categories as `Models/` plus `ui`, `dungeon`, `effects`. `Textures/Shader.txt` and `ShaderD.txt` are toon-shading ramps.
* `Sounds/` - WAV effects, OGG soundtrack. `Levels/` - level files. `Saves/` - save games.

Models are rebuilt procedurally with Blender Python scripts in `tools/blender/`; see [docs/remodeling.md](docs/remodeling.md).

## Level editor

Tile types, attributes, workflow and file format: [DungeonEditor/readme.md](DungeonEditor/readme.md).

## Code quality tools

- Format source files: `make format`
- Run static analysis (clang-tidy): `make tidy`

`clang-tidy` uses the project configuration from `.clang-tidy`.
