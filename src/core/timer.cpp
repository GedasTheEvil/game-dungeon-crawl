#include "timer.h"
#include <SDL/SDL_timer.h>
#include <SDL/SDL.h>

namespace {
bool gVirtualClock = false;
int gVirtualMs = 0;
} // namespace

void GameClock::enableVirtual() { gVirtualClock = true; }

void GameClock::advance(int ms) { gVirtualMs += ms; }

int GameClock::now() { return gVirtualClock ? gVirtualMs : static_cast<int>(SDL_GetTicks()); }

Timer::Timer() {
	time_start = GameClock::now();
	ticks = DEFAULT_TIMER_MS;
}

Timer::Timer(int defT) {
	time_start = GameClock::now();
	ticks = defT;
}

Timer::~Timer() {}

bool Timer::TimePassed() {
	int xxx = GameClock::now();
	if (xxx - time_start >= ticks) {
		time_start = GameClock::now();
		return true;
	}
	return false;
}

bool Timer::TimePassed(bool noRepeat) {
	int xxx = GameClock::now();
	if (xxx - time_start >= ticks) {
		if (!noRepeat)
			time_start = GameClock::now();
		return true;
	}
	return false;
}

void Timer::Reset() { time_start = GameClock::now(); }

int Timer::StartTime() const { return time_start; }

void Timer::SetStartTime(int start) { time_start = start; }