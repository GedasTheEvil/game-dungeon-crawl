#ifndef GAMEPLAY_COMBAT_H
#define GAMEPLAY_COMBAT_H

// Melee reach (a weapon's range / 10 tiles) is from the player's box edge to the monster's near edge. A monster
// whose far edge is this far behind the player's centre is still hit.
constexpr float MELEE_REACH_BEHIND = 0.1f;

// Bow (Dungeon::ShootArrow): the draw takes BOW_DRAW_MS, then the arrow flies on a parabola aimed at the near edge of
// the nearest monster ahead within the bow's range (half its height), else at the floor ARROW_FREE_RANGE away. Map
// units (tiles).
constexpr int BOW_DRAW_MS = 450;
constexpr float ARROW_GRAVITY = 10.f; // tiles / s^2
constexpr float ARROW_FREE_RANGE = 2.5f; // with nothing to aim at
constexpr float ARROW_MAX_RISE = 0.6f; // a monster's mid height higher above the bow than this is out of reach
// The arc rises this far above the higher end, more for a longer shot: a close shot flies flat.
constexpr float ARROW_ARC_BASE = 0.05f;
constexpr float ARROW_ARC_PER_TILE = 0.1f;
constexpr float ARROW_HIT_TOLERANCE = 0.05f; // tiles outside a monster's box an arrow still hits
constexpr int ARROW_STUCK_MS = 1500;
// An arrow at a monster down in a water basin is lobbed higher, ARROW_LOB_STEP tiles at a time, until it clears the
// basin's dry edge by this much.
constexpr float ARROW_EDGE_CLEARANCE = 0.02f;
constexpr float ARROW_LOB_STEP = 0.05f; // an arrow in a wall or the floor stays this long

#endif
