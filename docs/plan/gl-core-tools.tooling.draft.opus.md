# Port the editor and model viewer to SDL3 and the GL core path

Status: draft 2026-11-18, size M, needs [gl-core-foundation](gl-core-foundation.feature.draft.opus.md), [sdl3-window-input](sdl3-window-input.feature.md) first.

## Problem

`bin/editor` (`tools/editor/`) and `bin/model-viewer` (`tools/model-viewer/`) still use GLUT, GLU
(`gluPerspective` in `viewer.cpp`) and immediate mode. After
[sdl3-window-input](sdl3-window-input.feature.md) and
[gl-core-foundation](gl-core-foundation.feature.draft.opus.md) they are the last GLUT and GLU users.

## Plan

* Move the window and input code of the game to a small shared module (window, GL context, event pump) if the game
  plan did not already; both tools use it.
* The tools' drawing on `MatrixStack`, `Batch` and the default shaders; the model viewer draws models with the
  game's model code, as now.
* The model viewer's screenshot (`glReadPixels`) keeps working; its HUD on the ported font.
* makefile: drop `-lglut -lGLU` (`GL_LIBS`) everywhere; [development.md](../development.md) dependencies without
  GLUT; [modeling.md](../modeling.md) if it names a GLUT key or flag.
* Check by hand: open a level in the editor, paint, save; open a model in the viewer, play its clips, take a
  screenshot. Tooling only: no play-test needed.
