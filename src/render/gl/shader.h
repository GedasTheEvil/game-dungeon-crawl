#ifndef SHADER_H
#define SHADER_H

#include "gl_includes.h"

// A GLSL program from its vertex and fragment source (and a geometry stage, when given), 0 on failure (the log names
// `name`: "Lighting", "Ink"). The sources give their attributes' locations (layout(location = N), GLSL 330).
[[nodiscard]] GLuint
linkProgram(const char* name, const char* vertexSrc, const char* fragmentSrc, const char* geometrySrc = nullptr);

#endif
