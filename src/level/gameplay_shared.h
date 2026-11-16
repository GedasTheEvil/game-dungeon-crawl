#ifndef GAMEPLAY_SHARED_H
#define GAMEPLAY_SHARED_H

// Numbers (plan check-header-fanout) that inline code in widely included headers reads (player.h, monster.h, dungeon.h,
// monster_kinds.h, terrain.h, damage.h, bleeding.h, trap_hurt.h, ...): editing one recompiles most of the game. Every
// other number is in the topic's gameplay_*.h, which only the files that read it include. A new number goes in a topic
// header, here only when a widely included header needs it.

// From gameplay_player.h:
constexpr int UPDATE_TICK_MS = 16;
constexpr float WALK_SPEED = 1.f; // tiles per second, sprint x3
constexpr int JUMP_TIMER_MS = 5000;
constexpr int JUMP_UP_TIMER_MS = 4000;
constexpr int JUMP_TICK_MS = 40;
constexpr int FALL_TICK_MS = 40;
constexpr float FALL_STEP = 0.03f; // initial fall speed, grows by FALL_GRAVITY_STEP each tick

// From gameplay_bleeding.h:
constexpr int BLEED_BITE_PERCENT = 30;
constexpr int SPIKES_BLEED_PERCENT = 40;
constexpr int DEATH_TRAP_BLEED_PERCENT = 70;

// From gameplay_environment.h:
constexpr float SAND_SLOW_FACTOR = 0.4f;

// From gameplay_monster_kinds.h:
constexpr float WEB_SLOW_FACTOR = SAND_SLOW_FACTOR;
constexpr int BURROW_UNDER_MS = 900;
constexpr int BURROW_EVERY_MS = 6000;
constexpr float SAND_WORM_WAKE_RANGE = 2.5f;
constexpr float SAND_WORM_DIVE_MIN = 1.5f;
constexpr float SAND_WORM_DIVE_RANGE = 6.f;
constexpr float CHARGE_MIN = 2.5f, CHARGE_MAX = 6.f;
constexpr float CHARGE_SPEED = 4.f; // tiles per second

// From gameplay_monsters.h:
constexpr int MONSTER_STEP_MS = 70;
constexpr int MONSTER_STEP_PERIOD_MS = (MONSTER_STEP_MS + UPDATE_TICK_MS - 1) / UPDATE_TICK_MS * UPDATE_TICK_MS;
constexpr float MONSTER_BITE_REACH = 0.1f; // a walker stops and bites this far from the player
constexpr float MELEE_REACH_SLACK = 0.15f;
constexpr float MELEE_REACH_HALF_ANGLE = 35.f;
constexpr int MONSTER_JUMP_MAX_GAP = 2; // cells of pits and traps a leap clears
struct TerrainFactors {
	float speed = 1.f;
	float interval = 1.f;
	float damage = 1.f;
};
struct TerrainRow {
	TerrainFactors native;
	TerrainFactors hostile;
};
constexpr TerrainFactors HOSTILE_TERRAIN = {0.6f, 1.25f, 0.75f};
constexpr TerrainRow TERRAIN_ROWS[] = {
	{{2.f, 0.8f, 1.5f}, HOSTILE_TERRAIN}, // water
	{{2.f, 0.8f, 1.5f}, HOSTILE_TERRAIN}, // sand (quicksand too)
	{{1.5f, 0.8f, 1.25f}, HOSTILE_TERRAIN}, // darkness
	{{1.5f, 0.8f, 1.25f}, HOSTILE_TERRAIN}, // light
	{{2.f, 0.8f, 1.25f}, HOSTILE_TERRAIN}, // web
	{{1.5f, 0.8f, 1.5f}, HOSTILE_TERRAIN}, // fire
};
constexpr float TERRAIN_THREAT_WEIGHT = 0.1f;

// From gameplay_skills.h:
constexpr int WATER_FIRE_DAMAGE_PCT = 10;

// From gameplay_trap_damage.h:
// A body's share of a trap's damage in per-mille (plan same-hazards-for-all, Vitals::TrapLoss): the player's is cut by
// the trap ward, a monster's is its kind's trapPermille. Traps ignore armour.
constexpr int FULL_TRAP_SHARE = 1000;

#endif
