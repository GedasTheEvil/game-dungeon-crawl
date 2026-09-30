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

// A boss summons minions around itself while it lives (Dungeon::updateBoss).
struct BossRules {
	int minion = 0;		  // MonsterTypeId of its minions; 0: not a boss
	int minAlive = 0;	  // minions around it when it appears
	int maxAlive = 0;	  // no summon while this many are alive
	int summonMs = 0;	  // between summons
	int summonCap = 0;	  // summons per fight, after the first minAlive
	int lifeStealPct = 0; // heals this share of the damage it deals
};

// One kind of monster (level tile attribute, MonsterTypeId in level.h): loaded once, shared by its monsters.
struct MonsterType {
	const char* name = "";
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
	BossRules boss;
	[[nodiscard]] bool isBoss() const { return boss.minion != 0; }
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
	bool minion = false; // summoned by a boss: its XP depends on the boss (Dungeon::MinionXP)

	void enter(ModelState s) { type->model.Enter(state, s, playback); }
	void drawHealthBar();
	[[nodiscard]] bool sameRow(float py) const;

  public:
	Monster() = default;
	Monster(const Monster&) = delete;
	Monster& operator=(const Monster&) = delete;
	void Spawn(const MonsterType& kind, int spawnCol, int spawnRow);
	void Clear(); // the slot is empty
	// Summoned by a boss: chases the player at once.
	void MakeMinion() {
		minion = true;
		alerted = true;
	}
	[[nodiscard]] bool Minion() const { return minion; }
	[[nodiscard]] const MonsterType* Type() const { return type; }
	[[nodiscard]] int MaxHealth() const { return type->maxHealth; }
	[[nodiscard]] bool Active() const { return type != nullptr; }
	[[nodiscard]] int Col() const { return col; }
	[[nodiscard]] int Row() const { return row; }
	[[nodiscard]] int Health() const { return health; }
	[[nodiscard]] float CentreX() const { return static_cast<float>(col) + x + 0.5f; } // map x
	// Hitbox: the model's half width (CharacterModel::HalfWidth) at its drawn scale, map units. Height: BottomY, TopY.
	[[nodiscard]] float HalfWidth() const;
	[[nodiscard]] float Left() const { return CentreX() - HalfWidth(); }
	[[nodiscard]] float Right() const { return CentreX() + HalfWidth(); }
	// The box edge facing the player looking dir (-1 / +1), and the one away from them.
	[[nodiscard]] float NearEdge(int dir) const { return dir > 0 ? Left() : Right(); }
	[[nodiscard]] float FarEdge(int dir) const { return dir > 0 ? Right() : Left(); }
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

	// -1 / +1: the player is to the left / right on this row, 0: in reach (MONSTER_BITE_REACH or ROOTED_BITE_REACH
	// between the boxes) or not on this row.
	[[nodiscard]] int attackDirection(float px, float py) const;
	// Walkers: one step toward the player on its row; blocked: the cell in front of it blocks the walk.
	bool Seek(bool blocked, float px, float py);
	[[nodiscard]] float seekProbeX(int dir) const; // map x the walker checks for walls: its box edge on side dir
	void Attack(float py);
	// Ambushers: true while still disguised; wakes (and returns false) once the player is MIMIC_WAKE_RANGE close.
	bool Lurk(float px, float py);
	// Flyers: one step of the bat behaviour (see Flight); wallAhead: the cell in front of it blocks the flight.
	void Fly(bool wallAhead, float px, float py);
	[[nodiscard]] float flightProbeX() const; // map x the flyer checks for walls
	// Walk-jumpers: leap to tile-local x toX (see Dungeon::leapTarget), then move along the arc until landed.
	void Jump(float toX);
	void UpdateJump();
	// From the front edge of the player's box, facing dir (-1 / +1), to this box's near edge; < 0: they overlap.
	[[nodiscard]] float MeleeGap(float px, int dir) const;
	// In reach of a melee attack: MeleeGap at most range / 10 tiles, not behind the player (MELEE_REACH_BEHIND), on
	// its row.
	[[nodiscard]] bool Nearby(float px, float py, int range, int dir) const;
	// Body height in map y (row + height above its floor), for the arrows.
	[[nodiscard]] float BottomY() const;
	[[nodiscard]] float TopY() const;
	bool takeHit(int dmg);
	// With the frame origin at the spawn tile.
	// Once a tick while in view: the pose for its state, the facing, the blood, the clip frame (Draw only shows them).
	void Animate(float px, float py);
	void Draw();
};

#endif
