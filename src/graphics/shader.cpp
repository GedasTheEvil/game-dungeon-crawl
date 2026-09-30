#define GL_GLEXT_PROTOTYPES // GL 2.0 shader entry points, exported by libGL on Linux
#include "shader.h"

#include <GL/glext.h>
#include "../core/logger.h"

namespace {
GLuint compile(const char* name, GLenum type, const char* src) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);
	GLint ok = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (ok != GL_TRUE) {
		char log[1024];
		glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
		LOG_ERRORF("graphics", "%s shader compile failed: %s", name, log);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}
} // namespace

GLuint linkProgram(const char* name, const char* vertexSrc, const char* fragmentSrc) {
	GLuint vs = compile(name, GL_VERTEX_SHADER, vertexSrc);
	GLuint fs = compile(name, GL_FRAGMENT_SHADER, fragmentSrc);
	if (vs == 0 || fs == 0)
		return 0;
	GLuint program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glDeleteShader(vs);
	glDeleteShader(fs);
	GLint ok = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &ok);
	if (ok != GL_TRUE) {
		char log[1024];
		glGetProgramInfoLog(program, sizeof(log), nullptr, log);
		LOG_ERRORF("graphics", "%s shader link failed: %s", name, log);
		glDeleteProgram(program);
		return 0;
	}
	return program;
}
