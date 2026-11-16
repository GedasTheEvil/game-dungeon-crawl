// The Stargate's dial (plan stargate-chamber): the chevrons' timing, the track's turn and the glow.
#include "stargate_dial.h"
#include "../core/timer.h"
#include "../level/gameplay_world.h"
#include <algorithm>
#include <cmath>

namespace {
// The dial's steps, in order: a chevron's turn each (the seventh's too), the seventh failing, the end.
constexpr int STEP_FAIL = STARGATE_CHEVRONS;
constexpr int STEP_END = STARGATE_CHEVRONS + 1;

int stepMs(int step) {
	if (step < STARGATE_CHEVRONS)
		return step * STARGATE_CHEVRON_MS;
	return step == STEP_FAIL ? STARGATE_FAIL_AT_MS : STARGATE_DIAL_MS;
}

// The track's turn for chevron k: back and forth, a little further each time.
float turnDeg(int chevron) { return (chevron % 2 == 0 ? 1.f : -1.f) * (90.f + 23.f * static_cast<float>(chevron)); }
} // namespace

void StargateDial::Start(int at) {
	if (Dialling())
		return;
	cell = at;
	startMs = GameClock::now();
	step = 0;
}
std::vector<StargateDial::Step> StargateDial::Update() {
	std::vector<Step> due;
	if (!Dialling())
		return due;
	const int elapsed = GameClock::now() - startMs;
	while (step <= STEP_END && elapsed >= stepMs(step)) {
		const int now = step++;
		if (now < STARGATE_CHEVRONS - 1)
			due.push_back(Step::ChevronLock);
		else if (now == STARGATE_CHEVRONS - 1)
			due.push_back(Step::ChevronFail);
		else if (now == STEP_FAIL)
			due.push_back(Step::Failed);
		else {
			restDeg = std::fmod(restDeg + trackDeg(STARGATE_DIAL_MS), 360.f);
			cell = -1;
			startMs = 0;
			step = 0;
			break;
		}
	}
	return due;
}
float StargateDial::trackDeg(int elapsedMs) {
	float deg = 0.f;
	for (int k = 0; k < STARGATE_CHEVRONS; k++) {
		const float t = std::clamp(
			static_cast<float>(elapsedMs - k * STARGATE_CHEVRON_MS) / STARGATE_TURN_MS,
			0.f,
			1.f
		);
		deg += turnDeg(k) * t * t * (3.f - 2.f * t); // eased in and out
	}
	return deg;
}
float StargateDial::TrackDeg(int at) const {
	if (cell != at)
		return restDeg;
	return restDeg + trackDeg(GameClock::now() - startMs);
}
float StargateDial::ChevronGlow(int at, int chevron) const {
	if (cell != at || chevron < 0 || chevron >= STARGATE_CHEVRONS)
		return 0.f;
	const int elapsed = GameClock::now() - startMs;
	const int lockAt = chevron * STARGATE_CHEVRON_MS + STARGATE_TURN_MS;
	if (elapsed < lockAt)
		return 0.f;
	if (chevron == STARGATE_CHEVRONS - 1) { // flickers and dies
		const int since = elapsed - lockAt;
		if (since >= STARGATE_FAIL_MS)
			return 0.f;
		const float left = 1.f - static_cast<float>(since) / STARGATE_FAIL_MS;
		return left * (0.5f + 0.5f * std::sin(static_cast<float>(since) * 0.045f));
	}
	const int fadeFrom = STARGATE_FAIL_AT_MS + STARGATE_FAIL_MS;
	return elapsed < fadeFrom ? 1.f : std::max(0.f, 1.f - static_cast<float>(elapsed - fadeFrom) / STARGATE_FADE_MS);
}
