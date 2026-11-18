# GL core foundation: loader, math, matrix stack, vertex batcher, GLSL 330

Status: draft 2026-11-18, size L, needs [sdl3-window-input](sdl3-window-input.feature.draft.opus.md), [render-snapshots](solved/render-snapshots.tooling.md) first.

## Problem

The drawing is OpenGL 1.x fixed function with a few GLSL 1.20 shaders on top: about 68 `glBegin`/`glEnd` blocks
(fire, ui_draw, map, sand, page curl, ...), the matrix stack (`glPushMatrix`/`glTranslatef`/`glRotatef`, ~70 pairs),
`glColor`/`glTexCoord`/`glNormal`, client arrays in `animated_model.cpp`, display lists in `font.cpp`,
`glPushAttrib`, `glAlphaFunc`, `GL_QUADS` (39), fixed-function lights in the journal page, GLU (`gluPerspective`,
`gluProject`), matrix readbacks with `glGetFloatv` (17). Shaders: brightness, ink, lighting, motion FX
(`#version 120`, `GL_GLEXT_PROTOTYPES`). None of this exists in a core profile, on macOS, GLES or WebGL, and newer
drivers keep it only for compatibility.

This plan builds the parts every later port step uses, without changing a pixel. The port itself:
[gl-core-world](gl-core-world.feature.draft.opus.md), [gl-core-ui](gl-core-ui.feature.draft.opus.md),
[gl-core-tools](gl-core-tools.tooling.draft.opus.md), then [gl-core-profile](gl-core-profile.feature.draft.opus.md).

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
  count grows; ported files drop to zero. [gl-core-profile](gl-core-profile.feature.draft.opus.md) turns it into a
  ban.
* Port two small users as proof (e.g. `brightness.cpp` and `fire.cpp`, the largest `glBegin` user) and check them
  against [render-snapshots](solved/render-snapshots.tooling.md).
* Unit tests: `MatrixStack` against known matrices, `project` against the old `gluProject` result, `Batch` quad
  splitting.
* Docs: a "GL core path" section in [development.md](../development.md) (the matrix stack, `Batch`, shaders, how
  to port a file); glossary rows for the new terms.
* After the profile switch, SDL_GPU (Vulkan, Metal, D3D12 behind one SDL3 API) is the next step up; it needs shader
  cross-compiling (SPIR-V/MSL) and would be its own plan. Not worth it for this game unless a platform needs it.

## Play-test

* `bin/game --level 1`: the game looks and plays as before (fire, brightness, the first ported files).
