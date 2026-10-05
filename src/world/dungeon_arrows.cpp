#include "dungeon.h"
#include "../state/assets.h"
#include "../entities/player.h"
#include "item_bag.h"
#include "journal.h"
#include "rng.h"
#include "world_events.h"
#include "../core/gameplay_config.h"
#include "../graphics/ink.h"
#include "../graphics/render_config.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>

namespace {
constexpr float ARROW_LAUNCH_AHEAD = 0.1f; // tiles in front of the player's centre: the bow
constexpr float ARROW_STEP_S = 0.005f;	   // hit test interval along the flight
// The held bow is drawn 12 units tall for its 1.2 m (ITEM_DEFS in assets.cpp, items.py); arrow.md3 is in metres.
constexpr float ARROW_WORLD_PER_METRE = 10.f;
constexpr float ARROW_DEPTH = -18.f; // just in front of the monsters (-20)
constexpr float RAD_TO_DEG = 57.29578f;
} // namespace

float Dungeon::Arrow::Y() const { return y0 + vy * t - ARROW_GRAVITY * t * t / 2.f; }
//======================================================================================
bool Dungeon::aimTarget(float x, float y, int dir, float range, float& outX, float& outY) const {
	const auto row = static_cast<int>(std::floor(y));
	float nearest = range;
	bool found = false;
	for (const Monster& mon : monsters) {
		// A disguised mimic is left alone: aiming at it would give it away.
		if (!mon.Active() || !mon.Alive() || mon.lurking() || mon.Row() != row)
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
void Dungeon::ShootArrow(int damage, const DamageMix& mix, int dir, float height, float aimRange) {
	const float x0 = mapX + static_cast<float>(dir) * ARROW_LAUNCH_AHEAD;
	const float y0 = mapY + height;
	float targetX = x0 + static_cast<float>(dir) * ARROW_FREE_RANGE;
	float targetY = mapY; // the floor
	aimTarget(x0, y0, dir, aimRange, targetX, targetY);
	// Rise to the top of the arc, then come down through the target.
	const float dx = std::fabs(targetX - x0);
	const float dy = targetY - y0;
	const float rise = std::max(dy, 0.f) + ARROW_ARC_BASE + ARROW_ARC_PER_TILE * dx;
	const float vy = std::sqrt(2.f * ARROW_GRAVITY * rise);
	const float flight = (vy + std::sqrt(2.f * ARROW_GRAVITY * (rise - dy))) / ARROW_GRAVITY;
	arrows.push_back({x0, y0, static_cast<float>(dir) * dx / flight, vy, damage, mix, GameClock::now()});
}
//======================================================================================
void Dungeon::updateArrows() {
	const int now = GameClock::now();
	for (auto it = arrows.begin(); it != arrows.end();) {
		Arrow& a = *it;
		if (a.stuckMs >= 0) {
			it = now - a.stuckMs >= ARROW_STUCK_MS ? arrows.erase(it) : it + 1;
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
			if (isSolidTile(MapAt(col, row))) {
				a.stuckMs = now; // the tip in the wall or the floor
				sim.events->Play(WorldSound::ArrowWall);
				break;
			}
			for (Monster& mon : monsters)
				if (mon.Active() && mon.Alive() && x >= mon.Left() - ARROW_HIT_TOLERANCE &&
					x <= mon.Right() + ARROW_HIT_TOLERANCE && y >= mon.BottomY() && y <= mon.TopY()) {
					// The water takes the arrow's force: a monster in it is hit for a share.
					playerHit(mon, mon.InWater() ? a.damage * ARROW_WATER_DAMAGE_PCT / 100 : a.damage, &a.mix);
					sim.events->Play(WorldSound::ArrowHit);
					gone = true;
					break;
				}
		}
		it = gone ? arrows.erase(it) : it + 1;
	}
}
//======================================================================================
void Dungeon::drawArrows() {
	AnimatedModel* model = sim.assets->items.arrow.get();
	if (!model)
		return;
	const auto firstCol = static_cast<float>(view().firstCol()); // DrawMonsters' frame
	const auto firstRow = static_cast<float>(view().originRow);
	const float scale = ARROW_WORLD_PER_METRE * Ink::figureScale();
	const float length = model->YRange(0).second * scale;
	for (const Arrow& a : arrows) {
		const float heading = std::atan2(a.vy - ARROW_GRAVITY * a.t, a.vx) * RAD_TO_DEG;
		glPushMatrix();
		glTranslatef(RenderConfig::TILE_SIZE * (a.X() - firstCol), RenderConfig::TILE_SIZE * (a.Y() - firstRow),
					 ARROW_DEPTH);
		glRotatef(heading - 90.f, 0, 0, 1); // the model points up (+y), the tip at the flight position
		glTranslatef(0, -length, 0);
		glScalef(scale, scale, scale);
		model->Show();
		glPopMatrix();
	}
}
