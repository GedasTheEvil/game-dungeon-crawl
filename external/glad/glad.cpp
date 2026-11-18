// The GL loader (plan gl-core-foundation): glad 2.0.8, header-only, its implementation in this translation unit. Built
// with the game and the tools, not checked by clang-tidy. The game loads it with SDL_GL_GetProcAddress
// (src/game/window.cpp), the editor and the model viewer with glutGetProcAddress.
//
// Regenerate (pip install glad2 into a venv; --reproducible uses the spec bundled with glad, no download):
//   python -m glad --quiet --reproducible --api "gl:compatibility=3.3" \
//     --extensions GL_KHR_debug,GL_EXT_texture_filter_anisotropic,GL_ARB_texture_filter_anisotropic \
//     --out-path /tmp/glad c --header-only
// then copy /tmp/glad/include/glad/gl.h to external/glad/include/glad/. The compatibility profile keeps the legacy
// calls the code not yet ported makes; plan gl-core-profile regenerates it as "gl:core=3.3".
#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
