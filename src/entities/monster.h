#ifndef MONSTER_H
#define MONSTER_H

#include "character_model.h"
#include "../graphics/particles.h"
#include "../core/timer.h"
#include <memory>

// How a monster gets around. Only flyers cross pits and traps; walkers stop at their edge.
enum class Locomotion : unsigned char {
	Stationary, // rooted to its spawn tile (plant), attacks when the player is next to it
	Ambush,		// rooted like Stationary, idle and still (a treasure chest) until the player comes near, see Lurk;
				// killed, it leaves a real treasure chest on its tile
	Walk,		// follows the player along its row
	WalkJump,	// walks, leaps over pits and traps (giant rat, see Leap)
	Fly,		// see Flight
};

// Flying monsters (bats): hang on the ceiling until the player comes near, then swoop through them,
// biting on the way, fly on, turn and come back.
enum class FlightPhase : unsigned char { Roost, Swoop, Return };
struct Flight {
	FlightPhase phase = FlightPhase::Roost;
	int dir = 1;		 // +1 flying right, -1 left
	float lift = -1.f;	 // world units from the floor to the model origin; < 0: not placed yet (on the ceiling)
	float fall = 0.f;	 // falling speed after death, world units per second
	bool bitten = false; // this pass has bitten already
	int attackUntilMs = 0;
	int lastMs = -1; // GameClock time of the last update
};

// Walk-jumpers: a leap over a gap of pits and traps, x in tile-local map units like Monster::x.
struct Leap {
	float fromX = 0.f, toX = 0.f;
	int startMs = -1; // GameClock time of the take-off; < 0: on the ground
	int readyMs = 0;  // no new leap before this GameClock time
	float lift = 0.f; // world units from the floor to the model origin
};

// One kind of monster (level tile attribute, MonsterTypeId in level.h): loaded once, shared by its monsters.
struct MonsterType {
	CharacterModel model;
	int speed = 1;
	int maxHealth = 20;
	int damage = 1;
	int xp = 0;			// gained for the kill
	int attackMs = 800; // between bites
	float scale = 1.f;
	float rotA = 0.f; // model yaw facing the camera
	Locomotion locomotion = Locomotion::Walk;
	Rgb blood = {0.7f, 0.1f, 0.1f};
};

// A monster on the level. Map units are tiles; x is relative to the spawn tile's column.
// The player position (px, py) is in map units, like Dungeon's.
class Monster {
  private:
	const MonsterType* type = nullptr; // nullptr: an empty slot
	int col = -1, row = -1;			   // spawn tile
	float x = 0.f;
	int health = 0;
	ModelState state = ModelState::Move;
	int facing = 0; // -1 left, 0 the camera, +1 right
	ClipPlayback playback{};
	Flight flight;
	Leap leap;
	// Created on the slot's first spawn, kept over respawns in it.
	std::unique_ptr<ParticleSystem> blood;
	Timer stepTimer{70}, attackTimer{800};
	bool spawned = false; // the timers start on the first spawn
	// Has acted on the player (chased, bitten, left the roost) or been hit; the health bar shows from then on.
	bool alerted = false;

	void enter(ModelState s) { type->model.Enter(state, s, playback); }
	void drawHealthBar();
	[[nodiscard]] bool sameRow(float py) const;

  public:
	Monster() = default;
	Monster(const Monster&) = delete;
	Monster& operator=(const Monster&) = delete;
	void Spawn(const MonsterType& kind, int spawnCol, int spawnRow);
	void Clear(); // the slot is empty
	[[nodiscard]] bool Active() const { return type != nullptr; }
	[[nodiscard]] int Col() const { return col; }
	[[nodiscard]] int Row() const { return row; }
	[[nodiscard]] int Health() const { return health; }
	[[nodiscard]] float CentreX() const { return static_cast<float>(col) + x + 0.5f; } // map x
	[[nodiscard]] bool Alive() const { return health > 0; }
	[[nodiscard]] bool Alerted() const { return alerted; }
	[[nodiscard]] bool flies() const { return type->locomotion == Locomotion::Fly; }
	[[nodiscard]] bool rooted() const {
		return type->locomotion == Locomotion::Stationary || type->locomotion == Locomotion::Ambush;
	}
	[[nodiscard]] bool lurking() const { return type->locomotion == Locomotion::Ambush && !alerted; }
	// A dead ambusher whose die clip has played: its tile turns into a treasure chest.
	[[nodiscard]] bool LeavesChest() const;
	[[nodiscard]] bool jumping() const { return leap.startMs >= 0; }
	[[nodiscard]] bool canJump() const;
	[[nodiscard]] bool StepDue() { return stepTimer.TimePassed(); }
	[[nodiscard]] bool AttackDue() { return attackTimer.TimePassed(); }

	// -1 / +1: the player is to the left / right on this row, 0: in reach or not on this row.
	[[nodiscard]] int attackDirection(float px, float py) const;
	// Walkers: one step toward the player on its row; blocked: the cell in front of it blocks the walk.
	bool Seek(bool blocked, float px, float py);
	[[nodiscard]] float seekProbeX(int dir) const; // map x the walker checks for walls, dir from attackDirection
	void Attack(float py);
	// Ambushers: true while still disguised; wakes (and returns false) once the player is MIMIC_WAKE_RANGE close.
	bool Lurk(float px, float py);
	// Flyers: one step of the bat behaviour (see Flight); wallAhead: the cell in front of it blocks the flight.
	void Fly(bool wallAhead, float px, float py);
	[[nodiscard]] float flightProbeX() const; // map x the flyer checks for walls
	// Walk-jumpers: leap to tile-local x toX (the centre of the landing cell), then move along the arc until landed.
	void Jump(float toX);
	void UpdateJump();
	[[nodiscard]] bool Nearby(float px, float py, int range) const;
	bool takeHit(int dmg);
	// With the frame origin at the spawn tile.
	void Draw(float px, float py);
};

#endif
