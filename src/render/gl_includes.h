#ifndef GL_INCLUDES_H
#define GL_INCLUDES_H
// The GL API through the glad loader (external/glad, plan gl-core-foundation): include this, never <GL/gl.h> or
// <GL/glext.h> (glad refuses to come after them). Its functions are pointers, loaded once a context is current
// (initGlContext, gl_context.h: Window::open in the game, the tools' main). No GLU: mat4.h has its perspective and
// project. The game's window and context are SDL's (game/window.h, plan sdl3-window-input).
#include <glad/gl.h>
#endif
