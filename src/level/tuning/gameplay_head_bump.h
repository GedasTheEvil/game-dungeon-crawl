#ifndef GAMEPLAY_HEAD_BUMP_H
#define GAMEPLAY_HEAD_BUMP_H

#include "gameplay_player.h"

// A head bump (plan head-bump, Player::HeadBump): a jump into a low ceiling, or any knock on the head, costs the larger
// of HEAD_BUMP_MIN_HP and HEAD_BUMP_MAX_HP_PCT % of the max HP (no armour, no resistance) and daze for
// HEAD_BUMP_DAZE_MIN_MS .. HEAD_BUMP_DAZE_MAX_MS, HEAD_BUMP_DAZE_PCT % of the time. A jump only hurts when it rises
// faster than HEAD_BUMP_MIN_RISE at the ceiling; a graze only knocks.
constexpr int HEAD_BUMP_MIN_HP = 5;
constexpr int HEAD_BUMP_MAX_HP_PCT = 5;
constexpr int HEAD_BUMP_DAZE_PCT = 50;
constexpr int HEAD_BUMP_DAZE_MIN_MS = 1000;
constexpr int HEAD_BUMP_DAZE_MAX_MS = 2000;
constexpr float HEAD_BUMP_MIN_RISE_FRACTION = 1.f / 3.f; // of the take-off velocity
constexpr float HEAD_BUMP_MIN_RISE = JUMP_INITIAL_VELOCITY * HEAD_BUMP_MIN_RISE_FRACTION;
// A monster's leap (plan monster-head-bump) meets a low ceiling by the same rule; after a bump it does not pounce
// under that ceiling again for this long, so a fight is not a free stun loop.
constexpr int MONSTER_HEAD_BUMP_MEMORY_MS = 6000;

#endif
