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

timer::timer() {
	time_start = GameClock::now();
	ticks = CDefTime;
}

timer::timer(int defT) {
	time_start = GameClock::now();
	ticks = defT;
}

timer::~timer() {}

bool timer::TimePassed() {
	int xxx = GameClock::now();
	if (xxx - time_start >= ticks) {
		time_start = GameClock::now();
		return 1;
	}
	return 0;
}

bool timer::TimePassed(bool noRepeat) {
	int xxx = GameClock::now();
	if (xxx - time_start >= ticks) {
		if (!noRepeat)
			time_start = GameClock::now();
		return 1;
	}
	return 0;
}

void timer::Reset() { time_start = GameClock::now(); }

int timer::StartTime() const { return time_start; }

void timer::SetStartTime(int start) { time_start = start; }