#ifndef GAMEPLAY_CONFIG_H
#define GAMEPLAY_CONFIG_H

constexpr float PLAYER_MOVE_STEP = 0.025f;
constexpr float PLAYER_FORWARD_MOVE_STEP = 0.0225f;

constexpr float JUMP_FORWARD_SPEED = 0.054f;
constexpr float JUMP_INITIAL_VELOCITY = 0.085f;
constexpr float JUMP_GRAVITY_STEP = 0.01f;
constexpr int JUMP_STAMINA_COST = 20;

constexpr int JUMP_TIMER_MS = 5000;
constexpr int JUMP_UP_TIMER_MS = 4000;
constexpr int JUMP_TICK_MS = 40;
constexpr int FALL_TICK_MS = 40;

constexpr float FALL_STEP = 0.03f; // initial fall speed, grows by FALL_GRAVITY_STEP each tick
constexpr float FALL_GRAVITY_STEP = JUMP_GRAVITY_STEP;
constexpr float FALL_MAX_STEP = 0.3f; // < 1 tile, so one tick never skips a floor
constexpr float FALL_START_THRESHOLD = 0.03f;

constexpr int TRAP_HURT_INTERVAL_MS = 100;
constexpr int TRAP_DAMAGE_RAMP_HITS = 3; // damage grows by 1 every N consecutive hits
constexpr int TRAP_STREAK_RESET_MS = 2 * TRAP_HURT_INTERVAL_MS;
constexpr float TRAP_HITBOX_X_SCALE = 0.02f;
constexpr float TRAP_HITBOX_Y_SCALE = 0.006f;

constexpr float MONSTER_SEEK_STEP = 0.0042f;

// Bats (monster::Fly). Distances in tiles along the row, heights in world units (a tile is 40) from the floor
// to the model origin (the lowest point of the flying pose).
constexpr float BAT_SIGHT = 1.75f;			 // a roosting bat wakes up when the player is this close
constexpr float BAT_OVERSHOOT = 1.5f;		 // flies on this far past the player before it turns
constexpr float BAT_BITE_REACH = 0.15f;		 // bites when this close in front of the player (it flies through him)
constexpr float BAT_TILES_PER_SPEED = 0.25f; // flight speed: tiles per second per speed point
constexpr float BAT_LOW_LIFT = 2.f;			 // height of the wing tips when passing the player (bites at head height)
constexpr float BAT_WING_DIP = 0.25f;		 // the wing tips reach this far below the origin (wingspans, bat.py)
constexpr float BAT_HIGH_LIFT = 19.f;		 // height at the turning point
constexpr float BAT_LIFT_RATE = 5.f;		 // how fast the height follows its target (1/s)
constexpr float BAT_CEILING = 40.f;			 // roosting bats hang from here
constexpr float BAT_WALL_MARGIN = 0.35f;	 // turns this far before a wall
constexpr int BAT_ATTACK_MS = 450;			 // attack clip after a bite
constexpr float BAT_FALL_GRAVITY = 600.f;	 // dead bats fall to the floor (world units / s^2)

// Keys, gates and levers (dungeon_mechanisms.cpp).
constexpr int GATE_OPEN_MS = 1200;			  // the gate slides up into the ceiling, passable once it is up
constexpr int LOCKED_HINT_INTERVAL_MS = 3000; // "needs the X key" at most this often
constexpr float KEY_SPIN_DEG_PER_MS = 0.12f;

// Rock fall: stepping into the cell starts the rumble, the rock drops after ROCK_WARN_MS and lands
// ROCK_FALL_MS later. Sprinting, jumping on or stepping back gets the player clear; walking on does not.
constexpr int ROCK_WARN_MS = 650;
constexpr int ROCK_FALL_MS = 300;
constexpr int ROCK_DAMAGE = 20;
constexpr float ROCK_HIT_HALF_WIDTH = 0.55f; // tiles from the cell centre
constexpr float ROCK_HIT_HEIGHT = 0.8f;		 // tiles above the floor the rock still hits (a jump does not dodge it)

#endif
