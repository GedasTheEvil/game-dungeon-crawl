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

namespace {
constexpr float DART_STEP_S = 0.005f; // hit test interval along the flight, as an arrow's
} // namespace

bool Dungeon::plateLoaded(int col, int row) const {
	const JumpState& jump = sim.player->jump;
	if (sim.player->Alive() && !jump.jumping && !jump.falling && static_cast<int>(mapX) == col &&
		static_cast<int>(mapY) == row && PLAYER_WEIGHT >= DART_PLATE_WEIGHT)
		return true;
	for (const Monster& mon : monsters)
		if (mon.Active() && mon.Alive() && !mon.flies() && !mon.jumping() && !mon.Hidden() && mon.Row() == row &&
			static_cast<int>(std::floor(mon.CentreX())) == col && mon.Type()->weight >= DART_PLATE_WEIGHT)
			return true;
	return false;
}
//======================================================================================
// The click, then the volley from the nearer wall on the row within DART_RANGE (the left one on a tie); with no wall
// in range only the click.
void Dungeon::pressPlate(int col, int row, bool byPlayer) {
	setPlatePressed(map[MapIndex(col, row)], true);
	pressedPlates.push_back({MapIndex(col, row), GameClock::now()});
	sim.events->Play(WorldSound::PlateClick);
	if (byPlayer)
		sim.events->Note(FieldNote::DartTraps);
	int best = 0;
	for (int k = 1; k <= DART_RANGE && best == 0; k++)
		for (int dir : {-1, 1})
			if (best == 0 && (!IsInBounds(col + dir * k, row) || isSolidTile(MapAt(col + dir * k, row))))
				best = dir * k;
	if (best == 0 || !IsInBounds(col + best, row))
		return;
	// The darts fly away from the wall: from its face towards the plate.
	const int dir = best < 0 ? 1 : -1;
	const auto face = static_cast<float>(best < 0 ? col + best + 1 : col + best);
	volleys.push_back({GameClock::now(), face, row, dir, 0});
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
			darts.push_back(
				{v.fromX + static_cast<float>(v.dir) * 0.02f, static_cast<float>(v.row) + DART_HEIGHT, v.dir, now});
			sim.events->Play(WorldSound::Dart);
			v.fired++;
		}
		it = v.fired >= DART_COUNT ? volleys.erase(it) : it + 1;
	}
}
//======================================================================================
// Like the venom: straight along the row, small steps so it cannot skip a body; the first one in its way takes it.
void Dungeon::updateDarts() {
	const int now = GameClock::now();
	const float half = sim.player->HalfWidth();
	const float height = sim.player->Height();
	for (auto it = darts.begin(); it != darts.end();) {
		Dart& d = *it;
		if (d.stuckMs >= 0) {
			it = now - d.stuckMs >= DART_STICK_MS ? darts.erase(it) : it + 1;
			continue;
		}
		float left = static_cast<float>(std::min(now - d.lastMs, 100)) / 1000.f;
		d.lastMs = now;
		bool gone = false;
		while (left > 0.f && d.stuckMs < 0 && !gone) {
			const float dt = std::min(left, DART_STEP_S);
			left -= dt;
			d.x += static_cast<float>(d.dir) * DART_SPEED * dt;
			const auto col = static_cast<int>(std::floor(d.x)), row = static_cast<int>(std::floor(d.y));
			if (!IsInBounds(col, row)) {
				gone = true;
				break;
			}
			if (isSolidTile(MapAt(col, row))) {
				d.stuckMs = now;
				sim.events->Play(WorldSound::ArrowWall);
				break;
			}
			if (sim.player->Alive() && std::fabs(d.x - mapX) <= half && d.y >= mapY && d.y <= mapY + height) {
				sim.player->TakeHit(DART_DAMAGE, DART_ATTACK_MIX, *sim.events);
				sim.player->Poison(PoisonTier::Medium, *sim.events, sim.random->gameplay);
				gone = true;
				break;
			}
			for (Monster& mon : monsters)
				if (mon.Active() && mon.Alive() && !mon.Hidden() && !mon.Emerging() && d.x >= mon.Left() &&
					d.x <= mon.Right() && d.y >= mon.BottomY() && d.y <= mon.TopY()) {
					mon.TrapHit(DART_DAMAGE);
					mon.TakePoison(PoisonTier::Medium, false, sim.random->gameplay); // no XP for a trap's kill
					sim.events->Play(WorldSound::ArrowHit);
					gone = true;
					break;
				}
		}
		it = gone ? darts.erase(it) : it + 1;
	}
}
