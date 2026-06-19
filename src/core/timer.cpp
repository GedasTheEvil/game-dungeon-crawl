#include "timer.h"
#include <SDL/SDL_timer.h>
#include <SDL/SDL.h>

timer::timer() {
	time_start = SDL_GetTicks();
	ticks = CDefTime;
}

timer::timer(int defT) {
	time_start = SDL_GetTicks();
	ticks = defT;
}

timer::~timer() {}

bool timer::TimePassed() {
	int xxx = SDL_GetTicks();
	if (xxx - time_start >= ticks) {
		time_start = SDL_GetTicks();
		return 1;
	}
	return 0;
}

bool timer::TimePassed(bool noRepeat) {
	int xxx = SDL_GetTicks();
	if (xxx - time_start >= ticks) {
		if (!noRepeat)
			time_start = SDL_GetTicks();
		return 1;
	}
	return 0;
}

void timer::Reset() { time_start = SDL_GetTicks(); }