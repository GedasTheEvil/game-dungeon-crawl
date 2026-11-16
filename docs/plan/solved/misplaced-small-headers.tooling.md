# Move small headers to the layer that owns them

Status: solved 2026-11-16. Was: draft 2026-11-16, size S, needs nothing first.

## Problem

The architecture review (2026-11-16, smell 2): most include cycles between the app directories came from a few
small headers in the wrong place, not from real two-way coupling. ui, input and state included `graphics/` only for
the GLUT wrapper `gl_includes.h`; graphics included `game/` only for `Sound`, and `state/` for `Assets` (the loaded
models, textures and sounds: drawing resources); graphics included `input/` only for `keyCapOf`.

## Plan

* `gl_includes.h` to `render/`.
* `sound.*` to a new `audio/` directory (SDL_mixer; the render library has no SDL).
* `assets.*` and `loading_text.h` to `graphics/`.
* `keyCapOf` and `equipKeysCap` to `level/bindings.*`, taking the `Bindings`.
* Left (real coupling, later plans): graphics draws the ui screens, ui reads and writes `Game()`, the game loop calls
  the input module.
