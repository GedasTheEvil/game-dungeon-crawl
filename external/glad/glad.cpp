// The GL loader (plan gl-core-foundation): glad 2.0.8, header-only, its implementation in this translation unit. Built
// with the game and the tools, not checked by clang-tidy. The game, the editor and the model viewer load it with
// SDL_GL_GetProcAddress (src/game/gl_window.cpp).
//
// Regenerate (pip install glad2 into a venv; --reproducible uses the spec bundled with glad, no download):
//   python -m glad --quiet --reproducible --api "gl:core=3.3" \
//     --extensions GL_KHR_debug,GL_EXT_texture_filter_anisotropic,GL_ARB_texture_filter_anisotropic \
//     --out-path /tmp/glad c --header-only
// then copy /tmp/glad/include/glad/gl.h to external/glad/include/glad/. The core profile (plan gl-core-profile) has no
// legacy call: one left in the code does not compile.
#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
