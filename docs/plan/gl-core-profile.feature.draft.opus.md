# Switch to a GL 3.3 core profile context and ban legacy GL

Status: draft 2026-11-18, size M, needs [gl-core-world](gl-core-world.feature.draft.opus.md), [gl-core-ui](gl-core-ui.feature.draft.opus.md), [gl-core-tools](gl-core-tools.tooling.draft.opus.md) first.

## Problem

After [gl-core-world](gl-core-world.feature.draft.opus.md), [gl-core-ui](gl-core-ui.feature.draft.opus.md) and
[gl-core-tools](gl-core-tools.tooling.draft.opus.md) no code needs the compatibility context, but the game still
asks for one, so a forgotten legacy call would still work and drift back in.

## Plan

* Ask SDL for a 3.3 core profile (forward compatible, so it also runs on macOS); dev builds add the debug flag.
* Remove `MatrixStack::syncLegacy()` and every remaining compatibility leftover (`GL_QUADS` outside `Batch`,
  `glEnable(GL_TEXTURE_2D)`, `glShadeModel`, `GL_LIGHTING`, `GL_ALPHA_TEST`, the default VAO assumption).
* The legacy-GL counter from [gl-core-foundation](gl-core-foundation.feature.draft.opus.md) becomes a ban in
  `make check`: any legacy call fails the build check (an allowlist is not needed).
* Run everything with the `KHR_debug` callback on: `make test`, `make snapshots`, a level of each zone; zero GL
  errors and no snapshot diff.
* Check on llvmpipe (the tests) and the user's AMD GPU; note in [development.md](../development.md) the minimum:
  OpenGL 3.3.
* Version bump for the whole port in this plan's commit is the implementer's call per AGENTS.md (each feature plan
  already bumped DEFAULT).

## Play-test

* `bin/game --level 1` and a later level: the game starts (no blank window), looks and plays as before.
* Menus, journal, map and a boss fight show no missing or black parts.
