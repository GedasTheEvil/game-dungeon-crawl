#ifndef GAMEPLAY_MONSTERS_H
#define GAMEPLAY_MONSTERS_H

constexpr float STANDING_EPSILON = 0.05f; // above the floor by less than this still counts as standing on it
constexpr float MONSTER_SEEK_STEP = 0.0042f;
// A walker steps (Monster::StepDue) once this has passed, on the next tick: every MONSTER_STEP_PERIOD_MS (80).
// Monster and player hitboxes (Monster::HalfWidth, Player::HalfWidth) are measured from the models. Gaps are in tiles
// between the box edges.
constexpr float ROOTED_BITE_REACH = 0.25f; // a rooted monster (plant, mimic) bites the player this close
// The bite's reach (Monster::Reaches, MeleeReach; plan monster-attack-reach): a cone from its mouth along its facing,
// MELEE_REACH_SLACK tiles longer than the gap it bites from, MELEE_REACH_HALF_ANGLE degrees wide either way. A boss's
// is BOSS_MELEE_REACH_FACTOR longer, at least BOSS_MELEE_REACH_HALF_ANGLE wide. The bite lands MELEE_WINDUP_PCT of its
// attackMs after it starts: a jump or a step back in time makes it miss.
constexpr float BOSS_MELEE_REACH_FACTOR = 1.5f;
constexpr float BOSS_MELEE_REACH_HALF_ANGLE = 50.f;
constexpr int MELEE_WINDUP_PCT = 40;

// Walk-jumpers (Monster::Jump): the arc follows the _jump clip (10 frames at ~14 fps): crouch, air, landing crouch.
constexpr int MONSTER_JUMP_MS = 650;
constexpr float MONSTER_JUMP_TAKEOFF = 0.22f; // fraction of the leap when the feet leave the floor
constexpr float MONSTER_JUMP_TOUCHDOWN = 0.83f; // ... and touch it again
constexpr float MONSTER_JUMP_HEIGHT = 12.f; // world units at the top of the arc
constexpr int MONSTER_JUMP_COOLDOWN_MS = 2000; // from one take-off to the next
// Tiles a tick a monster's feet go up onto a step it jumped onto, or down off one (plan subcell-monsters): a quarter
// step in about 6 ticks, inside its leap.
constexpr float MONSTER_STEP_EASE = 0.04f;
// A coward that cannot reach the player runs this far along its row (between the boxes): past the composite bow (4.0).
constexpr float COWARD_SAFE_GAP = 4.5f;
// Kiters (KiteRules, plan monster-kiting): the player counts as coming at one for KITE_CLOSING_MS after a step
// towards it. Once it backs off, it goes on to its far while they keep coming, else to KITE_HYSTERESIS tiles past its
// near, so it does not jitter at the edge. The checker scores a kiter higher: it is hard to pin down.
constexpr int KITE_CLOSING_MS = 300;
constexpr float KITE_HYSTERESIS = 0.4f;
// A shooter under a step (plan anubis-stuck-under-step, Dungeon::lineUpAway): the shot's line is probed every
// LINE_UP_PROBE tiles, and so are the spots behind it; a boss steps back LINE_UP_BOSS_PACE times its walk step.
constexpr float LINE_UP_PROBE = 0.05f;
constexpr float LINE_UP_BOSS_PACE = 1.5f;
constexpr float KITER_THREAT_FACTOR = 1.15f;
// Light shunners (MonsterKind::shunsLight, the camel spider, plan camel-spider): a cell up to this many cells along the
// row from a flame (a wall torch, a brazier, an oil lamp) is lit. It does not step into one, and one in its way is a
// wall to it.
constexpr int LIGHT_SHUN_CELLS = 1;
// Climbers (plan monster-climbers): once they have seen the player they follow a path to them across floors, up to
// CLIMB_PATH_MAX steps; with no path for CLIMB_GIVE_UP_MS they stay where they are. They climb at a share of their walk
// speed, a boss at all of it. The checker scores a climber a little higher (the player cannot escape up a ladder).
constexpr int CLIMB_PATH_MAX = 20;
constexpr int CLIMB_GIVE_UP_MS = 5000;
constexpr float CLIMB_SPEED_FACTOR = 0.8f;
constexpr float BOSS_CLIMB_SPEED_FACTOR = 1.f;
constexpr float CLIMBER_THREAT_FACTOR = 1.05f;
constexpr float CLIMBER_FALL_STEP = 0.15f; // tiles a tick a climber killed on the rungs drops
// A leaping climber whose path climbs a ladder across a gap catches the rungs in the air (plan jump-onto-ladder): over
// the ladder's middle at this share of the leap's flight (0.5 the top of the arc). The boss's nearer the top, higher,
// and from one cell farther.
constexpr float LEAP_CATCH_AIR = 0.7f;
constexpr float BOSS_LEAP_CATCH_AIR = 0.55f;
constexpr int BOSS_LEAP_CATCH_EXTRA = 1;
// Terrain affinity (plan terrain-affinity, level/terrain.h): in a native terrain a monster walks faster, bites more
// often and harder; in a hostile one slower, less often and softer. One row per terrain (in Terrain's order), the same
// for every monster, bosses included. interval: times its attackMs (x0.8: 25% more bites).
// The checker scores a monster spawned in its native terrain higher (terrainThreatFactor): this share of the mean of
// its speed's and its damage per second's gain (water: about x1.09).
// The tell: its walk (and swim) clip plays this much faster in a native terrain, slower in a hostile one.
constexpr float NATIVE_CLIP_FACTOR = 1.1f;
constexpr float HOSTILE_CLIP_FACTOR = 0.9f;
// Fighting in a hostile cell, it steps out to a better one up to this many cells along its row from which it still
// bites the player; idle in one, it walks out to the nearest better cell up to TERRAIN_WALK_OUT_CELLS away.
constexpr int TERRAIN_STEP_OUT_CELLS = 2;
constexpr int TERRAIN_WALK_OUT_CELLS = 6;
constexpr int MINION_SUMMON_REACH = 3; // cells from its boss a summoned minion may appear
constexpr int MINION_EMERGE_MS = 700; // a summoned minion digs out or drops into place, then acts
// An Anubis guard steps out of the wall (Summon::Wall, plan mummy-boss): slower, so the player sees the stone move.
constexpr int WALL_EMERGE_MS = 1400;
constexpr int WALL_SUMMON_SIGHT = 3; // cells from the player: a wall this near is on screen (1280x720 shows about 3)

#endif
