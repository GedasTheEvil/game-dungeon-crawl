# GL core foundation: loader, math, matrix stack, vertex batcher, GLSL 330

Status: solved 2026-11-19 (play-tested by the user). Was: implemented 2026-11-18, waiting for the user's play-test. Was: draft 2026-11-18, size L, needs [sdl3-window-input](../sdl3-window-input.feature.md), [render-snapshots](render-snapshots.tooling.md) first.

## Problem

The drawing is OpenGL 1.x fixed function with a few GLSL 1.20 shaders on top: about 68 `glBegin`/`glEnd` blocks
(fire, ui_draw, map, sand, page curl, ...), the matrix stack (`glPushMatrix`/`glTranslatef`/`glRotatef`, ~70 pairs),
`glColor`/`glTexCoord`/`glNormal`, client arrays in `animated_model.cpp`, display lists in `font.cpp`,
`glPushAttrib`, `glAlphaFunc`, `GL_QUADS` (39), fixed-function lights in the journal page, GLU (`gluPerspective`,
`gluProject`), matrix readbacks with `glGetFloatv` (17). Shaders: brightness, ink, lighting, motion FX
(`#version 120`, `GL_GLEXT_PROTOTYPES`). None of this exists in a core profile, on macOS, GLES or WebGL, and newer
drivers keep it only for compatibility.

This plan builds the parts every later port step uses, without changing a pixel. The port itself:
[gl-core-world](../gl-core-world.feature.md), [gl-core-ui](../gl-core-ui.feature.md),
[gl-core-tools](gl-core-tools.tooling.md), then [gl-core-profile](../gl-core-profile.feature.md).

## Plan

* Target: OpenGL 3.3 core (runs everywhere that matters: Mesa incl. llvmpipe for the tests, Windows, macOS up to
  4.1). No Vulkan or SDL_GPU here (see the last point).
* Loader: glad 2 generated for GL 3.3 core (+ `KHR_debug`), committed under `external/glad/`, loaded with
  `SDL_GL_GetProcAddress`. Drop `GL_GLEXT_PROTOTYPES` and the `glext.h` include.
* Math: glm (`libglm-dev`, header-only, 1.0.1, installed) or a small own `Mat4`/`Vec3` in `src/render/`
  if glm's compile cost hurts `make check` (measure). Replaces GLU: `perspective`, `project`
  (`draw.cpp` `gluProject`).
* `MatrixStack` (`src/render/`): the CPU twin of the GL matrix stack, same verbs (`push`, `pop`, `translate`,
  `rotate`, `scale`, `mult`, `load`), a modelview and a projection stack. Call sites change mechanically
  (`glPushMatrix()` → `gMv.push()`); the `glGetFloatv(GL_MODELVIEW_MATRIX)` readbacks read the stack instead.
  While the compatibility context remains, a `syncLegacy()` loads the current matrices into GL so not yet ported
  draw code keeps working: the port goes file by file.
* `Batch` (`src/render/`): replaces immediate mode. `begin(primitive)`, `vertex`, `texCoord`, `color`, `normal`,
  `end()` fill a CPU array; `end` uploads to one streaming VBO (orphaning or a ring buffer) and draws with a VAO and
  the default shader. `GL_QUADS` turns into two triangles inside `Batch`, so call sites keep quads. Same API shape as
  `glBegin` so a port is a rename plus the matrix uniform.
* Default shaders (GLSL 330 core): position + colour, + texture, + texture with alpha test (`discard`, replaces
  `glAlphaFunc`), + normal with one directional light (replaces the journal page's fixed-function light, matching
  `GL_LIGHT0` ambient/diffuse). Uniforms: MVP, colour, sampler.
* Existing shaders (brightness, ink, lighting, motion FX) to `#version 330 core`: `in`/`out`, `texture()`, explicit
  attribute locations; they take their matrices as uniforms instead of `gl_ModelViewProjectionMatrix`.
* State: a tiny `GlState` that sets blend, depth test/write, cull per draw (replaces `glPushAttrib`/`glPopAttrib`
  with explicit save/restore of the few states the 8 call sites change).
* Dev builds: a `KHR_debug` message callback logs GL errors through the logger (category `graphics`).
* A legacy-GL counter: a script (in `make tidy` or `make check`) counts the legacy calls per file and fails when a
  count grows; ported files drop to zero. [gl-core-profile](../gl-core-profile.feature.md) turns it into a
  ban.
* Port two small users as proof (e.g. `brightness.cpp` and `fire.cpp`, the largest `glBegin` user) and check them
  against [render-snapshots](render-snapshots.tooling.md).
* Unit tests: `MatrixStack` against known matrices, `project` against the old `gluProject` result, `Batch` quad
  splitting.
* Docs: a "GL core path" section in [development.md](../../development.md) (the matrix stack, `Batch`, shaders, how
  to port a file); glossary rows for the new terms.
* After the profile switch, SDL_GPU (Vulkan, Metal, D3D12 behind one SDL3 API) is the next step up; it needs shader
  cross-compiling (SPIR-V/MSL) and would be its own plan. Not worth it for this game unless a platform needs it.

## Done

* Loader: glad 2.0.8, header-only, in `external/glad/` (`glad.cpp` holds the implementation and the command that
  regenerates it). Generated for **3.3 compatibility** + `KHR_debug` + the anisotropy extensions, not 3.3 core: the
  not yet ported code still calls `glBegin` and the rest, and glad's core header leaves them undeclared;
  [gl-core-profile](../gl-core-profile.feature.md) regenerates it as `gl:core=3.3`, which also makes any
  leftover legacy call a compile error. Every `<GL/gl.h>`, `<GL/glext.h>`, `<GL/glu.h>` and `GL_GLEXT_PROTOTYPES` is
  gone (`src/render/gl_includes.h`). `initGlContext` (`src/render/gl_context.*`) loads it: the game in
  `Window::open` with `SDL_GL_GetProcAddress`, the editor and the model viewer with `glutGetProcAddress`.
* Context: `Window::open` asks for a 3.3 compatibility profile (llvmpipe under Xvfb and Mesa give up to 4.5 compat),
  so the GLSL 330 programs and the legacy calls run side by side; no 3.3, no game (logged).
* Math: our own `Mat4` (`src/render/mat4.*`), not glm: glm's headers cost 0.3 s per translation unit (measured, an
  `-O3` compile of a file using `glm::perspective`/`rotate` against an empty one). `perspective` (gluPerspective, in
  double), `ortho`/`frustum`/`rotation` and the product (Mesa's, step for step), `project` (gluProject). GLU is gone:
  `draw.cpp`'s `gluPerspective` and `gluProject`, the model viewer's three `gluPerspective`; `-lGLU` dropped.
* `MatrixStack` (`gMv`, `gProj`, `src/render/matrix_stack.*`). The bridge both ways is `GlLegacy`
  (`src/render/gl_legacy.*`): `syncMatrices()` is the draft's `syncLegacy()`; `adoptMatrices()` is the way back,
  needed because the first ported files are called from legacy code (fire from the dungeon drawing).
* `Batch` (`src/render/batch.*`): glBegin-shaped, current values kept like GL's, one VAO and a streaming VBO
  (orphaned with `glBufferData` every draw), puts the program, VAO and buffer bindings back after a draw for the
  legacy code around it. Quads split a, b, c / a, c, d: the order llvmpipe splits `GL_QUADS` in (b-d moved fire and
  the sun shafts' pixels by one, a strip moved the blurred passes'). `drawScreenQuad()` for the full-screen passes.
* Default shaders (`src/render/default_shaders.*`), GLSL 330 core: `Colour`, `Texture`, `AlphaTest` (`discard`),
  `Lit` (GL_LIGHT0 as the journal page sets it: colour material, ambient + diffuse, per vertex, clamped). Uniforms:
  MVP, model view, tint, alpha reference, haze, light. The vertex colour is clamped like the fixed pipeline's.
  Attribute locations 0, 1, 6, 7: off NVIDIA's aliases of the legacy colour, normal and texture coordinate.
* Existing shaders: brightness, ink and motion FX are `#version 330 core` (`in`/`out`, `texture()`, the position at
  location 0, no built-in matrices: they are clip-space passes drawn by `drawScreenQuad`). Lighting is
  `#version 330 compatibility` with `in`/`out`, `texture()` and explicit locations for its own attributes, but keeps
  `gl_Vertex`, `gl_Color`, `gl_ModelViewMatrix` and `ftransform()`: the whole world feeds it through immediate mode,
  client arrays and the GL matrix stack, so its matrices can only become uniforms with
  [gl-core-world](../gl-core-world.feature.md) (left out here, on purpose).
* `GlState` (`src/render/gl_state.*`): blend and its function, depth test and write, culling; `read()` / `apply()`.
  Used by the brightness pass; the other 8 `glPushAttrib` sites are in the files of gl-core-world and gl-core-ui.
* `KHR_debug` (dev builds, not `make release`: `GAME_RELEASE`): errors, undefined behaviour and high severity to the
  log (`graphics`), the first 50. It found one at once: `MotionFx::end` saved `GL_COLOR_BUFFER_BIT` while its render
  target was bound, and the pop set `GL_COLOR_ATTACHMENT0` as the window's draw buffer (an error every frame of a
  sprint, a blind or a cut-scene); fixed by leaving the target first.
* Legacy counter: `tools/gl_legacy.py` in `make layers` (so `make tidy`, `make check`, `make tidy-push`). No baseline
  file: the base commit's counts are the baseline (`check` fails a file whose count grew since the merge base with
  `$TIDY_BASE` or master), so parallel branches never conflict over one. `report`, `show FILE`. 1269 legacy uses in
  48 files before this plan, 1052 in 47 after (`report`).
* Proof ports: `src/graphics/fire.cpp` (all sprites: fire, dust, grit, sun beams, daze stars; 0 legacy) and
  `src/render/brightness.cpp` (0). Fire takes the scene's haze from `Lighting::emissiveFog()`, as the lighting
  shader's emissive mode gave it.
* `make snapshots`: 79 images, 0 changed, before and after (no reference touched). Unit tests:
  `tests/unit/gl_core_test.cpp` (Mat4 against GL's and GLU's matrices, MatrixStack, `project` against gluProject's
  numbers, Batch's splitting and current values).
* Docs: [development.md](../../development.md#gl-core-path) (the parts, the bridge, the counter, how to port a file),
  glossary rows (GL core path, batch, matrix stack, default shader, legacy GL, GL bridge), a where-is-x row.
* Not done: SDL_GPU (the draft's last point is a note for later, no work).

## Play-test

* `bin/game --level 1`: the game looks and plays as before: the torches' and braziers' flames and smoke.
* Options > Display > Brightness up and down: the whole picture brightens and darkens as before.
* Sprint (motion blur) and toon mode (F1): the blur and the ink lines as before.
