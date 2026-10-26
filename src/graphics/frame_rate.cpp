#include "frame_rate.h"
#include <chrono>
#include <cstring>
#include <thread>
#include "gl_includes.h"
#include "../core/logger.h"
#include "../core/timer.h"
#include "../state/settings_ini.h"
#ifndef WIN32
#include <GL/glx.h>
#endif

namespace {
int gFrameRate = FRAME_RATE_MONITOR;
int gAppliedInterval = -1; // the swap interval set on the window; -1: none yet
int gLastDrawMs = 0;
// A short sleep while no frame is due: the input still gets through at once.
constexpr int IDLE_SLEEP_MS = 1;

#ifndef WIN32
bool hasExtension(const char* extensions, const char* name) {
	if (extensions == nullptr)
		return false;
	const size_t length = std::strlen(name);
	for (const char* at = std::strstr(extensions, name); at != nullptr; at = std::strstr(at + length, name))
		if ((at == extensions || at[-1] == ' ') && (at[length] == ' ' || at[length] == '\0'))
			return true;
	return false;
}

template <typename Fn> Fn procOf(const char* name) {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): GLX hands out untyped function pointers
	return reinterpret_cast<Fn>(glXGetProcAddressARB(reinterpret_cast<const GLubyte*>(name)));
}

// glXSwapIntervalEXT, or MESA, or SGI: whichever the driver has. False: none.
bool setSwapInterval(int interval) {
	Display* display = glXGetCurrentDisplay();
	if (display == nullptr)
		return false;
	const char* extensions = glXQueryExtensionsString(display, DefaultScreen(display));
	if (hasExtension(extensions, "GLX_EXT_swap_control")) {
		using Fn = void (*)(Display*, GLXDrawable, int);
		if (auto fn = procOf<Fn>("glXSwapIntervalEXT")) {
			fn(display, glXGetCurrentDrawable(), interval);
			return true;
		}
	}
	if (hasExtension(extensions, "GLX_MESA_swap_control")) {
		using Fn = int (*)(unsigned);
		if (auto fn = procOf<Fn>("glXSwapIntervalMESA"))
			return fn(static_cast<unsigned>(interval)) == 0;
	}
	if (hasExtension(extensions, "GLX_SGI_swap_control")) {
		using Fn = int (*)(int);
		if (auto fn = procOf<Fn>("glXSwapIntervalSGI"))
			return fn(interval) == 0; // SGI refuses 0: the driver's default stays
	}
	return false;
}
#else
bool setSwapInterval(int) { return false; }
#endif
} // namespace

namespace FrameRate {

void set(int frameRate) { gFrameRate = frameRate; }

void idle() {
	const int interval = gFrameRate == FRAME_RATE_MONITOR ? 1 : 0;
	if (interval != gAppliedInterval) {
		if (!setSwapInterval(interval))
			LOG_WARNINGF("display", "No swap interval control: the frame rate is the driver's (wanted %d)", interval);
		gAppliedInterval = interval; // tried once a change
	}
	const int now = GameClock::now();
	if (!frameRedrawDue(gLastDrawMs, now, gFrameRate)) {
		std::this_thread::sleep_for(std::chrono::milliseconds(IDLE_SLEEP_MS));
		return;
	}
	gLastDrawMs = now;
	glutPostRedisplay();
}

} // namespace FrameRate
