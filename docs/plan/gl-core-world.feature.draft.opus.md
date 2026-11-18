# Port the 3D world drawing to the GL core path

Status: draft 2026-11-18, size L, needs [gl-core-foundation](gl-core-foundation.feature.draft.opus.md) first.

## Problem

After [gl-core-foundation](gl-core-foundation.feature.draft.opus.md) the world still draws with immediate mode and
the GL matrix stack. This plan moves it to `MatrixStack`, `Batch`, VAOs and the GLSL 330 shaders.

## Plan

* Files: `src/graphics/` (`draw.cpp`, `dungeon_render*.cpp`, `fire.cpp`, `item.cpp`, `monster_draw.cpp`,
  `particles_draw.cpp`, `player_view.cpp`, `trap.cpp`, `assets.cpp`, `frame_rate.cpp`), `src/render/`
  (`animated_model.cpp`, `lighting.cpp`, `motion_fx.cpp`, `render_target.cpp`, `textures.cpp`,
  `inscription_draw.cpp`, `ink.cpp`), `src/level/view_window.h`.
* Models (`animated_model.cpp`): client arrays → one VAO per model with its VBO/IBO (already there); the
  vertex animation keeps its CPU blend for now, or moves to the vertex shader (two frames + blend factor) if that
  is simple; the snapshots decide.
* Static dungeon geometry (walls, floors): build per level into VBOs instead of re-sending vertices each frame,
  where it is built in one place now; per-frame `Batch` for the rest. Do not change what is drawn.
* Lighting: the existing lighting shader becomes the world shader (GLSL 330); fog, if any, in the shader.
* Render targets: check the FBO code against core (no `GL_DEPTH_STENCIL_EXT` style leftovers).
* Port file by file; after each file the legacy counter drops and `make snapshots` stays within tolerance
  ([render-snapshots](solved/render-snapshots.tooling.md)). A diff is fixed, not accepted, unless it is a
  plain precision change (say why in the commit).
* Measure frame time before and after (a fixed scene, `frame_rate.cpp`); report it in the plan.

## Play-test

* `bin/game --level 7` (or the level with the most decor): walls, water, sand, webs, drips, sun shafts, fires and
  monsters look the same as before.
* Fight a few monsters: animations, motion blur, particles and hit effects as before.
* The frame rate is the same or better.
