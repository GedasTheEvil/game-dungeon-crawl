#include "window.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <algorithm>
#include "../render/gl_includes.h"
#include "gl_window.h"
#include "../core/logger.h"
#include "../graphics/draw.h"
#include "../graphics/frame_rate.h"
#include "../input/input_events.h"
#include "../level/gameplay_shared.h"
#include "../level/input.h"
#include "../state/game_loop.h"
#include "../state/game_state.h"
#include "../state/settings.h"
#include "../test/scenario.h"

namespace {
GlWindow gGl;
bool gQuit = false;
bool gHeld = false;

constexpr Uint64 UPDATE_TICK_NS = static_cast<Uint64>(UPDATE_TICK_MS) * SDL_NS_PER_MS;
// Updates run to catch up after a slow pass, at most this many in a row; behind further (the loading, a stall), the
// clock starts afresh instead of a burst.
constexpr int MAX_CATCH_UP = 5;
// A short sleep while no update and no frame is due: the input still gets through at once.
constexpr Uint64 IDLE_SLEEP_NS = SDL_NS_PER_MS;

// The window's new size in pixels: the viewport and the UI layout follow it. Every screen sets its own projection
// each frame (the scene's in graphics/draw.cpp), so only the viewport here.
void resized() {
	int width = 0;
	int height = 0;
	SDL_GetWindowSizeInPixels(gGl.window, &width, &height);
	height = std::max(height, 1);
	glViewport(0, 0, width, height);
	LOG_INFOF("graphics", "Resized to : %d x %d", width, height);
	Game().render.resX = width;
	Game().render.resY = height;
	// A window resized by hand opens at that size next time (saved on quit); fullscreen keeps the window's size.
	Settings::Display& display = Game().settings.display;
	if (!display.fullscreen && !Scenario::active()) {
		int w = 0;
		int h = 0;
		SDL_GetWindowSize(gGl.window, &w, &h);
		display.width = std::clamp(w, MIN_WINDOW_W, MAX_WINDOW_W);
		display.height = std::clamp(h, MIN_WINDOW_H, MAX_WINDOW_H);
	}
}

// The window's own events, then (a played game only) the keys and the mouse.
void pollEvents(bool input) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_EVENT_QUIT:
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			gQuit = true;
			break;
		case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
			resized();
			break;
		case SDL_EVENT_WINDOW_MOUSE_ENTER:
		case SDL_EVENT_WINDOW_MOUSE_LEAVE:
			if (input)
				processMouseEntry();
			break;
		default:
			if (input)
				handleInputEvent(event, SDL_GetWindowPixelDensity(gGl.window));
			break;
		}
	}
}

// The scene's GL state, once: the screens set the rest per frame.
void initGl() {
	glBlendFunc(GL_DST_COLOR, GL_ZERO);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
}
} // namespace

namespace Window {

bool open(int width, int height, bool fullscreen) {
	SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
	if (fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN;
	if (!gGl.open("Dungeon Crawl", width, height, flags))
		return false;
	initGl();
	resized();
	return true;
}

void close() { gGl.close(); }

void run() {
	Uint64 nextUpdate = SDL_GetTicksNS();
	while (!gQuit) {
		pollEvents(true);
		bool updated = false;
		for (int steps = 0; !gQuit && SDL_GetTicksNS() >= nextUpdate; steps++) {
			if (steps == MAX_CATCH_UP) {
				nextUpdate = SDL_GetTicksNS() + UPDATE_TICK_NS;
				break;
			}
			Update();
			Scenario::runPlaySetup();
			nextUpdate += UPDATE_TICK_NS;
			updated = true;
		}
		if (gQuit)
			break;
		// A frame after every update, and between them as the frame rate wants (plan frame-rate-cap).
		if (FrameRate::due() || updated)
			Draw();
		else
			SDL_DelayNS(IDLE_SLEEP_NS);
	}
}

void runScenario() {
	while (!gQuit) {
		pollEvents(false);
		Scenario::tick();
		if (const int delay = Scenario::tickDelayMs(); delay > 0)
			SDL_Delay(static_cast<Uint32>(delay));
	}
}

void quit() { gQuit = true; }

void swap() { gGl.swap(); }

void setFullscreen(bool on) {
	if (!SDL_SetWindowFullscreen(gGl.window, on))
		LOG_WARNINGF("display", "Fullscreen %s failed: %s", on ? "on" : "off", SDL_GetError());
}

void resize(int width, int height) { SDL_SetWindowSize(gGl.window, width, height); }

Size desktopSize() {
	const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(SDL_GetDisplayForWindow(gGl.window));
	if (mode == nullptr)
		return {MAX_WINDOW_W, MAX_WINDOW_H};
	return {mode->w, mode->h};
}

bool setSwapInterval(int interval) { return SDL_GL_SetSwapInterval(interval); }

void holdPointer(bool held) {
	if (Scenario::active() || held == gHeld) // a script has no pointer
		return;
	gHeld = held;
	SDL_SetWindowRelativeMouseMode(gGl.window, held);
	if (!held) {
		int w = 0;
		int h = 0;
		SDL_GetWindowSize(gGl.window, &w, &h);
		SDL_WarpMouseInWindow(gGl.window, static_cast<float>(w) / 2, static_cast<float>(h) / 2);
	}
}

} // namespace Window
