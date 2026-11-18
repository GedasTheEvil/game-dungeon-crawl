#ifndef GL_CONTEXT_H
#define GL_CONTEXT_H

#include "gl_includes.h"

// Once a GL context is current (plan gl-core-foundation): loads the GL functions through the window library's
// lookup (SDL_GL_GetProcAddress in the game, glutGetProcAddress in the tools), logs the version and the renderer, and
// in a dev build sends GL's KHR_debug errors to the log (category "graphics"). False, logged, without GL 3.3: the
// default shaders and the ported drawing need it.
bool initGlContext(GLADloadfunc getProcAddress);

#endif
