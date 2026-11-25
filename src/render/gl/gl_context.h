#ifndef GL_CONTEXT_H
#define GL_CONTEXT_H

#include "gl_includes.h"

// Once a GL context is current (plan gl-core-foundation): loads the GL functions through the window library's
// lookup (SDL_GL_GetProcAddress, src/game/gl_window.cpp), logs the version and the renderer, and
// in a dev build sends GL's KHR_debug errors to the log (category "graphics"). False, logged, without GL 3.3: the
// default shaders and the ported drawing need it.
bool initGlContext(GLADloadfunc getProcAddress);

// Before the window library destroys the context (GlWindow::close).
void endGlContext();

// True from a good initGlContext to endGlContext: the GL objects (textures, render targets, models) free themselves
// only then (closing the window destroys the context before the game's own cleanup runs). Kept here, not asked of GLX:
// no libGL link (the window library loads the driver), and right on EGL too (plan portable-linux-release).
[[nodiscard]] bool glContextCurrent();

#endif
