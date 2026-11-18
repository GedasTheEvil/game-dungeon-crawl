# Port the editor and model viewer to SDL3 and the GL core path

Status: solved 2026-11-18. Was: draft 2026-11-18, size M, needs [gl-core-foundation](../gl-core-foundation.feature.md), [sdl3-window-input](../sdl3-window-input.feature.md) first.

## Problem

`bin/editor` (`tools/editor/`) and `bin/model-viewer` (`tools/model-viewer/`) still use GLUT, GLU
(`gluPerspective` in `viewer.cpp`) and immediate mode. After
[sdl3-window-input](../sdl3-window-input.feature.md) and
[gl-core-foundation](../gl-core-foundation.feature.md) they are the last GLUT and GLU users.

## Plan

* Move the window and input code of the game to a small shared module (window, GL context, event pump) if the game
  plan did not already; both tools use it.
* The tools' drawing on `MatrixStack`, `Batch` and the default shaders; the model viewer draws models with the
  game's model code, as now.
* The model viewer's screenshot (`glReadPixels`) keeps working; its HUD on the ported font.
* makefile: drop `-lglut -lGLU` (`GL_LIBS`) everywhere; [development.md](../../development.md) dependencies without
  GLUT; [modeling.md](../../modeling.md) if it names a GLUT key or flag.
* Check by hand: open a level in the editor, paint, save; open a model in the viewer, play its clips, take a
  screenshot. Tooling only: no play-test needed.

## Done

* `GlWindow` (`src/game/gl_window.*`): the SDL window and its GL 3.3 compatibility context, GL loaded with
  `SDL_GL_GetProcAddress`. The game's `Window::open` and both tools use it; the tools link `gl_window.o` alone (no
  `Game()`), as the unit tests link `keymap.o`. The event pump stays per program: the game's input is `Game()`-bound,
  each tool has a short SDL loop of its own (the editor: keys, text input for its fields, buttons, wheel notches as
  buttons; the viewer: keys, drag, Escape quits).
* Viewer: projection, turn and loop bar on `gProj`, `gMv` and a `Batch`; the game's model, lighting and stats panel
  get the matrices through `GlLegacy::syncMatrices()`; the glass tint and texturing through two new bridge functions
  (`GlLegacy::colour`, `texturing`). `--shot` opens on X11 and gives the same pictures as before (snapdiff: at most 1
  off). Editor: its canvas projection on `gProj`, synced for the UI code. Legacy-GL count of both tools: 0.
* makefile: no `-lglut`, no `GLUT_LIBS`; development.md, modeling.md, stuck-processes.md, where-is-x.md.
* By hand under Xvfb (xdotool): the editor opened a copy of lvl3, switched modes and layers, picked, painted, zoomed,
  panned (wheel, arrows, Ctrl+wheel), typed into the name field and saved (Ctrl+S); the viewer played anubis's clips,
  cycled texture and light, sped up, turned by drag, quit on Escape; `--shot` wrote its frames. Not checked: the
  middle-button drag, a real Wayland desktop.
