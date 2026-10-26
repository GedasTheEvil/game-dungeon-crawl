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
constexpr float ARROW_STEP_S = 0.005f; // hit test interval along the flight
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
	// A fire arrow: FIRE_ARROW_PERCENT of its damage turns to fire.
	const bool fire = kind == MissileKind::Arrow && sim.player->fireArrows > 0;
	DamageMix dealt = mix;
	if (fire) {
		sim.player->fireArrows--;
		dealt[FIRE] = 100;
		for (size_t t = 0; t < DAMAGE_TYPE_COUNT; t++)
			if (t != FIRE) {
				dealt[t] = mix[t] * (100 - FIRE_ARROW_PERCENT) / 100;
				dealt[FIRE] -= dealt[t];
			}
		if (sim.player->fireArrows == 0)
			sim.events->Status("The last fire arrow");
	}
	// Flaming arrows (plan skill-flaming-arrows): on a bow shot not already lit by a brazier,
	// FLAMING_ARROW_BONUS_PERCENT of the damage comes on top, as fire. Too little stamina: a normal arrow, no cost, the
	// HUD sign blinks.
	Player& player = *sim.player;
	int shotDamage = damage;
	bool flaming = false;
	if (!fire && kind == MissileKind::Arrow && player.staminaSkills && player.stats.heka.Has(SkillId::FlamingArrows)) {
		const int cost = std::max(1, player.stats.MaxStamina() * FLAMING_ARROW_STAMINA_PERCENT / 100);
		if (player.stats.ConsumeStamina(cost)) {
			flaming = true;
			shotDamage = damage * (100 + FLAMING_ARROW_BONUS_PERCENT) / 100;
			// The fire's share of the new total; the rest keeps its proportions, the rounding goes to the fire.
			const int fireShare = FLAMING_ARROW_BONUS_PERCENT * 100 / (100 + FLAMING_ARROW_BONUS_PERCENT);
			dealt[FIRE] = 100;
			for (size_t t = 0; t < DAMAGE_TYPE_COUNT; t++)
				if (t != FIRE) {
					dealt[t] = mix[t] * (100 - fireShare) / 100;
					dealt[FIRE] -= dealt[t];
				}
		} else {
			player.staminaSkillRefusedMs = GameClock::now();
		}
	}
	missiles.push_back(
		{kind, x0, y0, static_cast<float>(dir) * vx, vy, shotDamage, dealt, GameClock::now(), fire || flaming}
	);
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
			const bool inBasin = inHalfWater(MapAt(col, row + 1)) &&
								 y >= static_cast<float>(row + 1) -
										  RenderConfig::WATER_BASIN_DEPTH / RenderConfig::TILE_SIZE;
			if (a.fire && y - static_cast<float>(row) < 0.5f) // low over the floor: a web there burns
				burnWeb(MapIndex(col, row));
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
// At the player's chest as they stand at the release: a jump takes them out of its line. A volley's later shots wait
// at the release point, each aimed as it leaves; a big coal goes alone, at the feet (plan babi-boss).
void Dungeon::shootAtPlayer(const Monster& mon, float x, float y) {
	const std::optional<RangedRules>& ranged = mon.Type()->ranged;
	if (!ranged)
		return;
	const int now = GameClock::now();
	const bool big = mon.BigThrow();
	int count = 1;
	if (ranged->volley && !big)
		count = sim.random->gameplay.range(ranged->volley->minCount, ranged->volley->maxCount);
	for (int k = 0; k < count; k++) {
		const int at = now + k * (ranged->volley ? ranged->volley->gapMs : 0);
		for (int f = 0; f < ranged->fan; f++) { // a fan: the middle one at the chest, the others above and below it
			MonsterShot v = {x, y, 0.f, 0.f, mon.Type(), ranged->colour, at};
			v.spawnMs = at;
			v.big = big;
			v.spread = SHOT_FAN_SPREAD * (static_cast<float>(f) - static_cast<float>(ranged->fan - 1) / 2.f);
			aimShot(v);
			shots.push_back(v);
		}
	}
	if (big)
		sim.journal->SeeMove(mon.Type()->id, levelNumber, CreatureMove::Blaze);
}
//======================================================================================
void Dungeon::aimShot(MonsterShot& v) const {
	const float dx = mapX - v.x;
	const float dy = mapY + (v.big ? 0.f : SHOT_CHEST * sim.player->Height()) - v.y;
	const float len = std::max(std::hypot(dx, dy), 0.01f);
	const float c = std::cos(v.spread), s = std::sin(v.spread);
	v.vx = SHOT_SPEED * (c * dx - s * dy) / len;
	v.vy = SHOT_SPEED * (s * dx + c * dy) / len;
}
//======================================================================================
int Dungeon::ShotCount(bool big) const {
	return static_cast<int>(std::count_if(shots.begin(), shots.end(), [big](const MonsterShot& v) {
		return v.splatMs < 0 && (!big || v.big);
	}));
}
//======================================================================================
void Dungeon::updateShots() {
	const int now = GameClock::now();
	const float half = sim.player->HalfWidth();
	const float height = sim.player->Height();
	for (auto it = shots.begin(); it != shots.end();) {
		MonsterShot& v = *it;
		if (v.splatMs >= 0) {
			it = now - v.splatMs >= SHOT_SPLAT_MS ? shots.erase(it) : it + 1;
			continue;
		}
		if (v.waiting(now)) {
			++it;
			continue;
		}
		if (v.flown == 0.f) // leaving: a volley's later shot is aimed now
			aimShot(v);
		float left = static_cast<float>(std::min(now - v.lastMs, 100)) / 1000.f;
		v.lastMs = now;
		bool gone = false;
		while (left > 0.f && v.splatMs < 0 && !gone) {
			const float dt = std::min(left, ARROW_STEP_S);
			left -= dt;
			v.x += v.vx * dt;
			v.y += v.vy * dt;
			v.flown += SHOT_SPEED * dt;
			const auto col = static_cast<int>(std::floor(v.x)), row = static_cast<int>(std::floor(v.y));
			if (!IsInBounds(col, row) || v.flown > SHOT_MAX_FLIGHT) {
				gone = true;
				break;
			}
			const std::optional<RangedRules>& rules = v.from->ranged;
			if (isSolidTile(MapAt(col, row))) {
				v.splatMs = now;
				if (v.big && rules && rules->volley) // on the floor, or at the foot of a wall
					blazeAt(v.x - v.vx * ARROW_STEP_S, v.y - v.vy * ARROW_STEP_S, rules->volley->blazeMs);
				break;
			}
			if (rules && sim.player->Alive() && std::fabs(v.x - mapX) <= half && v.y >= mapY && v.y <= mapY + height) {
				const RangedRules& ranged = *rules;
				const bool dodged = sim.player->Dodges(*sim.events, sim.random->gameplay);
				if (v.big && ranged.volley)
					blazeAt(mapX, mapY, ranged.volley->blazeMs);
				if (!dodged) {
					sim.player->TakeHit(
						v.big && ranged.volley ? ranged.volley->bigDamage : ranged.damage,
						ranged.mix.value_or(v.from->attackMix),
						*sim.events,
						false,
						ranged.burnMs
					);
					sim.journal->HitByCreature(v.from->id, levelNumber);
					if (sim.player->Curse(v.from->rangedCurses, *sim.events, sim.random->gameplay))
						sim.journal->SeeMove(v.from->id, levelNumber, CreatureMove::Curse);
					if (ranged.blindMs > 0 && sim.random->gameplay.percent(ranged.blindPercent)) {
						sim.player->Blind(ranged.blindMs, *sim.events);
						sim.journal->SeeMove(v.from->id, levelNumber, CreatureMove::Blind);
					}
				}
				if (ranged.poison && !dodged) {
					sim.player->Poison(*ranged.poison, *sim.events, sim.random->gameplay);
					sim.journal->SeeMove(v.from->id, levelNumber, CreatureMove::Poison);
				}
				gone = true;
			}
		}
		it = gone ? shots.erase(it) : it + 1;
	}
}
