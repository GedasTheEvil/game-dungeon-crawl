#ifndef FRAME_RATE_H
#define FRAME_RATE_H

// How often the whole game is drawn (plan frame-rate-cap), the Frame rate option: the monitor's rate (vsync), a cap
// (30, 60) or unlimited. Drawing only: the game logic tick (UPDATE_TICK_MS) stays as it is.
namespace FrameRate {

// The Settings::Display frameRate (FRAME_RATE_MONITOR, FRAME_RATE_UNLIMITED or a cap). Safe before the window is
// made: the swap interval changes on the next idle call.
void set(int frameRate);
// The GLUT idle callback's work: applies the swap interval, posts a redraw when one is due, else sleeps a little.
void idle();

} // namespace FrameRate

#endif
