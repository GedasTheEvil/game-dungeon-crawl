#ifndef GL_WINDOW_H
#define GL_WINDOW_H

#include <SDL3/SDL_video.h>

// An SDL window with the OpenGL context the game, the level editor and the model viewer draw in (plans
// sdl3-window-input, gl-core-tools): a GL 3.3 core context (plan gl-core-profile), RGBA 8 bits each, 24-bit depth,
// double buffered, the GL functions loaded (initGlContext). SDL_Init(SDL_INIT_VIDEO) comes first. No Game(): the
// tools link this file alone (makefile, GL_WINDOW_OBJECT).
struct GlWindow {
	SDL_Window* window = nullptr;
	SDL_GLContext context = nullptr;

	// SDL_WINDOW_OPENGL is added to flags. False: SDL's error is logged under "graphics" and the window closed.
	bool open(const char* title, int width, int height, SDL_WindowFlags flags);
	void close();
	void swap() const { SDL_GL_SwapWindow(window); }
	// The drawable's size in pixels (high-DPI aware), the height at least 1.
	void pixelSize(int& width, int& height) const;
};

#endif
