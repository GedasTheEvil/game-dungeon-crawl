#ifndef MONSTER_H
#define MONSTER_H

#include "model_info.h"
#include "particles.h"
#include "../core/timer.h"
#include "trap_hurt.h"
#include "../world/damage.h"
#include "../world/items.h"
#include "../world/level.h"
#include "../world/monster_kinds.h"
#include "../world/poison.h"
#include "../world/rng.h"
#include "../world/world_events.h"
#include "monster_burrow.h"
#include "monster_charge.h"
#include "monster_climb.h"
#include "monster_flight.h"
#include "monster_grab.h"
#include "monster_leap.h"
#include "monster_roll.h"
#include <array>
#include <memory>
#include <optional>

// One kind of monster (level tile attribute, MonsterTypeId in level.h): its row (monster_kinds.h) and what the sim
// reads of its model, loaded once, shared by its monsters. The app draws it with its CharacterModel (Assets).
struct MonsterType : MonsterKind {
	ModelInfo model;
};
using MonsterTypes = std::array<MonsterType, MONSTER_TYPE_MAX + 1>; // by MonsterTypeId (level.h); index 0 is unused
// Every kind with the ModelInfo of its model files (LoadClips), for a sim without the app's Assets (the unit tests).
// False: a model has no reference clip.
bool LoadMonsterTypes(MonsterTypes& types);

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
	int col = -1, spawnRow = -1; // spawn tile
	int row = -1; // the row it is on: the spawn row, until it climbs
	float x = 0.f;
	int health = 0;
	ModelState state = ModelState::Move;
	int facing = 0; // -1 left, 0 the camera, +1 right
	void trapHit(int hundredths); // a trap's damage in hundredths of a HP, carried over (trapDamageCarry)
	int fleeDir = 0; // a fleeing coward runs this way (Flee), a kiter backs off (BackOff), -1 / +1; 0: neither
	float seenPlayerX = -1.f; // kiters: the player's map x on the last tick (WatchPlayer); < 0: not yet
	int closingUntilMs = -1; // kiters: GameClock time until which the player counts as coming at it
	ClipPlayback playback{};
	Flight flight;
	Leap leap;
	Burrow burrow;
	Charge charge;
	Grab grab;
	Climb climb;
	Roll roll;
	bool rungs = false; // on a ladder off the floor, or climbing (Dungeon::UpdateMonsters sets it every tick)
	int wayMs = -1; // GameClock time a climber last had a way to the player; < 0: never
	int wayCol = -1, wayRow = -1; // the player's cell then: it heads there until it gives up (CLIMB_GIVE_UP_MS)
	// Created on the slot's first spawn, kept over respawns in it.
	std::unique_ptr<ParticleSystem> blood;
	Timer stepTimer{70}, attackTimer{800};
	bool spawned = false; // the timers start on the first spawn
	// Has acted on the player (chased, bitten, left the roost) or been hit; the health bar shows from then on.
	bool alerted = false;
	bool minion = false; // summoned by a boss: its XP depends on the boss (Dungeon::MinionXP)
	int summonMs = -1; // GameClock time a boss summoned it; < 0: not summoned
	Summon summonedBy = Summon::DigOut;
	float wallX = 0.f; // Summon::Wall: tile-local x of the wall cell's centre it steps out of
	float tomb = 0.f; // entombed: world units its body is drawn back towards the wall, in its coffin
	bool inWater = false; // standing in half water (Dungeon::UpdateMonsters sets it every tick)
	bool headInWater = false; // its head is over half water: a swimmer floats only then (on the floor at the bank)
	float swim = 0.f; // a swimmer in the water: world units it floats up off the floor (swimLift), eased
	bool swimPlaced = false; // swim was set on the first Animate after the spawn
	float sink = 0.f; // world units it is drawn down into a water basin (Dungeon::WaterSink)
	TrapHurt trapHurt;
	int trapDamageCarry = 0; // hundredths of a HP of trap damage not dealt yet (trapDamagePct)
	int biteAtMs = -1; // GameClock time the bite under way lands or misses (UpdateBite); < 0: none
	int biteDir = 0; // the way the bite or grab under way goes, -1 / +1: it does not turn till then
	int rangedReadyMs = 0; // ranged attackers: no new shot before this GameClock time
	bool rangedReleased = true; // the ranged clip's shot has left (TakeShot)
	std::optional<ItemKind> drop; // the weapon chest it leaves once its die clip has played (RollKillDrop)
	Poison poison; // plan monster-poison
	bool poisonByPlayer = false; // a running tier is the player's doing: its kill is theirs
	Burning burning; // plan fire-damage
	bool burnByPlayer = false; // the fire is the player's doing (a weapon, a sunbeam): its kill is theirs, a trap's not

	void wake(); // a lurker stops lurking: the chest opens, the mummy starts to climb out

	void enter(ModelState s) { type->model.Enter(state, s, playback); }
	void sound(CharacterSound s) const { links.events->PlayCharacter(type->id, s); }
	void bite(); // the player takes its damage; a life-stealing boss heals by its share of the HP they lost
	void curse(const CurseEntries& entries); // its hit landed: it may lay a curse (Player::Curse), the journal sees it
	void sting(); // the scorpion's at the hold's end: a sure hit, its poison
	void letGo(); // the grab is over, missed or held to its end; the next bite after its attackMs
	[[nodiscard]] float roostLift() const; // flyers: world units from the floor to the origin, hanging from the ceiling
	[[nodiscard]] float emergeLift() const;
	[[nodiscard]] float burrowLift() const; // world units down in the floor while burrowing, <= 0 // world units off
											// its place while Emerging: < 0 in the floor, > 0 above
	// A swimmer in half water floats with its back (the clip's top) at the surface, a lurker with only its top
	// SUBMERGED_SHOW above it; 0 out of the water. Off the basin floor (sink).
	[[nodiscard]] float swimLift() const;
	// A floater in half water: world units from the basin floor up to the surface, where it floats; 0 elsewhere.
	[[nodiscard]] float floatLift() const;
	// World units off the row's floor: a flyer's height, a leap, a summon, a swim; < 0 down in a water basin.
	[[nodiscard]] float lift() const;
	[[nodiscard]] float climbLift() const; // world units up or down a ladder while climbing
	void showClimb(); // the climb clip's frame from its height (climbRise), as the player's

  public:
	Monster() = default;
	Monster(const Monster&) = delete;
	Monster& operator=(const Monster&) = delete;
	void Spawn(const MonsterType& kind, int spawnCol, int spawnRow, const MonsterLinks& world, Rng& effects);
	void Clear(); // the slot is empty
	// Summoned by a boss: comes out of the floor, the ceiling or a wall (Emerging) or climbs out of its coffin
	// (Rising), then chases the player at once. fromMapX: Summon::Wall, the map x of the wall cell's centre it steps
	// out of (onto its spawn cell).
	void MakeMinion(Summon how, float fromMapX = 0.f);
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
	[[nodiscard]] int SpawnRow() const { return spawnRow; }
	// The player at map y py is on its row (a climber halfway up a ladder: by its height).
	[[nodiscard]] bool sameRow(float py) const;
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
		return type->locomotion == Locomotion::Stationary || type->locomotion == Locomotion::Ambush || floats();
	}
	[[nodiscard]] bool floats() const { return type->locomotion == Locomotion::Floating; }
	[[nodiscard]] bool entombed() const { return type->locomotion == Locomotion::Entombed; }
	[[nodiscard]] bool submerged() const { return type->locomotion == Locomotion::Submerged; }
	[[nodiscard]] bool coiled() const { return type->locomotion == Locomotion::Coiled; }
	[[nodiscard]] bool figurine() const { return type->locomotion == Locomotion::Figurine; }
	// Woken, it plays its rise clip before it acts: the mummy climbs out of its coffin, the cobra and the lotus rear
	// up, the ushabti steps off its place.
	[[nodiscard]] bool rises() const { return entombed() || coiled() || floats() || figurine(); }
	[[nodiscard]] bool reckless() const { return type->courage == Courage::Reckless; }
	[[nodiscard]] bool lurking() const { return (ambushes(type->locomotion) || rises() || submerged()) && !alerted; }
	// The cell it stands in is half water (Dungeon::UpdateMonsters, every tick): it wades (Wading), an arrow hits it
	// for ARROW_WATER_DAMAGE_PCT. head: the cell under HeadX.
	// sink: world units it is drawn down into a water basin (Dungeon::WaterSink).
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
	[[nodiscard]] bool climbs() const { return type->climbs; }
	[[nodiscard]] bool Climbing() const { return climb.toRow >= 0; }
	[[nodiscard]] bool Falling() const { return climb.falls; }
	// On the rungs it does not attack, as the player cannot (plan no-attack-on-ladder).
	[[nodiscard]] bool OnRungs() const { return rungs; }
	void SetOnRungs(bool onRungs) { rungs = onRungs; }
	// Climbers: from the ladder in column ladderCol (its centre there) up or down to toRow, then UpdateClimb each
	// step until there.
	void StartClimb(int ladderCol, int toRow);
	void UpdateClimb();
	void DropTo(int floorRow); // killed on the rungs: falls down to the floor of floorRow
	// Climbers: one step along its path, dir -1 / +1 (as Seek, but the way is the path's); blocked: it stands.
	void WalkPath(int dir, bool blocked);
	// Climbers: a way to the player in cell (col, row) now. HeadingFor: the cell of the last way, while it has not
	// given up (CLIMB_GIVE_UP_MS); false: no way, or given up.
	void SawWay(int toCol, int toRow);
	[[nodiscard]] bool HeadingFor(int& outCol, int& outRow) const;
	[[nodiscard]] float WalkStep() const; // tiles a walk step goes, slower or faster in half water (Wading)
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
	// A walking coward, not a boss: it runs from a player it cannot reach (plan coward-flee-ranged).
	[[nodiscard]] bool flees() const;
	// Fleeing cowards: one step that way (dir -1 / +1), away from the player, as Seek; blocked: it stands.
	void Flee(bool blocked, int dir);
	void Stand(); // a fleeing coward out of the player's fire: stands, watching them
	// Kiters (KiteRules): once a tick, notes whether the player at px is coming at it (KITE_CLOSING_MS).
	void WatchPlayer(float px);
	[[nodiscard]] bool PlayerClosing() const;
	[[nodiscard]] int FleeDir() const { return fleeDir; } // a coward's flight or a kiter's back-off, 0: neither
	// Kiters: one step away from the player that way (dir -1 / +1), at its backOffSpeed.
	void BackOff(int dir);
	// Tiles between its box and the player's at px along the row; < 0: they overlap.
	[[nodiscard]] float PlayerGap(float px) const;
	// In reach: it starts a bite towards the player at px, or (HoldRules, rolled on the gameplay stream) opens its jaws
	// or claws for a grab (Grab). playerWading: the player stands in half water (a crocodile grabs only then).
	void Attack(float px, float py, bool playerWading);
	// A bite under way (plan monster-attack-reach): it winds up for MELEE_WINDUP_PCT of its attackMs, standing,
	// turned the way it bites, then UpdateBite.
	[[nodiscard]] bool Biting() const { return Alive() && biteAtMs >= 0; }
	// Once a tick while Biting: at its time the bite lands if it Reaches the player, else it snaps at the air.
	void UpdateBite(float px, float py);
	// Walkers, a step while Biting: on after the player who walks away the way it bites (a fast one catches them), not
	// back after one who jumped over it. blocked: the cell in front of it blocks the walk.
	void FollowBite(bool blocked, float px, float py);
	// The cone its bite reaches along (MeleeReach): tiles long from its mouth, degrees wide either way, the mouth
	// as a share of its height. A boss's is longer and wider (BOSS_MELEE_REACH_FACTOR).
	struct BiteCone {
		float length = 0.f;
		float halfAngleDeg = 0.f;
		float mouthY = 0.f;
	};
	[[nodiscard]] BiteCone Reach() const;
	// The player's box at (px, py) touches its body or its bite's cone, turned dir (-1 / +1; 0: the body only).
	[[nodiscard]] bool Reaches(float px, float py, int dir) const;
	// Ambushers, the entombed, the submerged, the coiled and the floating: true while still lurking; wakes (and returns
	// false) once the player is close (MUMMY_WAKE_RANGE, SUBMERGED_WAKE_RANGE, COILED_WAKE_RANGE,
	// FLOATING_WAKE_RANGE). An ambusher never wakes this way, only by Spring or a hit (plan interact-prompt).
	bool Lurk(float px, float py);
	// The interact key on an ambusher lying in wait (the mimic, opened like a chest): it wakes.
	void Spring();
	// Ranged attackers: the player is on its row, out of bite reach but within range, and the shot is ready.
	// The caller checks there is no wall between them.
	[[nodiscard]] bool canAttackRanged(float px, float py) const;
	void AttackRanged(); // stands still and plays the ranged clip; the shot leaves at its release
	[[nodiscard]] bool AttackingRanged() const; // the ranged clip is playing: it does not walk or bite
	[[nodiscard]] bool ShotReleased() const { return rangedReleased; } // this clip's shot has left (or none)
	// Once per shot, at the clip's release: true and the point (map units) the shot leaves from, never past the near
	// edge of the player at px.
	bool TakeShot(float px, float& outX, float& outY);
	// Burrowers: up, alerted and its dive is due (it waits for the player on its row).
	[[nodiscard]] bool canDive(float py) const;
	void Dive(float toX); // dives at once, comes up at tile-local x toX (see Burrow)
	// Sand burrowers (MonsterKind::sandBurrow) spawned in sand: under it (Hidden) until Surge.
	void BuryInSand();
	[[nodiscard]] bool UnderSand() const { return burrow.lurk; }
	// From under the sand since its spawn: it moves under it to tile-local x toX and comes up there, alerted.
	void Surge(float toX);
	// Under the sand or the floor: the map x of the ripple it makes (moving from where it went down to where it comes
	// up); nullopt when not Under.
	[[nodiscard]] std::optional<float> BurrowRipple() const;
	void DelayDive(); // no place to come up: try again later
	[[nodiscard]] bool Burrowing() const { return burrow.phase != BurrowPhase::Up; }
	// Diving or under the floor, or still in the wall it steps out of: no weapon hits it, the arrows fly over it.
	[[nodiscard]] bool Hidden() const {
		return burrow.phase == BurrowPhase::Dive || burrow.phase == BurrowPhase::Under ||
			   (summonedBy == Summon::Wall && Emerging());
	}
	// Once a tick while burrowing: the phases run on the clock; true on the tick it comes up (the hole opens).
	bool UpdateBurrow();
	// Chargers: ready, alerted, the player on its row ChargeRules::minRange..maxRange tiles away (the caller checks for
	// walls).
	[[nodiscard]] bool canCharge(float px, float py) const;
	void StartCharge(float px); // the windup, towards the player
	[[nodiscard]] bool Charging() const { return charge.phase != ChargePhase::Ready; }
	[[nodiscard]] bool Stunned() const { return charge.phase == ChargePhase::Stunned; }
	[[nodiscard]] int ChargeDir() const { return charge.dir; }
	// Once a tick while charging. wallAhead: the cell in front of its head blocks the rush (a wall, a pit). The rush
	// hits the player (double damage) once if they are in its way on the floor. True on the tick a stun ends with the
	// storm roar (MonsterKind::roar): the caller lets it loose (Dungeon::stormRoar).
	bool UpdateCharge(bool wallAhead, float px, float py);
	// Rollers (RollRules): ready, alerted, the player on its row RollRules::minRange..maxRange tiles away between the
	// boxes (the caller checks for walls).
	[[nodiscard]] bool canRoll(float px, float py) const;
	void StartRoll(float px); // turns its back on the player at px and shapes the ball (the ranged clip)
	// The windup, the kick, the stand up again: it does not walk or bite.
	[[nodiscard]] bool RollingBall() const { return roll.startMs >= 0; }
	// Once a tick while RollingBall: true on the tick of the kick (the caller sets the ball rolling from BallX).
	bool UpdateRoll();
	[[nodiscard]] int BallDir() const { return roll.dir; } // the way the ball goes, -1 / +1
	// Its RollRules; none (radius 0) for a kind that does not roll.
	[[nodiscard]] RollRules BallRules() const { return type->roll.value_or(RollRules{}); }
	// The ball in the windup, before the kick: 0..1 as it grows in shape; 0 with none.
	[[nodiscard]] float BallShaped() const;
	[[nodiscard]] float BallX() const; // map x of the ball's centre: the edge of its box towards the player
	// Grabbers (HoldRules): the tell, then the lunge, or the hold.
	// The tell: jaws or claws wide (monster_grab.cpp); Attack's roll, or the scenario `grab`. False: it does not grab
	// (HoldRules), or is grabbing already.
	bool StartGrab();
	// Its HoldRules; none (chancePercent 0) for a kind that does not grab.
	[[nodiscard]] HoldRules Hold() const { return type->hold.value_or(HoldRules{}); }
	[[nodiscard]] bool Grabbing() const { return Alive() && grab.phase != GrabPhase::Ready; }
	[[nodiscard]] bool Holding() const { return Alive() && grab.phase == GrabPhase::Hold; }
	// Once a tick while grabbing: the tell ends in the lunge (a miss out of reach or in a jump), the hold deals its
	// ticks and lets go at its end (a scorpion's sting). playerWading as for Attack.
	void UpdateGrab(float px, float py, bool playerWading);
	// The player hit it while it holds them: the hold ends HoldRules::breakMs sooner, at once if that is past (they
	// break free: no sting).
	void Struggle();
	// Holding: ms since the grab; 0 otherwise.
	[[nodiscard]] int HoldAgeMs() const;
	// Degrees it is drawn rolled over along its body: the death roll, a turn with each tick; 0 otherwise.
	[[nodiscard]] float HoldRoll() const;
	// Flyers: one step of the bat behaviour (see Flight); wallAhead: the cell in front of it blocks the flight.
	void Fly(bool wallAhead, float px, float py, const FlightGoal& goal);
	// A boss's minion, and a boss once it is roused, hunt the player along the whole row and farther up and down.
	[[nodiscard]] bool Hunts() const { return minion || (alerted && type->isBoss()); }
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
	// A hit with a weapon: its damage after resistances (resistedDamage); a fire share sets it burning. True: this
	// hit killed it.
	bool TakeWeaponHit(int dmg, const DamageMix& mix);
	// A poisoned hit (plan monster-poison): that tier (re)starts, as on the player, unless it shrugs it off
	// (poisonResistPercent, rolled on rng). byPlayer: the player's doing, its kill rewards them; else (a trap) it does
	// not, as a trap's kill. True: the poison took.
	bool TakePoison(PoisonTier tier, bool byPlayer, Rng& rng);
	// Once a tick: the running tiers' damage, which can kill. True: it killed and the player poisoned it
	// (Dungeon::rewardKill).
	bool UpdatePoison();
	[[nodiscard]] bool Poisoned() const { return poison.Any(); }
	// Once a tick: half water puts the fire out, else it burns (Burning), which can kill. True: it burnt to death
	// and the player set it alight (Dungeon::rewardKill).
	bool UpdateBurning();
	[[nodiscard]] bool Burns() const { return burning.Any(); }
	// Set on fire as by a hit with fire fire (a beam of sunlight on a mummy, plan sunlight-mirrors); not in water.
	void Kindle(double fire) {
		if (Alive() && !inWater) {
			burning.Ignite(fire);
			burnByPlayer = true;
		}
	}
	[[nodiscard]] int PoisonMask() const { return poison.Mask(); }
	// Each tick it stands in a spike or death trap (see TrapHurt).
	// venom: the death trap's poison tier, applied on a hit (no XP for the player's kill).
	void StandInTrap(std::optional<PoisonTier> venom, Rng& rng);
	// A trap's damage (TrapHurt, a falling rock), cut by trapDamagePct. A trap's kill gives no XP: the player must
	// not farm kills with traps.
	void TrapHit(int dmg);
	// Each tick it stands in a fire vent's jet (plan fire-jet-trap): FIRE_JET_MONSTER_HP_PER_SECOND, dealt at
	// TrapHurt's pace, all fire: its fire resistance counts on top of trapDamagePct, and it sets it burning (not in
	// water). No XP for the kill.
	void StandInFire();
	// Out of a trap that closes on it (wall spikes, plan rhythm-traps): its centre to map x on its row.
	void ShoveTo(float mapX) { x = mapX - static_cast<float>(col) - 0.5f; }
	// Once a tick: the pose for its state, the facing, the blood, the clip frame (DrawMonster only shows them).
	void Animate(float px, float py);

	// What DrawMonster (monster_draw.h) shows.
	// Map units from the spawn tile's column; on its way out of a wall, between the wall and its cell.
	[[nodiscard]] float LocalX() const;
	[[nodiscard]] ModelState State() const { return state; }
	[[nodiscard]] const ClipPlayback& Playback() const { return playback; }
	[[nodiscard]] int Facing() const { return facing; }
	// World units off the row's floor it is drawn at: lift(), but a flyer not placed yet is drawn below the floor.
	[[nodiscard]] float DrawnLift() const {
		return (flies() ? flight.lift : leap.lift + swim + floatLift() - sink + climbLift()) + emergeLift() +
			   burrowLift();
	}
	[[nodiscard]] float Tomb() const { return tomb; }
	// Climbing: world units it is drawn towards the wall (< 0), so its grip meets the ladder's rails; 0 otherwise.
	[[nodiscard]] float ClimbDepth() const;
	[[nodiscard]] bool ShowsClimb() const { return Alive() && state == ModelState::Climb; } // back to the camera
	[[nodiscard]] bool Roosting() const { return flies() && flight.phase == FlightPhase::Roost; }
	[[nodiscard]] const ParticleSystem& Blood() const { return *blood; }
};

#endif
