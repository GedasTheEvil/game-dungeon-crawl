#ifndef MONSTER_H
#define MONSTER_H

#include "model_info.h"
#include "particles.h"
#include "../core/timer.h"
#include "trap_hurt.h"
#include "../world/damage.h"
#include "../world/items.h"
#include "../world/monster_kinds.h"
#include "../world/poison.h"
#include "../world/rng.h"
#include "../world/world_events.h"
#include <memory>
#include <optional>

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

// Burrowers (Apep): dive into the floor, stay under it, come up elsewhere on the row (Dungeon::burrowTarget picks
// where). Hidden while diving and under: nothing hits it, it bites nothing.
enum class BurrowPhase : unsigned char { Up, Dive, Under, Surface };
struct Burrow {
	BurrowPhase phase = BurrowPhase::Up;
	int startMs = 0; // GameClock time the phase began
	int nextMs = 0;	 // no dive before this GameClock time
	float toX = 0.f; // tile-local x it comes up at
};

// Chargers (Sobek): from afar on its row it lowers its head (Windup), rushes along the row (Rush) through the player,
// and on into a wall that stuns it (Stunned: it takes double damage). The player jumps over it or leaves the row.
enum class ChargePhase : unsigned char { Ready, Windup, Rush, Stunned };
struct Charge {
	ChargePhase phase = ChargePhase::Ready;
	int startMs = 0;
	int nextMs = 0;	  // no charge before this GameClock time
	int dir = 1;	  // -1 / +1 along the row
	bool hit = false; // this rush has hit the player
};

// One kind of monster (level tile attribute, MonsterTypeId in level.h): its row (monster_kinds.h) and what the sim
// reads of its model, loaded once, shared by its monsters. The app draws it with its CharacterModel (Assets).
struct MonsterType : MonsterKind {
	ModelInfo model;
};

class Player;
class Journal;
class WorldEvents;

// What a monster touches of the world (Dungeon::SpawnMonster links it): the player it chases and bites, the journal
// that notes its moves, the event list for the player's hits, the level it lives on.
struct MonsterLinks {
	Player* player = nullptr;
	Journal* journal = nullptr;
	WorldEvents* events = nullptr;
	int level = 1;
	Rng* random = nullptr; // the gameplay stream: whether a poisoned bite poisons (the player's amulet)
};

// A monster on the level. Map units are tiles; x is relative to the spawn tile's column.
// The player position (px, py) is in map units, like Dungeon's.
class Monster {
  private:
	const MonsterType* type = nullptr; // nullptr: an empty slot
	MonsterLinks links;
	int col = -1, row = -1; // spawn tile
	float x = 0.f;
	int health = 0;
	ModelState state = ModelState::Move;
	int facing = 0; // -1 left, 0 the camera, +1 right
	ClipPlayback playback{};
	Flight flight;
	Leap leap;
	Burrow burrow;
	Charge charge;
	// Created on the slot's first spawn, kept over respawns in it.
	std::unique_ptr<ParticleSystem> blood;
	Timer stepTimer{70}, attackTimer{800};
	bool spawned = false; // the timers start on the first spawn
	// Has acted on the player (chased, bitten, left the roost) or been hit; the health bar shows from then on.
	bool alerted = false;
	bool minion = false; // summoned by a boss: its XP depends on the boss (Dungeon::MinionXP)
	int summonMs = -1;	 // GameClock time a boss summoned it; < 0: not summoned
	Summon summonedBy = Summon::DigOut;
	float tomb = 0.f;		  // entombed: world units its body is drawn back towards the wall, in its coffin
	bool inWater = false;	  // standing in half water (Dungeon::UpdateMonsters sets it every tick)
	bool headInWater = false; // its head is over half water: a swimmer floats only then (on the floor at the bank)
	float swim = 0.f;		  // a swimmer in the water: world units it floats up off the floor (swimLift), eased
	bool swimPlaced = false;  // swim was set on the first Animate after the spawn
	float sink = 0.f;		  // world units it is drawn down into a water basin (Dungeon::waterSink)
	TrapHurt trapHurt;
	int trapDamageCarry = 0;	  // hundredths of a HP of trap damage not dealt yet (trapDamagePct)
	int spitReadyMs = 0;		  // spitters: no new spit before this GameClock time
	bool spitReleased = true;	  // the spit clip's glob has left the mouth (TakeSpit)
	std::optional<ItemKind> drop; // the weapon chest it leaves once its die clip has played (RollKillDrop)
	Poison poison;				  // docs/plan/solved/monster-poison.md
	bool poisonByPlayer = false;  // a running tier is the player's doing: its kill is theirs

	void wake(); // a lurker stops lurking: the chest opens, the mummy starts to climb out

	void enter(ModelState s) { type->model.Enter(state, s, playback); }
	void sound(CharacterSound s) const { links.events->PlayCharacter(type->id, s); }
	void bite(); // the player takes its damage; a life-stealing boss heals by its share of the HP they lost
	[[nodiscard]] float roostLift() const; // flyers: world units from the floor to the origin, hanging from the ceiling
	[[nodiscard]] float emergeLift() const;
	[[nodiscard]] float burrowLift() const; // world units down in the floor while burrowing, <= 0 // world units off
											// its place while Emerging: < 0 in the floor, > 0 above
	// A swimmer in half water floats with its back (the clip's top) at the surface, a lurker with only its top
	// SUBMERGED_SHOW above it; 0 out of the water. Off the basin floor (sink).
	[[nodiscard]] float swimLift() const;
	// World units off the row's floor: a flyer's height, a leap, a summon, a swim; < 0 down in a water basin.
	[[nodiscard]] float lift() const;
	[[nodiscard]] bool sameRow(float py) const;

  public:
	Monster() = default;
	Monster(const Monster&) = delete;
	Monster& operator=(const Monster&) = delete;
	void Spawn(const MonsterType& kind, int spawnCol, int spawnRow, const MonsterLinks& world, Rng& effects);
	void Clear(); // the slot is empty
	// Summoned by a boss: comes out of the floor or the ceiling (Emerging) or climbs out of its coffin (Rising), then
	// chases the player at once.
	void MakeMinion(Summon how);
	[[nodiscard]] bool Minion() const { return minion; }
	// Still coming out after a summon: it does not act yet, it is drawn rising or dropping into place.
	[[nodiscard]] bool Emerging() const;
	[[nodiscard]] int SummonedMs() const { return summonMs; } // < 0: not summoned
	[[nodiscard]] Summon SummonedBy() const { return summonedBy; }
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
	[[nodiscard]] bool entombed() const { return type->locomotion == Locomotion::Entombed; }
	[[nodiscard]] bool submerged() const { return type->locomotion == Locomotion::Submerged; }
	[[nodiscard]] bool coiled() const { return type->locomotion == Locomotion::Coiled; }
	// Woken, it plays its rise clip before it acts: the mummy climbs out of its coffin, the cobra rears up.
	[[nodiscard]] bool rises() const { return entombed() || coiled(); }
	[[nodiscard]] bool reckless() const { return type->courage == Courage::Reckless; }
	[[nodiscard]] bool lurking() const {
		return (type->locomotion == Locomotion::Ambush || rises() || submerged()) && !alerted;
	}
	// The cell it stands in is half water (Dungeon::UpdateMonsters, every tick): it wades (Wading), an arrow hits it
	// for ARROW_WATER_DAMAGE_PCT. head: the cell under HeadX.
	// sink: world units it is drawn down into a water basin (Dungeon::waterSink).
	void SetInWater(bool water, bool head, float basin) {
		inWater = water;
		headInWater = head;
		sink = basin;
	}
	// Map x of the front of its box, the way it faces (its centre while it faces the camera).
	[[nodiscard]] float HeadX() const { return facing > 0 ? Right() : facing < 0 ? Left() : CentreX(); }
	[[nodiscard]] bool InWater() const { return inWater; }
	// Entombed or coiled: woken, still climbing out of its coffin or rearing up; it does not act yet.
	[[nodiscard]] bool Rising() const;
	// A dead ambusher whose die clip has played: its tile turns into a treasure chest.
	[[nodiscard]] bool LeavesChest() const;
	// A killed monster's weapon chest (RollKillDrop), once its die clip has played; nullopt before and after.
	[[nodiscard]] std::optional<ItemKind> TakeDrop();
	void SetDrop(std::optional<ItemKind> weapon) { drop = weapon; } // the player killed it (Dungeon::rewardKill)
	[[nodiscard]] bool jumping() const { return leap.startMs >= 0; }
	[[nodiscard]] bool canJump() const;
	[[nodiscard]] bool StepDue() { return stepTimer.TimePassed(); }
	[[nodiscard]] bool AttackDue() { return attackTimer.TimePassed(); }

	// -1 / +1: the player is to the left / right on this row, 0: in reach (MONSTER_BITE_REACH or ROOTED_BITE_REACH
	// between the boxes) or not on this row.
	[[nodiscard]] int attackDirection(float px, float py) const;
	// It has noticed the player, is awake and on their row, and can come at them (a rooted one: they are in its
	// reach). The caller checks for walls between (Dungeon::PlayerSafe).
	[[nodiscard]] bool Threatens(float px, float py) const;
	// Walkers: one step toward the player on its row, slower or faster in half water (Wading); blocked: the cell in
	// front of it blocks the walk.
	bool Seek(bool blocked, float px, float py);
	[[nodiscard]] float seekProbeX(int dir) const; // map x the walker checks for walls: its box edge on side dir
	void Attack(float py);
	// Ambushers, the entombed, the submerged and the coiled: true while still lurking; wakes (and returns false) once
	// the player is close (MIMIC_WAKE_RANGE, MUMMY_WAKE_RANGE, SUBMERGED_WAKE_RANGE, COILED_WAKE_RANGE).
	bool Lurk(float px, float py);
	// Spitters: the player is on its row, out of bite reach but within the spit's range, and the spit is ready.
	// The caller checks there is no wall between them.
	[[nodiscard]] bool canSpit(float px, float py) const;
	void Spit();						 // stands still and plays the spit clip; the glob leaves at its release
	[[nodiscard]] bool Spitting() const; // the spit clip is playing: it does not walk or bite
	// Once per spit, at the clip's release: true and the mouth (map units) the glob leaves from.
	bool TakeSpit(float& outX, float& outY);
	// Burrowers: up, alerted and its dive is due (it waits for the player on its row).
	[[nodiscard]] bool canDive(float py) const;
	void Dive(float toX); // dives at once, comes up at tile-local x toX (see Burrow)
	void DelayDive();	  // no place to come up: try again later
	[[nodiscard]] bool Burrowing() const { return burrow.phase != BurrowPhase::Up; }
	// Diving or under the floor: no weapon hits it, the arrows fly over it.
	[[nodiscard]] bool Hidden() const {
		return burrow.phase == BurrowPhase::Dive || burrow.phase == BurrowPhase::Under;
	}
	// Once a tick while burrowing: the phases run on the clock; true on the tick it comes up (the hole opens).
	bool UpdateBurrow();
	// Chargers: ready, alerted, the player on its row CHARGE_MIN..CHARGE_MAX tiles away (the caller checks for walls).
	[[nodiscard]] bool canCharge(float px, float py) const;
	void StartCharge(float px); // the windup, towards the player
	[[nodiscard]] bool Charging() const { return charge.phase != ChargePhase::Ready; }
	[[nodiscard]] bool Stunned() const { return charge.phase == ChargePhase::Stunned; }
	[[nodiscard]] int ChargeDir() const { return charge.dir; }
	// Once a tick while charging. wallAhead: the cell in front of its head blocks the rush (a wall, a pit). The rush
	// hits the player (double damage) once if they are in its way on the floor.
	void UpdateCharge(bool wallAhead, float px, float py);
	// Flyers: one step of the bat behaviour (see Flight); wallAhead: the cell in front of it blocks the flight.
	void Fly(bool wallAhead, float px, float py);
	[[nodiscard]] float flightProbeX() const; // map x the flyer checks for walls
	// Walk-jumpers: leap to tile-local x toX (see Dungeon::leapTarget), then move along the arc until landed.
	void Jump(float toX);
	void UpdateJump();
	// From the front edge of the player's box, facing dir (-1 / +1), to this box's near edge; < 0: they overlap.
	[[nodiscard]] float MeleeGap(float px, int dir) const;
	// In reach of a melee attack: MeleeGap at most reach tiles, not behind the player (MELEE_REACH_BEHIND), on
	// its row.
	[[nodiscard]] bool Nearby(float px, float py, float reach, int dir) const;
	// Body height in map y (row + height above its floor), for the arrows.
	[[nodiscard]] float BottomY() const;
	[[nodiscard]] float TopY() const;
	// From the player or a trap. True: this hit killed it (the player's kill: Dungeon::rewardKill).
	bool takeHit(int dmg);
	// A hit with a weapon: its damage after resistances (resistedDamage). True: this hit killed it.
	bool TakeWeaponHit(int dmg, const DamageMix& mix);
	// A poisoned hit (docs/plan/solved/monster-poison.md): that tier (re)starts, as on the player, unless it shrugs it
	// off (poisonResistPercent, rolled on rng). byPlayer: the player's doing, its kill rewards them; else (a trap) it
	// does not, as a trap's kill. True: the poison took.
	bool TakePoison(PoisonTier tier, bool byPlayer, Rng& rng);
	// Once a tick: the running tiers' damage, which can kill. True: it killed and the player poisoned it
	// (Dungeon::rewardKill).
	bool UpdatePoison();
	[[nodiscard]] bool Poisoned() const { return poison.Any(); }
	[[nodiscard]] int PoisonMask() const { return poison.Mask(); }
	// Each tick it stands in a spike or death trap (see TrapHurt).
	void StandInTrap();
	// A trap's damage (TrapHurt, a falling rock), cut by trapDamagePct. A trap's kill gives no XP: the player must
	// not farm kills with traps.
	void TrapHit(int dmg);
	// Once a tick: the pose for its state, the facing, the blood, the clip frame (DrawMonster only shows them).
	void Animate(float px, float py);

	// What DrawMonster (monster_draw.h) shows.
	[[nodiscard]] float LocalX() const { return x; } // map units from the spawn tile's column
	[[nodiscard]] ModelState State() const { return state; }
	[[nodiscard]] const ClipPlayback& Playback() const { return playback; }
	[[nodiscard]] int Facing() const { return facing; }
	// World units off the row's floor it is drawn at: lift(), but a flyer not placed yet is drawn below the floor.
	[[nodiscard]] float DrawnLift() const {
		return (flies() ? flight.lift : leap.lift + swim - sink) + emergeLift() + burrowLift();
	}
	[[nodiscard]] float Tomb() const { return tomb; }
	[[nodiscard]] bool Roosting() const { return flies() && flight.phase == FlightPhase::Roost; }
	[[nodiscard]] const ParticleSystem& Blood() const { return *blood; }
};

#endif
