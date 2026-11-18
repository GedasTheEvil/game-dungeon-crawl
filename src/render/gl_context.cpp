#include "gl_context.h"

#include "../core/logger.h"

namespace {
// A broken frame repeats its error every frame: the log takes the first ones.
constexpr int MAX_DEBUG_MESSAGES = 50;
int gDebugMessages = 0;

void GLAD_API_PTR onDebugMessage(
	GLenum /*source*/,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei /*length*/,
	const GLchar* message,
	const void* /*user*/
) {
	if (gDebugMessages >= MAX_DEBUG_MESSAGES)
		return;
	if (++gDebugMessages == MAX_DEBUG_MESSAGES) {
		LOG_ERROR("graphics", "GL: more debug messages, not logged");
		return;
	}
	const bool error = type == GL_DEBUG_TYPE_ERROR || type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR;
	if (error || severity == GL_DEBUG_SEVERITY_HIGH)
		LOG_ERRORF("graphics", "GL %u: %s", id, message);
	else
		LOG_WARNINGF("graphics", "GL %u: %s", id, message);
}

// Errors, undefined behaviour and the high severity rest; the compatibility context's deprecation and performance
// notes would flood the log while the port runs.
void installDebugLog() {
	if (!GLAD_GL_KHR_debug)
		return;
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(onDebugMessage, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
	glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_ERROR, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
	LOG_INFO("graphics", "GL debug messages to the log");
}
} // namespace

bool initGlContext(GLADloadfunc getProcAddress) {
	const int version = gladLoadGL(getProcAddress);
	if (version == 0) {
		LOG_ERROR("graphics", "No GL functions: the loader failed");
		return false;
	}
	const auto* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
	const auto* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	const auto* full = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	LOG_INFOF("graphics", "OpenGL %s, %s, %s", full ? full : "?", renderer ? renderer : "?", vendor ? vendor : "?");
	if (GLAD_VERSION_MAJOR(version) < 3 || (GLAD_VERSION_MAJOR(version) == 3 && GLAD_VERSION_MINOR(version) < 3)) {
		LOG_ERRORF(
			"graphics",
			"OpenGL 3.3 needed, the driver gives %d.%d",
			GLAD_VERSION_MAJOR(version),
			GLAD_VERSION_MINOR(version)
		);
		return false;
	}
#ifndef GAME_RELEASE
	installDebugLog();
#endif
	return true;
}
