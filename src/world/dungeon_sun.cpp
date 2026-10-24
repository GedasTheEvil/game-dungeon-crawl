// Sunlight mirrors (plan sunlight-mirrors): the beams of the light shafts, the mirrors that turn them, the sun discs
// they light. The tracing is sun_beam.h's, shared with the checker; drawn in dungeon_render_sun.cpp.
#include "dungeon.h"
#include "dungeon_rules.h"
#include "world_events.h"
#include "../core/gameplay_config.h"
#include <algorithm>
#include <cmath>

namespace {
float vectorDeg(float x, float y) { return std::atan2(-x, y) * 57.29578f; } // from up, counter-clockwise
} // namespace

void Dungeon::traceSunBeams() {
	sunBeams = traceBeams(map, [this](int cell) { return blocksBeam(map[cell]); });
	std::fill(sunlit.begin(), sunlit.end(), 0);
	for (const BeamStep& s : sunBeams)
		sunlit[static_cast<size_t>(s.cell)] = 1;
}
//======================================================================================
void Dungeon::updateSunBeams() {
	const int now = GameClock::now();
	mirrorSwings.erase(
		std::remove_if(
			mirrorSwings.begin(),
			mirrorSwings.end(),
			[now](const MirrorSwing& m) { return now - m.startMs >= MIRROR_TURN_MS; }
		),
		mirrorSwings.end()
	);
	litDiscs.erase(
		std::remove_if(
			litDiscs.begin(),
			litDiscs.end(),
			[now](const Motion& m) { return now - m.startMs >= SUN_DISC_FLARE_MS; }
		),
		litDiscs.end()
	);
	traceSunBeams();
	for (const BeamStep& s : sunBeams) {
		Fixture& disc = map[s.cell].fixture;
		if (disc.type != SunDisc || !isLockColour(disc.attr) || sunDiscLit(disc))
			continue;
		lightSunDisc(disc);
		litDiscs.push_back({s.cell, now});
		sim.events->Play(WorldSound::SunDisc);
		openGates(disc.attr);
		sim.events->Status("The Aten blazes. Somewhere a %s gate grinds open", lockColour(disc.attr).gem);
		sim.events->Note(FieldNote::SunMirrors);
	}
	for (Monster& mon : monsters) {
		const auto col = static_cast<int>(std::floor(mon.CentreX()));
		if (mon.Active() && mon.Alive() && !mon.lurking() && mon.Type()->sunBurns && Sunlit(col, mon.Row()))
			mon.Kindle(SUN_BURN_FIRE);
	}
}
//======================================================================================
bool Dungeon::TurnMirror() {
	const int cell = MapIndex(static_cast<int>(mapX), static_cast<int>(mapY));
	if (!IsInBounds(static_cast<int>(mapX), static_cast<int>(mapY)) || map[cell].type != Mirror)
		return false;
	const float from = mirrorAngle(cell);
	mirrorSwings.erase(
		std::remove_if(
			mirrorSwings.begin(),
			mirrorSwings.end(),
			[cell](const MirrorSwing& m) { return m.cell == cell; }
		),
		mirrorSwings.end()
	);
	turnMirror(map[cell]);
	traceSunBeams(); // the face's new angle follows the beam it now sends
	mirrorSwings.push_back({cell, GameClock::now(), from});
	sim.events->Play(WorldSound::MirrorTurn);
	sim.events->Note(FieldNote::SunMirrors);
	return true;
}
//======================================================================================
bool Dungeon::SetMirror(int col, int row, int dir) {
	if (!IsInBounds(col, row) || MapAt(col, row).type != Mirror)
		return false;
	map[MapIndex(col, row)].value = static_cast<int>(beamDir(dir));
	traceSunBeams();
	return true;
}
//======================================================================================
// The face's normal halves the angle between the beam coming in (reversed) and going out: edge-on to a beam it lets
// by, facing one it catches.
float Dungeon::mirrorFaceDeg(int cell) const {
	const BeamDir out = mirrorDir(map[cell]);
	BeamDir in = BeamDir::Down; // the sun is above
	for (const BeamStep& s : sunBeams)
		if (s.cell == cell)
			in = s.in;
	const auto ix = static_cast<float>(beamDx(in)), iy = static_cast<float>(beamDy(in));
	if (out == in)
		return vectorDeg(-iy, ix);
	return vectorDeg(static_cast<float>(beamDx(out)) - ix, static_cast<float>(beamDy(out)) - iy);
}
//======================================================================================
float Dungeon::mirrorAngle(int cell) const {
	const float to = mirrorFaceDeg(cell);
	for (const MirrorSwing& m : mirrorSwings)
		if (m.cell == cell) {
			const float p = motionProgress(m.startMs, MIRROR_TURN_MS);
			if (p >= 1.f)
				break;
			const float turn = std::remainder(to - m.fromDeg, 360.f); // the short way round
			return m.fromDeg + turn * p * p * (3.f - 2.f * p);
		}
	return to;
}
