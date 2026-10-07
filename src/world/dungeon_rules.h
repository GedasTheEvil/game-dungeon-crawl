#ifndef DUNGEON_RULES_H
#define DUNGEON_RULES_H

// Rules the dungeon's sim files share with its drawing (dungeon_render*.cpp): no GL here.

#include "items.h"
#include "world_events.h"
#include "../core/timer.h"
#include <algorithm>
#include <cstddef>

constexpr int STICK_RETURN_MS = 400; // the throwing stick flies back to the hand, a look only

// How each missile flies (MissileKind order). Arc: a share of the arrow's (a flat sling shot, a lobbed stick). Spin
// in degrees a second, drawn round its middle; a missile that does not spin is drawn tip first along its flight.
struct MissileRules {
	float arc;
	float spinDegPerS;
	bool sticks;  // stays in a wall or the floor for a while (ARROW_STUCK_MS); else gone at once
	bool returns; // flies back to the player after a hit or a wall
	float scale;  // drawn this many times its size in metres (the stone would be a speck)
	WorldSound hit, wall;
};
constexpr MissileRules MISSILE_RULES[MISSILE_KIND_COUNT] = {
	{1.f, 0.f, true, false, 1.f, WorldSound::ArrowHit, WorldSound::ArrowWall},	   // arrow
	{0.35f, 0.f, false, false, 2.5f, WorldSound::StoneHit, WorldSound::StoneWall}, // sling stone
	{0.6f, 900.f, false, true, 1.2f, WorldSound::StoneHit, WorldSound::StoneWall}, // throwing stick
	{1.f, 0.f, true, false, 1.f, WorldSound::ArrowHit, WorldSound::ArrowWall},	   // javelin
};
inline const MissileRules& rulesOf(MissileKind kind) { return MISSILE_RULES[static_cast<size_t>(kind)]; }

// 0..1 of the way through a motion that started at startMs and lasts durationMs (game clock).
inline float motionProgress(int startMs, int durationMs) {
	return std::clamp(static_cast<float>(GameClock::now() - startMs) / static_cast<float>(durationMs), 0.f, 1.f);
}

#endif
