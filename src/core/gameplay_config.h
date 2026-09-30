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
constexpr float MONSTER_WALL_MARGIN = 0.5f; // a walker stops this far before a wall (its half width)

// Walk-jumpers (Monster::Jump): the arc follows the _jump clip (10 frames at ~14 fps): crouch, air, landing crouch.
constexpr int MONSTER_JUMP_MS = 650;
constexpr float MONSTER_JUMP_TAKEOFF = 0.22f;	// fraction of the leap when the feet leave the floor
constexpr float MONSTER_JUMP_TOUCHDOWN = 0.83f; // ... and touch it again
constexpr float MONSTER_JUMP_HEIGHT = 12.f;		// world units at the top of the arc
constexpr int MONSTER_JUMP_COOLDOWN_MS = 2000;	// from one take-off to the next
constexpr int MONSTER_JUMP_MAX_GAP = 2;			// cells of pits and traps a leap clears
constexpr int MINION_SUMMON_REACH = 3;			// cells from its boss a summoned minion may appear

constexpr float MIMIC_WAKE_RANGE = 1.5f; // tiles along the row: an idle mimic (Locomotion::Ambush) wakes this close

// Bats (Monster::Fly). Distances in tiles along the row, heights in world units (a tile is 40) from the floor
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

constexpr float MELEE_REACH_BEHIND = 0.1f; // tiles: a monster overlapping the player this far behind is still hit

// Bow (Dungeon::ShootArrow): the draw takes BOW_DRAW_MS, then the arrow flies on a parabola aimed at the centre of
// the nearest monster ahead within the bow's range, else at the floor ARROW_FREE_RANGE away. Map units (tiles).
constexpr int BOW_DRAW_MS = 450;
constexpr float ARROW_GRAVITY = 10.f;	 // tiles / s^2
constexpr float ARROW_FREE_RANGE = 2.5f; // with nothing to aim at
constexpr float ARROW_MAX_RISE = 0.6f;	 // a monster centre higher above the bow than this is out of reach
// The arc rises this far above the higher end, more for a longer shot: a close shot flies flat.
constexpr float ARROW_ARC_BASE = 0.05f;
constexpr float ARROW_ARC_PER_TILE = 0.1f;
constexpr float ARROW_HIT_HALF_WIDTH = 0.3f; // tiles either side of a monster's centre
constexpr int ARROW_STUCK_MS = 1500;		 // an arrow in a wall or the floor stays this long

// Keys, gates and levers (dungeon_mechanisms.cpp).
constexpr int GATE_OPEN_MS = 1200;			  // the gate slides up into the ceiling, passable once it is up
constexpr int LOCKED_HINT_INTERVAL_MS = 3000; // "needs the X key" at most this often
constexpr float KEY_SPIN_DEG_PER_MS = 0.12f;

// Rock fall: stepping into the cell starts the rumble, the rock drops after ROCK_WARN_MS and lands
// ROCK_FALL_MS later. Walking on without stopping, sprinting, jumping on or stepping back gets the player clear.
constexpr int ROCK_WARN_MS = 650;
constexpr int ROCK_FALL_MS = 300;
// Armor does not help against a boulder: a hit on the head crushes, the edge of it still breaks a leg.
constexpr int ROCK_CRUSH_DAMAGE = 1000;
constexpr int ROCK_GRAZE_DAMAGE = 50;
constexpr float ROCK_CRUSH_HALF_WIDTH = 0.3f; // tiles from the cell centre
constexpr float ROCK_GRAZE_HALF_WIDTH = 0.6f;
constexpr float ROCK_HIT_HEIGHT = 0.8f; // tiles above the floor the rock still hits (a jump does not dodge it)

#endif
