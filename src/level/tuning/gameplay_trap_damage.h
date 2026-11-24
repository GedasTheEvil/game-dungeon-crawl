#ifndef GAMEPLAY_TRAP_DAMAGE_H
#define GAMEPLAY_TRAP_DAMAGE_H

constexpr int TRAP_HURT_INTERVAL_MS = 100;
constexpr int TRAP_DAMAGE_RAMP_HITS = 3; // damage grows by 1 every N consecutive hits
constexpr int TRAP_STREAK_RESET_MS = 2 * TRAP_HURT_INTERVAL_MS;
constexpr float TRAP_HITBOX_X_SCALE = 0.02f;
constexpr float TRAP_HITBOX_Y_SCALE = 0.006f;
// The traps' size: their drawn scale (assets.cpp) and, through the factors above, their hitboxes
// (Dungeon::updateTraps).
constexpr float SPIKES_SCALE = 16.f;
constexpr float DEATH_TRAP_SCALE = 40.f;
// Ceiling traps (plan ceiling-spikes, DungeonView::drawCeilingTrap): the spikes' model a quarter tile wide, this many
// along the cell's depth on each underside, its height stretched (the death trap's tips longer).
constexpr int CEILING_TRAP_ROWS = 4;
constexpr float CEILING_SPIKES_STRETCH = 1.5f;
constexpr float CEILING_DEATH_STRETCH = 2.5f;
// The player and the reckless walkers touch a spike or death trap with their box (plan trap-player-box), less this
// share of their half width on each side: grazing its edge doesn't count.
constexpr float TRAP_BOX_FORGIVE = 0.25f;
// Crossing a spike or death trap on foot (the player and a reckless monster alike, plan same-hazards-for-all;
// TrapBell): over the whole hitbox at walking speed it deals *_CROSS_HP, or *_CROSS_PERCENT of the max HP when more,
// spread over the hitbox as a bell curve (TRAP_BELL_SIGMA of its half width): the middle hurts most, a step into its
// edge only a little. Faster costs more, up to TRAP_SPRINT_FACTOR at a sprint (WALK_SPEED x 3). The TrapHurt ramp
// comes on top, so standing in it kills too. Then the body's trap share (FULL_TRAP_SHARE).
constexpr int SPIKES_CROSS_HP = 200;
constexpr int SPIKES_CROSS_PERCENT = 20;
constexpr int DEATH_TRAP_CROSS_HP = 400;
constexpr int DEATH_TRAP_CROSS_PERCENT = 40;
constexpr float TRAP_BELL_SIGMA = 0.35f;
constexpr float TRAP_SPRINT_FACTOR = 1.25f;

// Fire vents (plan fire-jet-trap): the jet burns whoever stands in it (the player and the monsters alike, plan
// same-hazards-for-all) for FIRE_JET_PERCENT_PER_SECOND of their max HP a second, at least FIRE_JET_HP_PER_SECOND,
// dealt each TRAP_HURT_INTERVAL_MS, cut by their trap share (a boss's is small: none melts in it): a whole jet costs
// the player 540 HP or more, the burning after a half more. It sets them burning as from a hit of a second's fire: a
// fifth of it a second for BURN_MS after.
constexpr int FIRE_JET_PERCENT_PER_SECOND = 50;
constexpr int FIRE_JET_HP_PER_SECOND = 450;

#endif
