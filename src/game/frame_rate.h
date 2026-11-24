#ifndef FRAME_RATE_H
#define FRAME_RATE_H

// How often the whole game is drawn (plan frame-rate-cap), the Frame rate option: the monitor's rate (vsync), a cap
// (30, 60) or unlimited. Drawing only: the game logic tick (UPDATE_TICK_MS) stays as it is.
namespace FrameRate {

// The Settings::Display frameRate (FRAME_RATE_MONITOR, FRAME_RATE_UNLIMITED or a cap). Safe before the window is
// made: the swap interval changes on the next due call.
void set(int frameRate);
// Asked by the main loop (Window::run) on each pass: applies the swap interval; true when a frame is due.
bool due();
// FRAME_TIME=N in the environment (plan gl-core-world): Draw times each frame, from its start to the buffer swap, and
// every N frames the log ("graphics") gets their mean and the fastest. drawStarted before the frame's first GL call,
// drawEnded after the swap.
void drawStarted();
void drawEnded();

} // namespace FrameRate

#endif
