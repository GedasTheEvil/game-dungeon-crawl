// What flies or rolls on the level (plan dungeon-subsystems): the parts that need no level.
#include "projectiles.h"
#include "../level/tuning/gameplay_combat.h"
#include "../level/tuning/gameplay_monster_kinds.h"
#include <algorithm>
#include <cmath>

float Missile::Y() const { return y0 + vy * t - ARROW_GRAVITY * t * t / 2.f; }
void MonsterShot::aimAt(float tx, float ty) {
	const float dx = tx - x;
	const float dy = ty - y;
	const float len = std::max(std::hypot(dx, dy), 0.01f);
	const float c = std::cos(spread), s = std::sin(spread);
	vx = SHOT_SPEED * (c * dx - s * dy) / len;
	vy = SHOT_SPEED * (s * dx + c * dy) / len;
}
void Projectiles::Clear() {
	missiles.clear();
	shots.clear();
	balls.clear();
}
int Projectiles::ShotCount(bool big) const {
	return static_cast<int>(std::ranges::count_if(shots, [big](const MonsterShot& v) {
		return v.splatMs < 0 && (!big || v.big);
	}));
}
std::vector<float> Projectiles::StuckMissileXs() const {
	std::vector<float> xs;
	for (const Missile& a : missiles)
		if (a.stuckMs >= 0)
			xs.push_back(a.endX);
	return xs;
}
