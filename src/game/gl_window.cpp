#include "gl_window.h"

#include <SDL3/SDL_error.h>
#include <algorithm>
#include "../core/logger.h"
#include "../render/gl_context.h"

bool GlWindow::open(const char* title, int width, int height, SDL_WindowFlags flags) {
	// GL 3.3 for the core path's shaders (plan gl-core-foundation), in a compatibility context while the fixed-function
	// drawing is ported file by file (plan gl-core-profile moves to core).
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	window = SDL_CreateWindow(title, width, height, flags | SDL_WINDOW_OPENGL);
	if (window == nullptr) {
		LOG_ERRORF("graphics", "No window: %s", SDL_GetError());
		return false;
	}
	context = SDL_GL_CreateContext(window);
	if (context == nullptr) {
		LOG_ERRORF("graphics", "No OpenGL 3.3 context: %s", SDL_GetError());
		close();
		return false;
	}
	if (!initGlContext(SDL_GL_GetProcAddress)) {
		close();
		return false;
	}
	return true;
}

void GlWindow::close() {
	if (context != nullptr)
		SDL_GL_DestroyContext(context);
	if (window != nullptr)
		SDL_DestroyWindow(window);
	context = nullptr;
	window = nullptr;
}

void GlWindow::pixelSize(int& width, int& height) const {
	SDL_GetWindowSizeInPixels(window, &width, &height);
	height = std::max(height, 1);
}
