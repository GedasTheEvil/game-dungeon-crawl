// The bite's reach and timing (plan monster-attack-reach): a cone from the mouth along the way it bites, checked when
// the bite lands, so a jump over a low monster or a step back in time makes it miss.
#include "monster.h"
#include <algorithm>
#include <array>
#include <cmath>
#include "player.h"
#include "../world/journal.h"
#include "../core/gameplay_config.h"
#include "../graphics/render_config.h"

namespace {
struct Point {
	float x = 0.f, y = 0.f;
};
// A convex polygon: a box cut by two half-planes has at most 6 corners.
struct Polygon {
	std::array<Point, 8> points{};
	size_t size = 0;
	void add(Point p) { points[size++] = p; }
};

// The part of in where a * x + b * y <= 0 (one Sutherland-Hodgman step).
Polygon clip(const Polygon& in, float a, float b) {
	Polygon out;
	for (size_t i = 0; i < in.size; i++) {
		const Point p = in.points[i];
		const Point q = in.points[(i + 1) % in.size];
		const float dp = a * p.x + b * p.y;
		const float dq = a * q.x + b * q.y;
		if (dp <= 0.f)
			out.add(p);
		if ((dp <= 0.f) != (dq <= 0.f)) {
			const float t = dp / (dp - dq);
			out.add({p.x + (q.x - p.x) * t, p.y + (q.y - p.y) * t});
		}
	}
	return out;
}

float distanceToSegment(Point p, Point q) { // from the origin
	const float dx = q.x - p.x;
	const float dy = q.y - p.y;
	const float len2 = dx * dx + dy * dy;
	const float t = len2 > 0.f ? std::clamp(-(p.x * dx + p.y * dy) / len2, 0.f, 1.f) : 0.f;
	return std::hypot(p.x + dx * t, p.y + dy * t);
}

// The box [x0, x1] x [y0, y1] touches the cone from the origin along +x, length long, halfAngleDeg wide either way.
bool boxTouchesCone(float x0, float x1, float y0, float y1, float length, float halfAngleDeg) {
	if (x0 <= 0.f && x1 >= 0.f && y0 <= 0.f && y1 >= 0.f)
		return true;
	const float slope = std::tan(halfAngleDeg * 3.14159265f / 180.f);
	Polygon box;
	for (const Point p : {Point{x0, y0}, Point{x1, y0}, Point{x1, y1}, Point{x0, y1}})
		box.add(p);
	const Polygon wedge = clip(clip(box, -slope, 1.f), -slope, -1.f); // |y| <= x * slope
	float nearest = INFINITY;
	for (size_t i = 0; i < wedge.size; i++)
		nearest = std::min(nearest, distanceToSegment(wedge.points[i], wedge.points[(i + 1) % wedge.size]));
	return nearest <= length;
}
} // namespace

Monster::BiteCone Monster::Reach() const {
	const MeleeReach& melee = type->melee;
	const float gap = rooted() ? ROOTED_BITE_REACH : MONSTER_BITE_REACH;
	if (!type->isBoss())
		return {gap + melee.slack, melee.halfAngleDeg, melee.mouthY};
	return {(gap + melee.slack) * BOSS_MELEE_REACH_FACTOR, std::max(melee.halfAngleDeg, BOSS_MELEE_REACH_HALF_ANGLE),
			melee.mouthY};
}

bool Monster::Reaches(float px, float py, int dir) const {
	const Player& player = *links.player;
	const float left = px - player.HalfWidth();
	const float right = px + player.HalfWidth();
	const float top = py + player.Height();
	// Its body off the row's floor, not down in a water basin: the player's box does not sink either.
	const float bottom = BottomY() + sink / RenderConfig::TILE_SIZE;
	const float height = TopY() - BottomY();
	if (left <= Right() && right >= Left() && py <= bottom + height && top >= bottom)
		return true; // in its body: no safe spot inside it
	if (dir == 0)
		return false;
	const BiteCone cone = Reach();
	const float mouthX = dir > 0 ? Right() : Left();
	const float mouthY = bottom + cone.mouthY * height;
	const float near = dir > 0 ? left - mouthX : mouthX - right;
	const float far = dir > 0 ? right - mouthX : mouthX - left;
	return boxTouchesCone(near, far, py - mouthY, top - mouthY, cone.length, cone.halfAngleDeg);
}

void Monster::FollowBite(bool blocked, float px, float py) {
	if (!blocked && !rooted() && attackDirection(px, py) == biteDir)
		x += WalkStep() * static_cast<float>(biteDir);
}

void Monster::UpdateBite(float px, float py) {
	if (GameClock::now() < biteAtMs)
		return;
	biteAtMs = -1;
	const int dir = biteDir;
	biteDir = 0;
	if (!links.player->Alive())
		return;
	if (Reaches(px, py, dir)) {
		bite();
		return;
	}
	sound(CharacterSound::Attack); // the jaws snap at the air
	if (links.player->jump.jumping)
		links.events->Note(FieldNote::JumpedBite);
}
