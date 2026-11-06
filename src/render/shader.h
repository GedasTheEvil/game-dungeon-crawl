#ifndef SHADER_H
#define SHADER_H

#include <GL/gl.h>
#include <initializer_list>
#include <utility>

// A GLSL program from its vertex and fragment source, 0 on failure (the log names `name`: "Lighting", "Ink").
// attributes: generic vertex attributes bound to fixed locations before the link.
[[nodiscard]] GLuint linkProgram(
	const char* name,
	const char* vertexSrc,
	const char* fragmentSrc,
	std::initializer_list<std::pair<GLuint, const char*>> attributes = {}
);

#endif
