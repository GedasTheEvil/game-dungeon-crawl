// The mechanisms' live state (plan dungeon-subsystems): the gates' motion, the plates' weighing, the sun's beams.
#include "mechanisms.h"
#include "dungeon_rules.h"
#include "../../core/timer.h"
#include "../../level/tuning/gameplay_world.h"
#include <algorithm>
#include <cmath>

namespace {
float vectorDeg(float x, float y) { return std::atan2(-x, y) * 57.29578f; } // from up, counter-clockwise

// The motion that started at startMs, as a start for the other way from the same height (the smoothstep is
// symmetric).
int reversedStart(int startMs, int now) {
	const float p = motionProgress(startMs, GATE_OPEN_MS);
	return now - static_cast<int>((1.f - p) * static_cast<float>(GATE_OPEN_MS));
}

// Drops the motions in `motions` whose time has run (durationMs), after done(cell) for each.
template <typename Done> void finish(std::vector<CellMotion>& motions, int durationMs, Done done) {
	for (auto it = motions.begin(); it != motions.end();) {
		if (motionProgress(it->startMs, durationMs) >= 1.f) {
			done(it->cell);
			it = motions.erase(it);
		} else
			++it;
	}
}
} // namespace

void GateMotions::Clear() {
	opening.clear();
	closing.clear();
}
void GateMotions::StartOpening(Tile* map, int cell, int startMs) {
	if (map[cell].type != Gate || gateState(map[cell]) != GateState::Closed)
		return;
	setGateState(map[cell], GateState::Opening);
	opening.push_back({cell, startMs});
}
void GateMotions::OpenPlate(Tile* map, int cell, int now) {
	Tile& gate = map[cell];
	if (gateState(gate) == GateState::Closed) {
		StartOpening(map, cell, now);
		return;
	}
	if (gateState(gate) != GateState::Closing)
		return;
	for (auto it = closing.begin(); it != closing.end(); ++it)
		if (it->cell == cell) {
			const int start = reversedStart(it->startMs, now);
			closing.erase(it);
			setGateState(gate, GateState::Opening);
			opening.push_back({cell, start});
			return;
		}
}
void GateMotions::ClosePlate(Tile* map, int cell, int now) {
	Tile& gate = map[cell];
	if (gateState(gate) == GateState::Open) {
		setGateState(gate, GateState::Closing);
		closing.push_back({cell, now});
		return;
	}
	if (gateState(gate) != GateState::Opening)
		return;
	for (auto it = opening.begin(); it != opening.end(); ++it)
		if (it->cell == cell) {
			const int start = reversedStart(it->startMs, now);
			opening.erase(it);
			setGateState(gate, GateState::Closing);
			closing.push_back({cell, start});
			return;
		}
}
void GateMotions::FinishOpened(Tile* map) {
	finish(opening, GATE_OPEN_MS, [map](int cell) { setGateState(map[cell], GateState::Open); });
}
void GateMotions::FinishClosed(Tile* map) {
	finish(closing, GATE_OPEN_MS, [map](int cell) { setGateState(map[cell], GateState::Closed); });
}
void GateMotions::DelayOpening(int cell, int startMs) {
	for (CellMotion& m : opening)
		if (m.cell == cell)
			m.startMs = startMs;
}
void GateMotions::HurryOpening(int now) {
	for (CellMotion& m : opening)
		m.startMs = std::min(m.startMs, now);
}
float GateMotions::Lift(int cell, GateState state) const {
	if (state == GateState::Open)
		return 1.f;
	if (state != GateState::Opening && state != GateState::Closing)
		return 0.f;
	// A plate gate comes down the way it went up (plan pressure-plate).
	const bool up = state == GateState::Opening;
	float lift = 0.f;
	for (const CellMotion& m : up ? opening : closing)
		if (m.cell == cell) {
			const float p = motionProgress(m.startMs, GATE_OPEN_MS);
			const float s = p * p * (3.f - 2.f * p);
			lift = up ? s : 1.f - s;
		}
	return lift;
}

void PressurePlates::Reset() {
	std::fill(down.begin(), down.end(), 0);
	settled = false;
	opened = 0;
}
PressurePlates::Weighed PressurePlates::Weigh(const Tile* map, const std::function<int(int, int)>& weightOn) {
	Weighed w;
	w.held.fill(true);
	for (int cell = 0; cell < LEVEL_CELL_COUNT; cell++) {
		const Fixture& plate = map[cell].fixture;
		if (plate.type != PressurePlate || !isLockColour(plate.attr))
			continue;
		const bool isDown = weightOn(cell % LEVEL_WIDTH, cell / LEVEL_WIDTH) >= std::max(1, plate.value);
		w.clicked = w.clicked || (isDown != (down[static_cast<size_t>(cell)] != 0));
		down[static_cast<size_t>(cell)] = isDown ? 1 : 0;
		w.plates[plate.attr] = true;
		w.held[plate.attr] = w.held[plate.attr] && isDown;
	}
	w.clicked = w.clicked && settled;
	w.settled = settled;
	settled = true;
	return w;
}

void SunMirrors::Clear() {
	swings.clear();
	flares.clear();
}
void SunMirrors::Trace(const Tile* map) {
	beams = traceBeams(map, [map](int cell, BeamDir way) { return blocksBeam(map[cell], way); });
	std::fill(lit.begin(), lit.end(), 0);
	for (const BeamStep& s : beams)
		lit[static_cast<size_t>(s.cell)] = 1;
}
void SunMirrors::Expire(int now) {
	std::erase_if(swings, [now](const MirrorSwing& m) { return now - m.startMs >= MIRROR_TURN_MS; });
	std::erase_if(flares, [now](const CellMotion& m) { return now - m.startMs >= SUN_DISC_FLARE_MS; });
}
float SunMirrors::Flare(int cell) const {
	float flare = 0.f;
	for (const CellMotion& m : flares)
		if (m.cell == cell)
			flare = 1.f - motionProgress(m.startMs, SUN_DISC_FLARE_MS);
	return flare;
}
void SunMirrors::Swing(int cell, float fromDeg) {
	std::erase_if(swings, [cell](const MirrorSwing& m) { return m.cell == cell; });
	swings.push_back({cell, GameClock::now(), fromDeg});
}
void SunMirrors::MoveSwing(int from, int to) {
	for (MirrorSwing& m : swings)
		if (m.cell == from)
			m.cell = to;
}
// The face's normal halves the angle between the beam coming in (reversed) and going out: edge-on to a beam it lets
// by, facing one it catches.
float SunMirrors::FaceDeg(const Tile* map, int cell) const {
	const BeamDir out = mirrorDir(map[cell]);
	BeamDir in = BeamDir::Down; // the sun is above
	for (const BeamStep& s : beams)
		if (s.cell == cell)
			in = s.in;
	const auto ix = static_cast<float>(beamDx(in)), iy = static_cast<float>(beamDy(in));
	if (out == in)
		return vectorDeg(-iy, ix);
	return vectorDeg(static_cast<float>(beamDx(out)) - ix, static_cast<float>(beamDy(out)) - iy);
}
float SunMirrors::Angle(const Tile* map, int cell) const {
	const float to = FaceDeg(map, cell);
	for (const MirrorSwing& m : swings)
		if (m.cell == cell) {
			const float p = motionProgress(m.startMs, MIRROR_TURN_MS);
			if (p >= 1.f)
				break;
			const float turn = std::remainder(to - m.fromDeg, 360.f); // the short way round
			return m.fromDeg + turn * p * p * (3.f - 2.f * p);
		}
	return to;
}
