# Switch to a GL 3.3 core profile context and ban legacy GL

Status: implemented 2026-11-19, waiting for the user's play-test. Was: draft 2026-11-18, size M, needs [gl-core-world](gl-core-world.feature.md), [gl-core-ui](gl-core-ui.feature.md), [gl-core-tools](solved/gl-core-tools.tooling.md) first.

## Problem

After [gl-core-world](gl-core-world.feature.md), [gl-core-ui](gl-core-ui.feature.md) and
[gl-core-tools](solved/gl-core-tools.tooling.md) no code needs the compatibility context, but the game still
asks for one, so a forgotten legacy call would still work and drift back in.

## Plan

* Ask SDL for a 3.3 core profile (forward compatible, so it also runs on macOS); dev builds add the debug flag.
* Remove `MatrixStack::syncLegacy()` and every remaining compatibility leftover (`GL_QUADS` outside `Batch`,
  `glEnable(GL_TEXTURE_2D)`, `glShadeModel`, `GL_LIGHTING`, `GL_ALPHA_TEST`, the default VAO assumption).
* The legacy-GL counter from [gl-core-foundation](gl-core-foundation.feature.md) becomes a ban in
  `make check`: any legacy call fails the build check (an allowlist is not needed).
* Run everything with the `KHR_debug` callback on: `make test`, `make snapshots`, a level of each zone; zero GL
  errors and no snapshot diff.
* Check on llvmpipe (the tests) and the user's AMD GPU; note in [development.md](../development.md) the minimum:
  OpenGL 3.3.
* Version bump for the whole port in this plan's commit is the implementer's call per AGENTS.md (each feature plan
  already bumped DEFAULT).

## Done

* `GlWindow::open` asks for a forward-compatible 3.3 core context, with the debug flag in dev builds; the log names
  it ("GL context: core, forward compatible, debug"). The editor now logs to `game.log` too.
* glad regenerated as `gl:core=3.3` (+ `KHR_debug`): a legacy call no longer compiles. The last three in
  `game/window.cpp` are gone, and so is the bridge `src/render/gl_legacy.*` (`syncLegacy` was its `syncMatrices`).
* `tools/gl_legacy.py` is a ban: any counted call fails `make layers`. It also counts `glLineWidth`,
  `GL_LINE_SMOOTH`, one-sided `glPolygonMode`, `GL_POLYGON_OFFSET_LINE` and `glPointSize`.
* What core rejects or draws otherwise goes through `WideLines` (`src/render/wide_lines.*`): a geometry shader
  variant of the default programs and the world shader turns wide lines (HUD and menu strokes, webs, hitboxes),
  smooth lines (map pencil, page curl edge), back face edges (trap contour, journal sketch; depth offset included)
  and big points (blood) into quads. The quads follow Mesa's llvmpipe code (`lp_setup_line.c`, the draw module's
  aaline, offset, unfilled and cull stages, `lp_setup_point.c`). A probe that compared legacy lines with the shader
  on 3000 random lines each found no differing pixel for plain or smooth lines.
* Every draw binds its own vertex array; `Batch` and `drawScreenQuad` no longer read and restore the program and
  vertex array, `AnimatedModel::Compile` no longer reads the bound one.
* GL errors (`KHR_debug`, debug context): 0 in `make test`, 0 in `make snapshots`, 0 in a level of each zone
  (4, 9, 13, 18, 22, 27, 32, 37, 41, 45, 50, 54), 0 in the editor and the model viewer.
* `make snapshots`: 76 of 79 images identical. Three differ under the threshold: the map and the journal turn by one
  level of a channel on a few pixels (the smooth coverage's float order), the trap contour on 9 rim pixels by one
  faint pass (a depth tie between overlapping edge quads). No reference was retaken.
* Not checked here: the user's AMD GPU (the runs stay on Xvfb and llvmpipe): left to the play-test.

## Play-test

* `bin/game --level 1` and a later level: the game starts (no blank window), looks and plays as before.
* Menus, journal, map and a boss fight show no missing or black parts.
* A trapped chest with Disarm trap shows its red contour; the journal's pencil sketch has its outline; the map's
  pencil lines are smooth.
* `game.log` says "GL context: core, forward compatible, debug" and no "GL <number>:" error lines.
