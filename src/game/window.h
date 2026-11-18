#ifndef WINDOW_H
#define WINDOW_H

// The game's window on SDL3 (plan sdl3-window-input): one SDL_Window with an OpenGL context, the main loop, the
// display's size, fullscreen and the pointer. SDL_Init(SDL_INIT_VIDEO) comes first (main).
namespace Window {

// Opens the window at width x height (fullscreen: borderless on the desktop) with a GL 2.1 compatibility context:
// RGBA 8 bits each, 24-bit depth, double buffered. Draws at its size in pixels (high-DPI); the first resize is applied
// at once. False: SDL's error is logged.
bool open(int width, int height, bool fullscreen);
void close();

// The main loop until quit() or the window's close button: the events, the game's update every UPDATE_TICK_MS (a
// fixed step on SDL's clock) and a draw after each update or when the frame rate wants one.
void run();
// A scenario's loop: only the window's events (its resize, the close button) and Scenario::tick every
// Scenario::tickDelayMs; the script draws its own frames and ends the process.
void runScenario();
// The loop ends after this pass (Quit in the menu).
void quit();

void swap(); // the drawn frame on screen

void setFullscreen(bool on);
void resize(int width, int height); // the window's size, not in fullscreen
struct Size {
	int w;
	int h;
};
[[nodiscard]] Size desktopSize(); // the display the window is on
// Vsync on (1) or off (0). False: the driver keeps its own.
bool setSwapInterval(int interval);

// The pointer held: hidden, in relative mode, its motions only turn the look (plan mouse-leaves-window). Let go, it
// shows again at the window's centre.
void holdPointer(bool held);

} // namespace Window

#endif
