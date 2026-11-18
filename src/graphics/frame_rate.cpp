#include "frame_rate.h"
#include "../core/logger.h"
#include "../core/timer.h"
#include "../game/window.h"
#include "../level/settings_ini.h"

namespace {
int gFrameRate = FRAME_RATE_MONITOR;
int gAppliedInterval = -1; // the swap interval set on the window; -1: none yet
int gLastDrawMs = 0;
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

} // namespace FrameRate
