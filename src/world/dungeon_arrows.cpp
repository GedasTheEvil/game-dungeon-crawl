#include "dungeon.h"
#include "../state/game_state.h"
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
bool Dungeon::aimTarget(float x, float y, int dir, float& outX, float& outY) const {
	const auto row = static_cast<int>(std::floor(y));
	float nearest = ARROW_AIM_RANGE;
	bool found = false;
	for (const Monster& mon : monsters) {
		// A disguised mimic is left alone: aiming at it would give it away.
		if (!mon.Active() || !mon.Alive() || mon.lurking() || mon.Row() != row)
			continue;
		const float ahead = (mon.CentreX() - x) * static_cast<float>(dir);
		const float centreY = (mon.BottomY() + mon.TopY()) / 2.f;
		if (ahead <= 0.f || ahead > nearest || centreY - y > ARROW_MAX_RISE)
			continue;
		bool clear = true; // no wall between the bow and the monster
		for (auto col = static_cast<int>(std::floor(x)); col != static_cast<int>(std::floor(mon.CentreX())); col += dir)
			if (!IsInBounds(col, row) || isSolidTile(MapAt(col, row))) {
				clear = false;
				break;
			}
		if (!clear)
			continue;
		nearest = ahead;
		outX = mon.CentreX();
		outY = centreY;
		found = true;
	}
	return found;
}
//======================================================================================
void Dungeon::ShootArrow(int damage, int dir, float height) {
	const float x0 = mapX + static_cast<float>(dir) * ARROW_LAUNCH_AHEAD;
	const float y0 = mapY + height;
	float targetX = x0 + static_cast<float>(dir) * ARROW_FREE_RANGE;
	float targetY = mapY; // the floor
	aimTarget(x0, y0, dir, targetX, targetY);
	// Rise to the top of the arc, then come down through the target.
	const float dx = std::fabs(targetX - x0);
	const float dy = targetY - y0;
	const float rise = std::max(dy, 0.f) + ARROW_ARC_BASE + ARROW_ARC_PER_TILE * dx;
	const float vy = std::sqrt(2.f * ARROW_GRAVITY * rise);
	const float flight = (vy + std::sqrt(2.f * ARROW_GRAVITY * (rise - dy))) / ARROW_GRAVITY;
	arrows.push_back({x0, y0, static_cast<float>(dir) * dx / flight, vy, damage, GameClock::now()});
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
				break;
			}
			for (Monster& mon : monsters)
				if (mon.Active() && mon.Alive() && std::fabs(mon.CentreX() - x) <= ARROW_HIT_HALF_WIDTH &&
					y >= mon.BottomY() && y <= mon.TopY()) {
					mon.takeHit(a.damage);
					gone = true;
					break;
				}
		}
		it = gone ? arrows.erase(it) : it + 1;
	}
}
//======================================================================================
void Dungeon::drawArrows() {
	AnimatedModel* model = Game().assets.items.arrow.get();
	if (!model)
		return;
	const float firstCol = static_cast<float>(static_cast<int>(mapX) - 4);
	const float firstRow = static_cast<float>(static_cast<int>(mapY) - 3);
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
