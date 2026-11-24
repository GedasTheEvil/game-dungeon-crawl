#ifndef GAMEPLAY_ENVIRONMENT_H
#define GAMEPLAY_ENVIRONMENT_H

// Half water (crocodiles-and-flooded-cells): the player and the slowed walkers wade at this share of their speed (a
// monster's own: Wading, and its terrain affinity). No jump and no sprint while standing in it, except with the Favour
// of Hapi skill (plan skill-blessing-of-hapi). An arrow hits a monster standing in it for a share of its damage; melee
// is not affected.
constexpr float WADE_SPEED_FACTOR = 0.5f;
// Sand (plan desert-chapter, Dungeon::PlayerInSand): the walk at SAND_SPEED_FACTOR, no sprint, jumping is fine.
constexpr float SAND_SPEED_FACTOR = 0.75f;
constexpr int SAND_STEP_MS = 380; // between the sounds and dust puffs of the steps in sand
constexpr int SAND_PUFF_MS = 700;
constexpr int LANDING_PUFF_MS = 900; // a fall's puff of a landing heap's matter (plan soft-landing)
constexpr int SPRINT_PUFF_MS =
	220; // between the dust puffs of a sprint on a dry floor (plan sprint-toon-blur-and-extras)
// Quicksand (Dungeon::updateQuicksand): standing in it the player sinks. The walk starts at QUICKSAND_SPEED_FACTOR and
// slows to nothing at QUICKSAND_STUCK_MS; from QUICKSAND_HURT_MS on it takes QUICKSAND_HURT_PERCENT of the max HP
// every QUICKSAND_HURT_EVERY_MS (no armour). A jump frees the player for QUICKSAND_JUMP_STAMINA (the jump's twice).
// Drawn down to QUICKSAND_SINK_DEPTH tiles.
constexpr float QUICKSAND_SPEED_FACTOR = 0.5f;
constexpr int QUICKSAND_STUCK_MS = 3000;
constexpr int QUICKSAND_HURT_MS = 5000;
constexpr int QUICKSAND_HURT_EVERY_MS = 1000;
constexpr int QUICKSAND_HURT_PERCENT = 5;
constexpr int QUICKSAND_JUMP_STAMINA = 40;
constexpr float QUICKSAND_SINK_DEPTH = 0.2f;
constexpr int QUICKSAND_MAX_RUN = 2; // cells in a row (levelcheck): a jump out of the first lands past the second
// A sand fall (RockFall, SAND_FALL): SAND_FALL_DAMAGE (no armour) to the player under it and a SAND_SLOW_MS slow at
// SAND_SLOW_FACTOR; the cell turns to sand.
constexpr int SAND_FALL_DAMAGE = 40;
constexpr int SAND_SLOW_MS = 1500;

#endif
