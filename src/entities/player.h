#ifndef PLAYER_H
#define PLAYER_H

#include "model_info.h"
#include "particles.h"
#include "player_stats.h"
#include "../core/timer.h"
#include "../core/gameplay_config.h"
#include "../world/world_events.h"
#include "../world/rng.h"

// Power hit (plan skill-power-hit): the bonus% of a melee hit charged with this many stamina points, on the weapon's
// damage (POWER_CURVE_SCALE, POWER_CURVE_POINTS); 0 for none.
[[nodiscard]] int powerHitBonusPercent(int points);

// A power hit's charge (Player::StartCharge): the melee attack key held down.
struct PowerCharge {
	int startMs = -1; // GameClock time the key went down; < 0: not charging
	int spent = 0; // stamina points drained so far
	bool dry = false; // the stamina ran out: the charge holds what it built
};

struct JumpState {
	bool jumping = false;
	bool falling = false;
	float dir_x = 0.f;
	float speed = 0.f;
	float velocity = 0.f;
	float fall_velocity = FALL_STEP;
	float start_y = 0.f;
	int counter = 0;
	Timer jump_timer{JUMP_TIMER_MS};
	Timer jump_up_timer{JUMP_UP_TIMER_MS};
	Timer jump_inc{JUMP_TICK_MS};
	Timer fall_inc{FALL_TICK_MS};
};

// The player: stats, pose (clip state, blood), jump and attack state. PlayerView draws them at the frame origin (the
// dungeon scrolls around it).
class Player {
  private:
	ModelInfo model; // of the player's model files (PlayerView::Load)
	ModelState state = ModelState::Idle;
	ClipPlayback playback{};
	ParticleSystem blood{0}; // stopped: no splash until the first hit
	int shownFrame = 0; // of the clip shown, as of the last Animate (it advances after the drawing showed it)

	void die(WorldEvents& events); // the death pose and sound; the poison and the curses end

  public:
	PlayerStats stats;
	float rotA = 0.f;
	float scale = 1.f;
	float depthOffset = 0.f; // moved towards the back wall (world units) while climbing
	Timer attackTimer{1000}; // the equipped weapon's WeaponMotion::AttackMs() between attacks
	JumpState jump;
	// The attack under way (the swing, or the bow draw): GameClock time it began, < 0: none. Landed: its hit time
	// (WeaponMotion::hitMs) has passed.
	int attackStartMs = -1;
	bool attackLanded = false;
	bool god = false; // the scenario god mode: no damage (TakeHit) and no poison damage
	Burning burning; // plan fire-damage; not saved, it lasts seconds
	// The stamina-skill toggle (plan skill-flaming-arrows): on, every learned stamina skill that fits the attack
	// applies and costs stamina. Not saved. The refusal (too little stamina for a skill) blinks the HUD sign.
	bool staminaSkills = false;
	std::optional<int> staminaSkillRefusedMs;
	int fireArrows = 0; // the next bow shots carry fire: a brazier lit them (Dungeon::lightArrows); not saved
	// Power hit (plan skill-power-hit), with the skill and the toggle on and a melee weapon in hand: the attack key
	// down starts the charge (false, nothing started: not even a tap's worth of stamina, the HUD sign blinks), it
	// drains once a tick (UpdateCharge, no regen meanwhile), the key up ends it (ReleaseCharge: the points spent, 0
	// for a tap) and the swing then started hits with swingPower's bonus (PowerDamage). Not saved.
	PowerCharge charge;
	int swingPower = 0; // stamina points the swing under way was charged with; 0: a plain one
	// The attack key charges: the skill, the toggle on, a melee weapon (ranged: false; a bow draw never charges).
	[[nodiscard]] bool CanCharge(bool ranged) const {
		return staminaSkills && stats.heka.Has(SkillId::PowerHit) && !ranged;
	}
	bool StartCharge();
	void UpdateCharge();
	int ReleaseCharge();
	[[nodiscard]] bool Charging() const { return charge.startMs >= 0; }
	// 0..1 of the cap (POWER_CAP_PERCENT of max stamina): the charge built so far, the one of the swing under way.
	[[nodiscard]] float ChargeShare() const;
	[[nodiscard]] float SwingShare() const;
	[[nodiscard]] int PowerDamage(int damage) const { return damage + damage * powerHitBonusPercent(swingPower) / 100; }

	void SetModel(const ModelInfo& info); // the clips and measures of its model; standing
	void Animate(); // once a tick: death / revival pose, the blood, the clip frame (the drawing only shows them)
	[[nodiscard]] bool Alive() const { return stats.Alive(); }
	// Hitbox round mapX, like Monster::HalfWidth: half width and height in map units (from the idle clip).
	[[nodiscard]] float HalfWidth() const;
	[[nodiscard]] float Height() const;
	// Armour absorbs some of dmg unless ignoreArmor (at least 1 HP is lost), but not its fire share (playerHitDamage),
	// which sets the player burning for burnMs, in god mode too. No damage in the scenario god mode.
	// Returns the HP lost (never more than it had): 0 in god mode or when already dead. The first hit writes the
	// health note.
	int TakeHit(int dmg, const DamageMix& mix, WorldEvents& events, bool ignoreArmor = false, int burnMs = BURN_MS);
	// A monster's attack finds the player: with the Dodge skill (plan skills) one in DODGE_PERCENT misses, rolled on
	// rng. True: dodged (the "Dodged" line and a whoosh); the caller deals no damage, no poison. Also in god mode
	// (scenarios).
	bool Dodges(WorldEvents& events, Rng& rng);
	// A poisoned bite or sting: that tier (re)starts (stats.poison). Applied in god mode too; only the damage is not.
	// Poisons unless the worn amulet and a resistance potion ward it off (PlayerStats::PoisonResistPercent, rolled on
	// rng).
	void Poison(PoisonTier tier, WorldEvents& events, Rng& rng);
	// A monster's hit landed: rolls its curse entries in order on rng, the first that takes is laid (one a hit).
	// Laid in god mode too. True when one was laid.
	bool Curse(const CurseEntries& entries, WorldEvents& events, Rng& rng);
	// Once a tick: the running tiers' damage, which can kill; the resistance potion's and the curses' time.
	void UpdatePoison(WorldEvents& events);
	// Once a tick: half water (inWater) puts the fire out, else it burns (Burning), armour or not, and can kill.
	// Also notes whether it stands in water: a hit's fire share is then WATER_FIRE_DAMAGE_PCT % (plan fire-in-water).
	void UpdateBurning(bool wading, WorldEvents& events);
	bool inWater = false; // as of the last UpdateBurning; not saved
	// Spit in the eyes (plan giant-cobra-blinding-spit): blinded for ms, at least what is left of an earlier one.
	void Blind(int ms, WorldEvents& events);
	// A screech (plan giant-bat-screech): dazed for ms, at least what is left of an earlier one.
	void Daze(int ms, WorldEvents& events);
	// Once a tick, after UpdateBurning: the blind's and the daze's time run, the blind's BLIND_WATER_FACTOR times as
	// fast in water.
	void UpdateSenses();
	// How blind the player is: 1 .. 0 over the last BLIND_CLEAR_MS, 0 when they see.
	[[nodiscard]] float Blindness() const;
	// Dazed: half the walk speed, no sprint, no jump, no grip on a ladder (Dungeon).
	[[nodiscard]] bool Dazed() const { return dazeMs > 0; }
	// How dazed, for the view's wobble: 1 .. 0 over the last DAZE_CLEAR_MS.
	[[nodiscard]] float Daziness() const;
	int blindMs = 0; // left of the blind; not saved, it lasts seconds
	int dazeMs = 0; // left of the daze; not saved
	void Reanimate(); // full HP, standing, no poison, no curse, no fire
	void setModelState(ModelState s) { model.Enter(state, s, playback); }
	// Climb clip at phase 0..1 of its cycle, set by the caller instead of the clock (no-op without the file).
	void showClimb(float phase);
	[[nodiscard]] bool climbing() const { return state == ModelState::Climb; }

	// What PlayerView shows.
	[[nodiscard]] const ModelInfo& Model() const { return model; }
	[[nodiscard]] ModelState State() const { return state; }
	[[nodiscard]] const ClipPlayback& Playback() const { return playback; }
	[[nodiscard]] int ShownFrame() const { return shownFrame; }
	[[nodiscard]] const ParticleSystem& Blood() const { return blood; }
};

#endif
