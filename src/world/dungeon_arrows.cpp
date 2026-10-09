#include "dungeon.h"
#include "dungeon_rules.h"
#include "../entities/player.h"
#include "item_bag.h"
#include "journal.h"
#include "rng.h"
#include "world_events.h"
#include "../core/gameplay_config.h"
#include "../graphics/render_config.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr float ARROW_LAUNCH_AHEAD = 0.1f; // tiles in front of the player's centre: the bow
constexpr float ARROW_STEP_S = 0.005f;	   // hit test interval along the flight
} // namespace

float Dungeon::Missile::Y() const { return y0 + vy * t - ARROW_GRAVITY * t * t / 2.f; }
//======================================================================================
bool Dungeon::aimTarget(float x, float y, int dir, float range, float& outX, float& outY) const {
	const auto row = static_cast<int>(std::floor(y));
	float nearest = range;
	bool found = false;
	for (const Monster& mon : monsters) {
		// A disguised mimic is left alone: aiming at it would give it away.
		if (!mon.Active() || !mon.Alive() || mon.lurking() || mon.Hidden() || mon.Row() != row)
			continue;
		const float near = mon.NearEdge(dir);
		const float ahead = (near - x) * static_cast<float>(dir);
		const float centreY = (mon.BottomY() + mon.TopY()) / 2.f;
		if (ahead <= 0.f || ahead > nearest || centreY - y > ARROW_MAX_RISE)
			continue;
		bool clear = true; // no wall between the bow and the monster
		for (auto col = static_cast<int>(std::floor(x)); col != static_cast<int>(std::floor(near)); col += dir)
			if (!IsInBounds(col, row) || isSolidTile(MapAt(col, row))) {
				clear = false;
				break;
			}
		if (!clear)
			continue;
		nearest = ahead;
		outX = near;
		outY = centreY;
		found = true;
	}
	return found;
}
//======================================================================================
void Dungeon::Shoot(MissileKind kind, int damage, const DamageMix& mix, int dir, float height, float aimRange) {
	const float x0 = mapX + static_cast<float>(dir) * ARROW_LAUNCH_AHEAD;
	const float y0 = mapY + height - PlayerSink() / RenderConfig::TILE_SIZE; // wading, the bow is down in the water
	float targetX = x0 + static_cast<float>(dir) * std::min(ARROW_FREE_RANGE, aimRange);
	float targetY = mapY; // the floor
	aimTarget(x0, y0, dir, aimRange, targetX, targetY);
	// Rise to the top of the arc, then come down through the target.
	const float dx = std::fabs(targetX - x0);
	const float dy = targetY - y0;
	float rise = std::max(dy, 0.f) + rulesOf(kind).arc * (ARROW_ARC_BASE + ARROW_ARC_PER_TILE * dx);
	auto speeds = [&](float r, float& vx, float& vy) {
		vy = std::sqrt(2.f * ARROW_GRAVITY * r);
		const float flight = (vy + std::sqrt(2.f * ARROW_GRAVITY * (r - dy))) / ARROW_GRAVITY;
		vx = dx / flight;
	};
	float vx = 0.f, vy = 0.f;
	speeds(rise, vx, vy);
	// A target down in a water basin, past dry floor: lob the arrow steeply enough to clear the basin's edge.
	const auto row = static_cast<int>(std::floor(mapY));
	const auto floorY = static_cast<float>(row);
	if (targetY < floorY && !inHalfWater(MapAt(static_cast<int>(std::floor(x0)), row))) {
		for (auto col = static_cast<int>(std::floor(x0)); col != static_cast<int>(std::floor(targetX)); col += dir) {
			if (!inHalfWater(MapAt(col + dir, row)))
				continue;
			const float edge = std::fabs(static_cast<float>(dir > 0 ? col + 1 : col) - x0);
			for (int k = 0; k < 40; k++) {
				const float t = edge / vx;
				if (y0 + vy * t - ARROW_GRAVITY * t * t / 2.f >= floorY + ARROW_EDGE_CLEARANCE)
					break;
				rise += ARROW_LOB_STEP;
				speeds(rise, vx, vy);
			}
			break;
		}
	}
	missiles.push_back({kind, x0, y0, static_cast<float>(dir) * vx, vy, damage, mix, GameClock::now()});
}
//======================================================================================
void Dungeon::updateMissiles() {
	const int now = GameClock::now();
	for (auto it = missiles.begin(); it != missiles.end();) {
		Missile& a = *it;
		const MissileRules& rules = rulesOf(a.kind);
		if (a.stuckMs >= 0) {
			const int lasts = rules.returns ? STICK_RETURN_MS : ARROW_STUCK_MS;
			it = now - a.stuckMs >= lasts ? missiles.erase(it) : it + 1;
			continue;
		}
		const float flown = static_cast<float>(now - a.startMs) / 1000.f;
		bool gone = false;
		while (a.t < flown && a.stuckMs < 0 && !gone) {
			a.t = std::min(flown, a.t + ARROW_STEP_S);
			const float x = a.X(), y = a.Y();
			const auto col = static_cast<int>(std::floor(x)), row = static_cast<int>(std::floor(y));
			if (!IsInBounds(col, row)) {
				gone = true;
				break;
			}
			// Over half water the floor is the basin's, WATER_BASIN_DEPTH down into the cell below.
			const bool inBasin =
				inHalfWater(MapAt(col, row + 1)) &&
				y >= static_cast<float>(row + 1) - RenderConfig::WATER_BASIN_DEPTH / RenderConfig::TILE_SIZE;
			if (isSolidTile(MapAt(col, row)) && !inBasin) {
				a.stuckMs = now; // the tip in the wall or the floor
				sim.events->Play(rules.wall);
				gone = !rules.sticks && !rules.returns;
				break;
			}
			for (Monster& mon : monsters)
				if (mon.Active() && mon.Alive() && !mon.Hidden() && x >= mon.Left() - ARROW_HIT_TOLERANCE &&
					x <= mon.Right() + ARROW_HIT_TOLERANCE && y >= mon.BottomY() && y <= mon.TopY()) {
					// The water takes the arrow's force: a monster in it is hit for a share.
					playerHit(mon, mon.InWater() ? a.damage * ARROW_WATER_DAMAGE_PCT / 100 : a.damage, &a.mix);
					sim.events->Play(rules.hit);
					if (rules.returns)
						a.stuckMs = now;
					else
						gone = true;
					break;
				}
		}
		if (a.stuckMs >= 0) {
			a.endX = a.X();
			a.endY = a.Y();
		}
		it = gone ? missiles.erase(it) : it + 1;
	}
}
//======================================================================================
bool Dungeon::clearRow(float fromX, float toX, int row) const {
	const int dir = toX > fromX ? 1 : -1;
	for (auto col = static_cast<int>(std::floor(fromX)); col != static_cast<int>(std::floor(toX)) + dir; col += dir)
		if (!IsInBounds(col, row) || isSolidTile(MapAt(col, row)))
			return false;
	return true;
}
//======================================================================================
// At the player's chest as they stand at the spit: a jump takes them out of its line.
void Dungeon::spitVenom(const Monster& mon, float x, float y) {
	const float dx = mapX - x;
	const float dy = mapY + VENOM_CHEST * sim.player->Height() - y;
	const float len = std::max(std::hypot(dx, dy), 0.01f);
	venoms.push_back({x, y, VENOM_SPEED * dx / len, VENOM_SPEED * dy / len, mon.Type(), GameClock::now()});
}
//======================================================================================
void Dungeon::updateVenoms() {
	const int now = GameClock::now();
	const float half = sim.player->HalfWidth();
	const float height = sim.player->Height();
	for (auto it = venoms.begin(); it != venoms.end();) {
		Venom& v = *it;
		if (v.splatMs >= 0) {
			it = now - v.splatMs >= VENOM_SPLAT_MS ? venoms.erase(it) : it + 1;
			continue;
		}
		float left = static_cast<float>(std::min(now - v.lastMs, 100)) / 1000.f;
		v.lastMs = now;
		bool gone = false;
		while (left > 0.f && v.splatMs < 0 && !gone) {
			const float dt = std::min(left, ARROW_STEP_S);
			left -= dt;
			v.x += v.vx * dt;
			v.y += v.vy * dt;
			v.flown += VENOM_SPEED * dt;
			const auto col = static_cast<int>(std::floor(v.x)), row = static_cast<int>(std::floor(v.y));
			if (!IsInBounds(col, row) || v.flown > VENOM_MAX_FLIGHT) {
				gone = true;
				break;
			}
			if (isSolidTile(MapAt(col, row))) {
				v.splatMs = now;
				break;
			}
			if (sim.player->Alive() && std::fabs(v.x - mapX) <= half && v.y >= mapY && v.y <= mapY + height) {
				const SpitRules& spit = *v.from->spit;
				sim.player->TakeHit(spit.damage, v.from->attackMix, *sim.events);
				sim.player->Poison(spit.poison, *sim.events, sim.random->gameplay);
				sim.journal->HitByCreature(v.from->id, levelNumber);
				sim.journal->SeeMove(v.from->id, levelNumber, CreatureMove::Poison);
				gone = true;
			}
		}
		it = gone ? venoms.erase(it) : it + 1;
	}
}
