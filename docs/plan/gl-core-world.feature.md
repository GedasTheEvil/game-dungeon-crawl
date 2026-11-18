# Port the 3D world drawing to the GL core path

Status: implemented 2026-11-18, waiting for the user's play-test. Was: draft 2026-11-18, size L, needs [gl-core-foundation](gl-core-foundation.feature.md) first.

## Problem

After [gl-core-foundation](gl-core-foundation.feature.md) the world still draws with immediate mode and
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

## Done

* World shader: the lighting shader is GLSL 330 core (`src/render/lighting.cpp`): vertices from Batch's attribute
  locations or a model's vertex array, `uMvp`, `uModelView` and the normal matrix as uniforms, the fog in it as
  before; it also does the alpha test (a `Style` of kind `AlphaTest`, the expedition page's torn edge).
  `Lighting::use` is a `Batch::Program` (a hook added to `Batch`); `Lighting::batch()` is the world's batch, its
  current colour, normal and texture coordinate the world's, as GL's current values were.
* Models: one vertex array per model over its buffers (`AnimatedModel::Compile`); the key frame blend was already in
  the vertex shader and stays there (next frame at attributes 2 and 3). The legacy client arrays stay only for a draw
  outside `Lighting` (the journal's sketches with fixed-function light, gl-core-ui's).
* Files ported: `draw.cpp` (the world part; the HUD after it gets GL's matrices), `dungeon_render*.cpp`,
  `fire.cpp` (bridge calls gone), `item.cpp`, `monster_draw.cpp`, `particles_draw.cpp`, `player_view.cpp`,
  `trap.cpp`, `assets.cpp` (the loading screen's quads), `animated_model.cpp`, `lighting.cpp`, `motion_fx.cpp`,
  `ink.cpp`, `inscription_draw.cpp`. Left legacy: `draw.cpp` 1 (`GL_TEXTURE_2D` for the legacy tooltip),
  `assets.cpp` 3 (the font's colour), `animated_model.cpp` 9 (the fallback above): the UI's, for gl-core-ui.
* The border: `GlLegacy::PortedScope` / `LegacyScope` carry the matrices and the current values between ported and
  legacy code (the inventory's and journal's models and inscriptions, the model viewer, a monster's status cards).
* Exactness: `Mat4::rotation` now takes `sinf`/`cosf` of the float angle as Mesa (cos 90 degrees is -4.4e-8, not
  6e-17) and `mat4.o` is built without FMA contraction: compared op by op with GL's stack over every snapshot
  scenario, the matrices have GL's bits. The references were retaken once for a precision change: the normal matrix
  (now our inverse transpose, Mesa picks its inverse by the matrix's history) and the lights' eye positions, at most
  14 of 255 in a dozen pixels, 1 to 4 elsewhere; and once more for `monsters-webs` (4 pixels off by 1: a monster's
  status cards, still legacy UI, no longer drawn through the lighting program's fog).
* Render targets: `render_target.cpp` and the ink's target were already core (`GL_DEPTH_COMPONENT24`, no `_EXT`); no
  change.
* Not done: static dungeon geometry in per-level VBOs. The cell surfaces are drawn cell by cell between the decals,
  decor and contents of each cell (blended decals and coplanar faces depend on that order, the snapshots are
  bit-exact), and they change while a level runs (sand drifts swirl with the clock, buried cells turn to sand);
  every quad goes through the world batch's streaming buffer instead. The frame time dropped without it.
* Frame time: `FRAME_TIME=N` (`frame_rate.*`) logs Draw's mean time over N frames. A scenario on level 7 at
  1280 x 720 drawing every tick (`level content/levels/lvl7`, `walk right 2`, `wait 600`, a screenshot), Xvfb and
  llvmpipe, the old build (fab5dc17) and this one run in turns, 8 rounds of three 200-frame means: the same within the
  machine's noise. The quietest round: before 2.68, 2.73, 2.78 ms, after 2.71, 2.80, 2.82 ms; over all rounds the new
  build is 0 to 4 % slower in a pair (the world's quads go to the GPU as 6 vertices, not 4 and an index; most of the
  frame is llvmpipe's fill). The world shader sets a uniform only when it changed (most draws in a row share the
  matrices); a build without that measured 5 to 10 % slower.
