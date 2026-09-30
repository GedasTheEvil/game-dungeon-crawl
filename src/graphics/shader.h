#ifndef SHADER_H
#define SHADER_H

#include <GL/gl.h>

// A GLSL program from its vertex and fragment source, 0 on failure (the log names `name`: "Lighting", "Ink").
[[nodiscard]] GLuint linkProgram(const char* name, const char* vertexSrc, const char* fragmentSrc);

#endif
