// Dart traps (docs/plan/poison-dart-trap.md): the plate, the volley from the wall and the darts in flight. Drawn in
// dungeon_render_mechanisms.cpp.
#include "dungeon.h"
#include "../entities/player.h"
#include "journal.h"
#include "rng.h"
#include "world_events.h"
#include "../core/gameplay_config.h"
#include <algorithm>
#include <cmath>

bool Dungeon::plateLoaded(int col, int row) const {
	const float centre = static_cast<float>(col) + 0.5f;
	const JumpState& jump = sim.player->jump;
	if (sim.player->Alive() && !jump.jumping && !jump.falling && std::fabs(mapX - centre) <= DART_PLATE_HALF_WIDTH &&
		static_cast<int>(mapY) == row && PLAYER_WEIGHT >= DART_PLATE_WEIGHT)
		return true;
	for (const Monster& mon : monsters)
		if (mon.Active() && mon.Alive() && !mon.flies() && !mon.jumping() && !mon.Hidden() && mon.Row() == row &&
			std::fabs(mon.CentreX() - centre) <= DART_PLATE_HALF_WIDTH && mon.Type()->weight >= DART_PLATE_WEIGHT)
			return true;
	return false;
}
//======================================================================================
// The click, then the volley from the holes above the plate.
void Dungeon::pressPlate(int col, int row, bool byPlayer) {
	setPlatePressed(map[MapIndex(col, row)], true);
	pressedPlates.push_back({MapIndex(col, row), GameClock::now()});
	sim.events->Play(WorldSound::PlateClick);
	if (byPlayer)
		sim.events->Note(FieldNote::DartTraps);
	volleys.push_back({GameClock::now(), col, row, 0});
}
//======================================================================================
void Dungeon::updateDartTraps() {
	const int now = GameClock::now();
	for (auto it = pressedPlates.begin(); it != pressedPlates.end();) {
		const int col = it->cell % MAP_WIDTH, row = it->cell / MAP_WIDTH;
		if (now - it->startMs >= DART_REARM_MS && !plateLoaded(col, row)) {
			setPlatePressed(map[it->cell], false);
			it = pressedPlates.erase(it);
		} else
			++it;
	}
	// Plates in reach of a walker: the player's cell and the monsters' (a plate elsewhere has no one on it).
	auto tryPress = [this](int col, int row, bool byPlayer) {
		if (IsInBounds(col, row) && MapAt(col, row).type == DartPlate && !platePressed(MapAt(col, row)) &&
			plateLoaded(col, row))
			pressPlate(col, row, byPlayer);
	};
	tryPress(static_cast<int>(mapX), static_cast<int>(mapY), true);
	for (const Monster& mon : monsters)
		if (mon.Active())
			tryPress(static_cast<int>(std::floor(mon.CentreX())), mon.Row(), false);

	for (auto it = volleys.begin(); it != volleys.end();) {
		DartVolley& v = *it;
		while (v.fired < DART_COUNT && now - v.startMs >= DART_DELAY_MS + v.fired * DART_GAP_MS) {
			darts.push_back({static_cast<float>(v.col) + 0.5f + DART_HOLE_X[v.fired],
							 static_cast<float>(v.row) + DART_HEIGHT, 0.f, now});
			sim.events->Play(WorldSound::Dart);
			v.fired++;
		}
		it = v.fired >= DART_COUNT ? volleys.erase(it) : it + 1;
	}
}
//======================================================================================
// Out of the wall towards the camera; at the walking line it takes the body in front of its hole, else it flies on out
// of the corridor and is gone.
void Dungeon::updateDarts() {
	const int now = GameClock::now();
	const float half = sim.player->HalfWidth();
	const float height = sim.player->Height();
	for (auto it = darts.begin(); it != darts.end();) {
		Dart& d = *it;
		d.depth += DART_SPEED * static_cast<float>(std::min(now - d.lastMs, 100)) / 1000.f;
		d.lastMs = now;
		bool hit = false;
		if (!d.passed && d.depth >= DART_HIT_DEPTH) {
			d.passed = true;
			if (sim.player->Alive() && std::fabs(d.x - mapX) <= half + DART_HALF_WIDTH && d.y >= mapY &&
				d.y <= mapY + height) {
				sim.player->TakeHit(DART_DAMAGE, DART_ATTACK_MIX, *sim.events);
				sim.player->Poison(PoisonTier::Medium, *sim.events, sim.random->gameplay);
				hit = true;
			}
			for (Monster& mon : monsters)
				if (!hit && mon.Active() && mon.Alive() && !mon.Hidden() && !mon.Emerging() &&
					d.x + DART_HALF_WIDTH >= mon.Left() && d.x - DART_HALF_WIDTH <= mon.Right() &&
					d.y >= mon.BottomY() && d.y <= mon.TopY()) {
					mon.TrapHit(DART_DAMAGE);
					mon.TakePoison(PoisonTier::Medium, false, sim.random->gameplay); // no XP for a trap's kill
					sim.events->Play(WorldSound::ArrowHit);
					hit = true;
				}
		}
		it = hit || d.depth >= 1.f ? darts.erase(it) : it + 1;
	}
}
