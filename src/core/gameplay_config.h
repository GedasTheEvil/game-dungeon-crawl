#ifndef GAMEPLAY_CONFIG_H
#define GAMEPLAY_CONFIG_H

constexpr float PLAYER_SCALE = 15.f; // the archeologist model's scale
// Walls and closed gates stop the player's centre this far off: wider than the hitbox the monsters and traps use
// (Player::HalfWidth, 0.06 tiles, from the model), so the drawn figure never sinks into a wall.
constexpr float PLAYER_BODY_HALF_WIDTH = PLAYER_SCALE / 60.f;
// Climbing up needs ladder this far above the player's feet (their height, roughly), so they stop with the head
// below the top rung.
constexpr float PLAYER_CLIMB_HEADROOM = PLAYER_SCALE / 40.f;
// Update() runs once a tick (game.cpp's timer, the scenario tick). A held walk key moves the player once a tick
// (stepHeldWalk), so the walk speed is WALK_SPEED on every machine, not the key repeat rate.
constexpr int UPDATE_TICK_MS = 16;
constexpr float WALK_SPEED = 1.f; // tiles per second, sprint x3
constexpr float PLAYER_MOVE_STEP = WALK_SPEED * static_cast<float>(UPDATE_TICK_MS) / 1000.f;
constexpr float PLAYER_FORWARD_MOVE_STEP = 0.9f * PLAYER_MOVE_STEP; // climbing up

// Map x within a cell at which the drawn player (always at the screen centre) is in front of the ladder:
// tile i is drawn from 40 * (i - mapX) - 2 and the ladder stands at its middle (Dungeon::Draw). Climbing pulls the
// player there.
constexpr float LADDER_GRIP_X = 0.45f;

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
// The traps' size: their drawn scale (assets.cpp) and, through the factors above, their hitboxes
// (Dungeon::updateTraps).
constexpr float SPIKES_SCALE = 16.f;
constexpr float DEATH_TRAP_SCALE = 40.f;

constexpr float STANDING_EPSILON = 0.05f; // above the floor by less than this still counts as standing on it
constexpr float MONSTER_SEEK_STEP = 0.0042f;
// Monster and player hitboxes (Monster::HalfWidth, Player::HalfWidth) are measured from the models. Gaps are in tiles
// between the box edges.
constexpr float MONSTER_BITE_REACH = 0.1f; // a walker stops and bites this far from the player
constexpr float ROOTED_BITE_REACH = 0.25f; // a rooted monster (plant, mimic) bites the player this close

// Walk-jumpers (Monster::Jump): the arc follows the _jump clip (10 frames at ~14 fps): crouch, air, landing crouch.
constexpr int MONSTER_JUMP_MS = 650;
constexpr float MONSTER_JUMP_TAKEOFF = 0.22f;	// fraction of the leap when the feet leave the floor
constexpr float MONSTER_JUMP_TOUCHDOWN = 0.83f; // ... and touch it again
constexpr float MONSTER_JUMP_HEIGHT = 12.f;		// world units at the top of the arc
constexpr int MONSTER_JUMP_COOLDOWN_MS = 2000;	// from one take-off to the next
constexpr int MONSTER_JUMP_MAX_GAP = 2;			// cells of pits and traps a leap clears
// A coward that cannot reach the player runs this far along its row (between the boxes): past the composite bow (4.0).
constexpr float COWARD_SAFE_GAP = 4.5f;
// Climbers (docs/plan/monster-climbers.md): once they have seen the player they follow a path to them across floors,
// up to CLIMB_PATH_MAX steps; with no path for CLIMB_GIVE_UP_MS they stay where they are. They climb at a share of
// their walk speed, a boss at all of it. The checker scores a climber a little higher (the player cannot escape up a
// ladder).
constexpr int CLIMB_PATH_MAX = 20;
constexpr int CLIMB_GIVE_UP_MS = 5000;
constexpr float CLIMB_SPEED_FACTOR = 0.8f;
constexpr float BOSS_CLIMB_SPEED_FACTOR = 1.f;
constexpr float CLIMBER_THREAT_FACTOR = 1.05f;
constexpr int MINION_SUMMON_REACH = 3; // cells from its boss a summoned minion may appear
constexpr int MINION_EMERGE_MS = 700;  // a summoned minion digs out or drops into place, then acts

// Half water (crocodiles-and-flooded-cells): the player and the slowed walkers wade at this share of their speed (a
// monster's own share: MonsterType::waterSpeed). No jump and no sprint while standing in it. An arrow hits a monster
// standing in it for a share of its damage; melee is not affected.
constexpr float WADE_SPEED_FACTOR = 0.5f;
constexpr int ARROW_WATER_DAMAGE_PCT = 50;
constexpr int WADE_SPLASH_MS = 450;		 // a splashing step while the player wades
constexpr int WATER_JUMP_HINT_MS = 3000; // "too deep to jump" shows at most this often

// The crocodile (Locomotion::Submerged): lies in the water, its top SUBMERGED_SHOW world units above the surface, until
// the player comes this close along its row. A swimmer's float height follows its target at SWIM_LIFT_RATE (1/s).
constexpr float SUBMERGED_WAKE_RANGE = 1.4f; // from its centre: its head is then ~0.65 tiles off, in view
constexpr float BOSS_WAKE_RANGE = 4.f;		 // a lurking boss (Sobek) wakes from farther, across its room
constexpr float SUBMERGED_SHOW = 0.5f;		 // the eyes, the nostrils and the back ridge (crocodile.py)
constexpr float SWIM_LIFT_RATE = 4.f;

// The cobra (Locomotion::Coiled) lies coiled until the player comes this close along its row, then rears up.
constexpr float COILED_WAKE_RANGE = 1.8f;
// Venom (Dungeon::Venom): flies straight at the player's chest as it was at the spit, on past it until it hits a
// wall or the floor, or has flown VENOM_MAX_FLIGHT tiles.
constexpr float VENOM_SPEED = 3.f;	// tiles per second
constexpr float VENOM_CHEST = 0.6f; // of the player's height
constexpr float VENOM_MAX_FLIGHT = 4.f;
constexpr int VENOM_SPLAT_MS = 400; // a splat on a wall or the floor stays this long

// Burrowers (Apep, Monster::Dive): sink into the floor, stay under, come up BURROW_BEHIND tiles past the player (the
// side away from where it dived), then walk and bite for BURROW_EVERY_MS before the next dive. A hole stays drawn
// where it went down and came up for BURROW_HOLE_MS.
constexpr int BURROW_DIVE_MS = 700;
constexpr int BURROW_UNDER_MS = 900;
constexpr int BURROW_SURFACE_MS = 700;
constexpr int BURROW_EVERY_MS = 6000;
constexpr int BURROW_RETRY_MS = 1000;
constexpr float BURROW_BEHIND = 1.6f;
constexpr int BURROW_HOLE_MS = 3000;

// Chargers (Sobek, Monster::StartCharge): from CHARGE_MIN..CHARGE_MAX tiles along its row it winds up, then rushes
// at CHARGE_SPEED. Its rush hits the player on the floor (not more than CHARGE_JUMP_CLEAR tiles up) for
// CHARGE_HIT_FACTOR x its damage; into a wall it is stunned and takes CHARGE_STUN_DAMAGE_FACTOR x the weapons' damage.
constexpr float CHARGE_MIN = 2.5f, CHARGE_MAX = 6.f;
constexpr int CHARGE_WINDUP_MS = 700;
constexpr float CHARGE_SPEED = 4.f; // tiles per second
constexpr float CHARGE_JUMP_CLEAR = 0.15f;
constexpr int CHARGE_HIT_FACTOR = 2;
constexpr int CHARGE_STUN_MS = 2500;
constexpr int CHARGE_STUN_DAMAGE_FACTOR = 2;
constexpr int CHARGE_COOLDOWN_MS = 4000;
constexpr float CHARGE_OVERRUN = 3.f; // tiles past the player a rush with no wall ahead stops

constexpr float MIMIC_WAKE_RANGE = 1.5f; // tiles along the row: an idle mimic (Locomotion::Ambush) wakes this close
// The mummy (Locomotion::Entombed) lies in its coffin (decor_coffin, drawn on its spawn tile), its body drawn
// MUMMY_COFFIN_DEPTH world units back from the walk line. Woken, its rise clip slides it out between the two fractions.
constexpr float MUMMY_WAKE_RANGE = 1.6f; // tiles along the row: in view (about 2 tiles each side)
constexpr float MUMMY_COFFIN_DEPTH = 14.4f;
constexpr float MUMMY_CLIMB_FROM = 0.3f, MUMMY_CLIMB_TO = 0.75f;

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

// Melee reach (a weapon's range / 10 tiles) is from the player's box edge to the monster's near edge. A monster
// whose far edge is this far behind the player's centre is still hit.
constexpr float MELEE_REACH_BEHIND = 0.1f;

// Bow (Dungeon::ShootArrow): the draw takes BOW_DRAW_MS, then the arrow flies on a parabola aimed at the near edge of
// the nearest monster ahead within the bow's range (half its height), else at the floor ARROW_FREE_RANGE away. Map
// units (tiles).
constexpr int BOW_DRAW_MS = 450;
constexpr float ARROW_GRAVITY = 10.f;	 // tiles / s^2
constexpr float ARROW_FREE_RANGE = 2.5f; // with nothing to aim at
constexpr float ARROW_MAX_RISE = 0.6f;	 // a monster's mid height higher above the bow than this is out of reach
// The arc rises this far above the higher end, more for a longer shot: a close shot flies flat.
constexpr float ARROW_ARC_BASE = 0.05f;
constexpr float ARROW_ARC_PER_TILE = 0.1f;
constexpr float ARROW_HIT_TOLERANCE = 0.05f; // tiles outside a monster's box an arrow still hits
constexpr int ARROW_STUCK_MS = 1500;
// An arrow at a monster down in a water basin is lobbed higher, ARROW_LOB_STEP tiles at a time, until it clears the
// basin's dry edge by this much.
constexpr float ARROW_EDGE_CLEARANCE = 0.02f;
constexpr float ARROW_LOB_STEP = 0.05f; // an arrow in a wall or the floor stays this long

// Keys, gates and levers (dungeon_mechanisms.cpp).
constexpr int GATE_OPEN_MS = 1200;			  // the gate slides up into the ceiling, passable once it is up
constexpr float GATE_APPROACH = 0.45f;		  // tiles from the gate at which a held key opens it
constexpr int LOCKED_HINT_INTERVAL_MS = 3000; // "needs the X key" at most this often
constexpr float KEY_SPIN_DEG_PER_MS = 0.12f;

// Rock fall: stepping into the cell starts the rumble, the rock drops after ROCK_WARN_MS and lands
// ROCK_FALL_MS later. Walking on without stopping, sprinting, jumping on or stepping back gets the player clear.
constexpr int ROCK_WARN_MS = 900;
constexpr int ROCK_FALL_MS = 300;
// Armor does not help against a boulder: a hit on the head crushes, the edge of it still breaks a leg.
constexpr int ROCK_CRUSH_DAMAGE = 1000;
constexpr int ROCK_GRAZE_DAMAGE = 50;
constexpr float ROCK_CRUSH_HALF_WIDTH = 0.3f; // tiles from the cell centre
constexpr float ROCK_GRAZE_HALF_WIDTH = 0.6f;
constexpr float ROCK_HIT_HEIGHT = 0.8f; // tiles above the floor the rock still hits (a jump does not dodge it)
static_assert(WALK_SPEED * static_cast<float>(ROCK_WARN_MS + ROCK_FALL_MS) / 1000.f >
				  1.f + ROCK_GRAZE_HALF_WIDTH - 0.5f,
			  "walking on from the cell's edge gets the player out of the graze before the rock lands");

// Dart trap (docs/plan/solved/poison-dart-trap.md): a pressure plate under the player or a heavy monster (weight
// DART_PLATE_WEIGHT and up, MonsterKind::weight) clicks; DART_DELAY_MS later the holes in the back wall above it shoot
// DART_COUNT darts out across the corridor, DART_GAP_MS apart, one per hole. At the walking line (DART_HIT_DEPTH)
// they take the body in front of their hole at DART_HEIGHT over the floor: a jump at its top lets them pass under,
// small monsters too. Each one that hits deals DART_DAMAGE and poisons (medium). The plate re-arms DART_REARM_MS after
// the click, once nothing heavy stands on it.
constexpr int PLAYER_WEIGHT = 2;
constexpr int DART_PLATE_WEIGHT = 2;
constexpr int DART_DELAY_MS = 200;
constexpr int DART_COUNT = 3; // one per hole
constexpr int DART_GAP_MS = 160;
constexpr float DART_SPEED = 6.f;	   // tiles a second
constexpr float DART_HIT_DEPTH = 0.5f; // tiles out of the back wall: where the player and the monsters walk
constexpr float DART_HOLE_X[3] = {-0.22f, 0.f, 0.22f}; // the holes, from the plate's centre, in firing order
constexpr float DART_HEIGHT = 0.3f;					   // tiles over the floor
constexpr int DART_DAMAGE = 2;
constexpr float DART_PLATE_HALF_WIDTH = 0.27f; // the slab (mechanism.py): a centre over it presses it
constexpr float DART_HALF_WIDTH = 0.1f;		   // a dart takes a body whose box comes this near its hole
constexpr int DART_REARM_MS = 3000;

#endif
