# Window, input and main loop on SDL3: drop GLUT

Status: implemented 2026-11-18, waiting for the user's play-test. Was: draft 2026-11-18, size L, needs [sdl3-audio](sdl3-audio.feature.md) first.

## Problem

The window, GL context, event loop and input run on GLUT (freeglut): callbacks registered in `src/game/game.cpp`
(`glutDisplayFunc`, `glutIdleFunc`, `glutTimerFunc` for the update tick and the scenario tick, keyboard, special
keys, mouse, motion, entry), `glutWarpPointer`/`glutSetCursor` in `src/input/input.cpp`, `glutIgnoreKeyRepeat` and
`glutPostRedisplay` in `src/state/game_loop.cpp`. GLUT gives no control over the context version (needed for
[gl-core-profile](gl-core-profile.feature.draft.opus.md)), no high-DPI, no gamepad, no raw mouse motion, no
Wayland, and keys come as characters plus "special" codes.

## Plan

* One `SDL_Window` with an OpenGL context (`SDL_GL_CreateContext`). Ask for a 2.1 compatibility context for now (the
  game is still fixed-function); [gl-core-profile](gl-core-profile.feature.draft.opus.md) switches it to core.
  Depth, double buffer, alpha as now; vsync by `SDL_GL_SetSwapInterval`.
* Own main loop instead of `glutMainLoop`: poll events, run the update tick at `UPDATE_TICK_MS` from a fixed-step
  accumulator on `SDL_GetTicksNS` (replaces `glutTimerFunc`), draw, swap. `glutPostRedisplay` call sites become a
  "redraw" flag or go (the loop draws every frame). Scenario mode keeps its own tick (`Scenario::tickDelayMs`) and
  its no-op display.
* Input (`src/input/`): map SDL keycodes/scancodes to the game's actions; the current GLUT key and special-key tables
  go. Movement keys by scancode (layout independent), text-like keys (journal search, if any) by keycode. Key repeat
  from `event.key.repeat` (replaces `glutIgnoreKeyRepeat`). Mouse: buttons, motion; `SDL_WarpMouseInWindow` and
  relative mouse mode where the code warps the pointer to keep it centred; cursor show/hide by
  `SDL_ShowCursor`/`SDL_HideCursor`. Window enter/leave for `processMouseEntry`.
* Fullscreen: borderless desktop fullscreen (`SDL_SetWindowFullscreen`), resize events replace `reSizeGlScene`.
  High-DPI: draw at `SDL_GetWindowSizeInPixels`, UI layout from the pixel size as now.
* Window close: the loop ends, the same shutdown path as now (`GLUT_ACTION_GLUTMAINLOOP_RETURNS`).
* Drop `-lglut` and `freeglut_ext.h` from the game (the editor and the model viewer keep GLUT until
  [gl-core-tools](gl-core-tools.tooling.draft.opus.md)); `gl_includes.h` includes `<SDL3/SDL_opengl.h>` or the GL
  headers directly.
* Gamepad: not in this plan; with SDL3 in place it is a small follow-up (save it as a draft when done).
* Scenario tests: they inject input through the same functions the callbacks called; keep that seam so `scenario.cpp`
  changes little. `make test` and the snapshots ([render-snapshots](solved/render-snapshots.tooling.md)) pass
  unchanged.
* Docs: [development.md](../development.md) (dependencies, `game/game.cpp` row), [where-is-x.md](../where-is-x.md)
  rows that name GLUT.

## Done

* `src/game/window.*`: one `SDL_Window` with a GL 2.1 compatibility context (RGBA 8, depth 24, double buffer),
  high-DPI (draws at `SDL_GetWindowSizeInPixels`, mouse positions scaled by the pixel density), fullscreen as SDL's
  borderless desktop one, resize by `SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED`. Own main loop `Window::run`: events, the
  update at `UPDATE_TICK_MS` as a fixed step on `SDL_GetTicksNS` (at most 5 to catch up, then afresh: the loading),
  a draw after each update or when `FrameRate::due` wants one (vsync by `SDL_GL_SetSwapInterval`, the GLX code went).
  The `glutPostRedisplay` calls went: a draw follows every update, as before. `Window::runScenario`: only the
  window's events and `Scenario::tick`. Close button or Quit: the loop ends, the same cleanup (settings saved). SDL
  also turns SIGTERM into that quit.
* Input: `handleInputEvent` (`src/input/input.cpp`) feeds the old seam (`keyPressed`, `specialKeyPressed`, their
  releases, `processMouse`, the motion functions), which the scenarios call as before. Keys go by place
  (`Keymap::physical`, `src/input/keymap.*`, scancodes read as on a US layout), so the bindings and the `[controls]`
  names stay; the text fields (riddle, Hall of Records name) take the layout's typed character. The game's key codes
  kept GLUT's numbers, now named `SpecialKey` (`level/bindings.h`); `MOUSE_DOWN` / `MOUSE_UP` replace `GLUT_DOWN` /
  `GLUT_UP`. Repeat from `event.key.repeat`, dropped in a cut-scene (`ignoreKeyRepeat`). The wheel: buttons 3 / 4 per
  notch, as freeglut sent them. Window enter / leave: `processMouseEntry`.
* The held pointer (plan mouse-leaves-window) is SDL's relative mode (`Window::holdPointer`): hidden, cannot leave the
  window, its motions only turn the look; let go, it shows at the window's centre (`SDL_WarpMouseInWindow`). The warp
  hold in `MouseLook` and its tests went.
* `-lglut` and `freeglut_ext.h` are out of the game (the editor and the model viewer link `GLUT_LIBS`);
  `gl_includes.h` includes `GL/gl.h` and `GL/glu.h`.
* `--no-sound` and scenarios set `SDL_VIDEO_DRIVER=x11` (unless set): SDL3 prefers Wayland and `WAYLAND_DISPLAY`
  outlives `xvfb-run`, so background runs and the snapshots would open on the desktop, on the GPU.
* Gamepad: draft [sdl3-gamepad](sdl3-gamepad.feature.draft.opus.md). Unit test `tests/unit/keymap_test.cpp`.
  `make test` and `make snapshots` pass unchanged (79 images, 0 changed).

## Play-test

* `bin/game --level 1`: walk, turn, jump, crouch, sprint; mouse look (now raw motion: its speed may feel a little
  different) and clicks in the inventory work as before; the pointer never leaves the window while looking.
* Hold a movement key: smooth walk, no stutter from key repeat; in a cut scene keys do not repeat.
* Toggle fullscreen and back, resize the window: the picture and the UI scale right.
* Close the window with its close button: the game quits cleanly (settings saved).
