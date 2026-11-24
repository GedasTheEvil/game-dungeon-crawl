// What flies or rolls on the level (plan dungeon-subsystems): the player's missiles, the monsters' shots, the dung
// balls. Not saved. Their flight against the walls, the monsters and the player is the Dungeon's (dungeon_arrows.cpp,
// dungeon_balls.cpp); drawn in dungeon_render_effects.cpp.
#ifndef PROJECTILES_H
#define PROJECTILES_H
#include "../../level/rules/damage.h"
#include "../../level/rules/items.h"
#include "../../level/rules/monster_kinds.h"
#include "../../core/rgb.h"
#include <optional>
#include <vector>

struct MonsterType;

// A missile (dungeon_arrows.cpp): an arrow, a sling stone, a throwing stick or a javelin. A parabola from the launch
// point, x0 + vx t, y0 + vy t - g t^2 / 2 in map units.
struct Missile {
	MissileKind kind;
	float x0, y0, vx, vy;
	int damage;
	DamageMix mix;
	int startMs; // GameClock time of the shot
	bool fire = false; // a fire arrow (lightArrows): it flies burning
	bool exact = false; // a test weapon's shot (plan test-weapons): 1 HP, whatever the rest says
	float t = 0.f; // seconds flown so far
	int stuckMs = -1; // GameClock time it hit a wall or the floor (or a monster: the stick turns back); < 0: flying
	float endX = 0.f, endY = 0.f; // where it stopped: the throwing stick flies back from there
	float drift = 0.f; // map units a sand gust has carried it along x (plan sandstorm-halls)
	// The weapon that shot it, for the run's damage per weapon (plan endgame-stats-skills-weapon); none: a test's shot.
	std::optional<ItemKind> weapon;
	[[nodiscard]] float X() const { return x0 + vx * t + drift; }
	[[nodiscard]] float Y() const;
};

// A monster's ranged attack in flight, venom or bolt (dungeon_arrows.cpp): a straight line from its release point,
// map units. A volley's later shots (plan babi-boss) wait at the release point until their spawnMs, then are aimed.
struct MonsterShot {
	float x, y, vx, vy;
	const MonsterType* from; // its damage, mix and poison
	Rgb colour; // RangedRules::colour
	int lastMs; // GameClock time of the last update
	float flown = 0.f; // tiles
	int splatMs = -1; // GameClock time it hit a wall or the floor; < 0: flying
	int spawnMs = 0; // GameClock time it was shot: the seed of its flames
	bool big = false; // a volley thrower's big coal (VolleyRules): at the feet, it sets the floor burning
	float spread = 0.f; // radians off the aim: a fan's side shots (RangedRules::fan)
	float damage = 1.f; // the thrower's terrain damage factor as it threw (plan terrain-affinity)
	[[nodiscard]] bool waiting(int now) const { return now < spawnMs; }
	// Aimed at (x, y) (map units), its spread turned in.
	void aimAt(float x, float y);
};

// A dung ball (plan scarab-dung-ball, dungeon_balls.cpp): kicked off by a roller (Monster::UpdateRoll), it rolls
// along the floor of its row; map units, (x, y) the bottom of the ball.
struct DungBall {
	float x, y;
	int dir; // -1 / +1
	RollRules rules; // its roller's
	int from; // the roller's MonsterTypeId: the journal's
	int startMs; // GameClock time of the kick: the seed of its flames and crumbs
	float rolled = 0.f; // tiles: the spin, the end of its roll
	float vy = 0.f; // tiles per second: falling into a pit (< 0)
	bool falling = false;
	int breakMs = -1; // GameClock time it broke; < 0: rolling
	bool sank = false; // broke in water: no crumbs
};

struct Projectiles {
	std::vector<Missile> missiles;
	std::vector<MonsterShot> shots;
	std::vector<DungBall> balls;
	void Clear(); // a level or save was loaded
	// Monster shots in flight or waiting in a volley (plan babi-boss); big: only the big coals.
	[[nodiscard]] int ShotCount(bool big = false) const;
	// The map x where the missiles stuck in a wall, the floor or a solid sub-cell are.
	[[nodiscard]] std::vector<float> StuckMissileXs() const;
};

#endif
