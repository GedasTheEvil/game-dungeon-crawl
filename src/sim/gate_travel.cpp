// Gate travel's phases and their timing (plan dungeon-subsystems): the fade, the rings' drop and rise.
#include "gate_travel.h"
#include "dungeon_rules.h"
#include "../core/timer.h"
#include "../level/gameplay_world.h"
#include <algorithm>

void GateTravel::Start(bool exitGate) {
	phase = Phase::Out;
	startMs = GameClock::now();
	exit = exitGate;
	rings = false;
}
void GateTravel::StartRings() {
	phase = Phase::RingsDown;
	startMs = GameClock::now();
	exit = false;
	rings = true;
}
int GateTravel::phaseMs() const {
	switch (phase) {
	case Phase::RingsDown:
		return RINGS_DOWN_MS;
	case Phase::RingsUp:
		return RINGS_UP_MS;
	case Phase::Out:
	case Phase::In:
		return rings ? RING_FLASH_MS : GATE_TRAVEL_FADE_MS;
	case Phase::None:
		break;
	}
	return 0;
}
GateTravel::Phase GateTravel::Ended() const { return GameClock::now() - startMs < phaseMs() ? Phase::None : phase; }
void GateTravel::Next(Phase ended) {
	switch (ended) {
	case Phase::RingsDown:
		phase = Phase::Out;
		break;
	case Phase::Out:
		phase = Phase::In;
		break;
	case Phase::In:
		phase = rings ? Phase::RingsUp : Phase::None;
		break;
	case Phase::RingsUp:
	case Phase::None:
		phase = Phase::None;
		break;
	}
	startMs = GameClock::now();
}
float GateTravel::Fade() const {
	if (phase != Phase::Out && phase != Phase::In)
		return 0.f;
	const float t = motionProgress(startMs, phaseMs());
	return phase == Phase::Out ? t : 1.f - t;
}
float GateTravel::RingDrop(int ring) const {
	if (!rings)
		return 0.f;
	const int since = GameClock::now() - startMs;
	auto way = [](int ms) { // eased: they drop fast and settle
		const float t = std::clamp(static_cast<float>(ms) / RING_FALL_MS, 0.f, 1.f);
		return 1.f - (1.f - t) * (1.f - t);
	};
	switch (phase) {
	case Phase::RingsDown:
		return way(since - ring * RING_STAGGER_MS);
	case Phase::RingsUp:
		return 1.f - way(since - (RING_COUNT - 1 - ring) * RING_STAGGER_MS);
	case Phase::Out:
	case Phase::In:
		return 1.f;
	case Phase::None:
		break;
	}
	return 0.f;
}
