#include "frame_rate.h"
#include "../core/logger.h"
#include "../core/timer.h"
#include "../game/window.h"
#include "../level/settings_ini.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>

namespace {
int gFrameRate = FRAME_RATE_MONITOR;
int gAppliedInterval = -1; // the swap interval set on the window; -1: none yet
int gLastDrawMs = 0;

// FRAME_TIME: frames per report (0: off), the current frame's start and the sums since the last report.
struct FrameTimes {
	int every = 0;
	std::chrono::steady_clock::time_point start;
	int frames = 0;
	double sumMs = 0.0;
	double minMs = 0.0;
};
FrameTimes& frameTimes() {
	static FrameTimes times = [] {
		FrameTimes t;
		const char* every = std::getenv("FRAME_TIME");
		t.every = every != nullptr ? std::max(0, std::atoi(every)) : 0;
		return t;
	}();
	return times;
}
} // namespace

namespace FrameRate {

void set(int frameRate) { gFrameRate = frameRate; }

bool due() {
	const int interval = gFrameRate == FRAME_RATE_MONITOR ? 1 : 0;
	if (interval != gAppliedInterval) {
		if (!Window::setSwapInterval(interval))
			LOG_WARNINGF("display", "No swap interval control: the frame rate is the driver's (wanted %d)", interval);
		gAppliedInterval = interval; // tried once a change
	}
	const int now = GameClock::now();
	if (!frameRedrawDue(gLastDrawMs, now, gFrameRate))
		return false;
	gLastDrawMs = now;
	return true;
}

void drawStarted() {
	FrameTimes& t = frameTimes();
	if (t.every > 0)
		t.start = std::chrono::steady_clock::now();
}

void drawEnded() {
	FrameTimes& t = frameTimes();
	if (t.every <= 0)
		return;
	const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t.start).count();
	t.minMs = t.frames == 0 ? ms : std::min(t.minMs, ms);
	t.sumMs += ms;
	if (++t.frames < t.every)
		return;
	LOG_INFOF("graphics", "Frame time over %d frames: mean %.2f ms, fastest %.2f ms", t.frames, t.sumMs / t.frames, t.minMs);
	t.frames = 0;
	t.sumMs = 0.0;
}

} // namespace FrameRate
