#ifndef PLAYER_H
#define PLAYER_H

#include "character_model.h"
#include "player_stats.h"
#include "../graphics/particles.h"
#include "../core/timer.h"
#include "../core/gameplay_config.h"
#include "../graphics/texture_registry.h"
#include "../world/world_events.h"
#include "../world/rng.h"
#include <memory>

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

// The player: stats, figure (model, animation, blood), jump and attack state. Drawn at the frame origin (the dungeon
// scrolls around it).
class Player {
  private:
	CharacterModel model;
	ModelState state = ModelState::Idle;
	ClipPlayback playback{};
	ParticleSystem blood{0}; // stopped: no splash until the first hit
	// Corner indices of the two fists (model -x, +x), the same in every clip (one mesh); -1: not found.
	std::array<int, 2> fists{-1, -1};
	int shownFrame = 0; // of the clip Draw() showed last (it advances after showing)

	void findFists();
	void die(); // the death pose and sound; the poison ends

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

	bool Load(const char* name, Texture&& texture);
	void Animate(); // once a tick: death / revival pose, the blood, the clip frame (Draw only shows them)
	void Draw(const TextureRegistry& textures);
	[[nodiscard]] bool Alive() const { return stats.Alive(); }
	// Hitbox round mapX, like Monster::HalfWidth: half width and height in map units (from the idle clip).
	[[nodiscard]] float HalfWidth() const;
	[[nodiscard]] float Height() const;
	// Armour absorbs some of dmg unless ignoreArmor (at least 1 HP is lost). No damage in the scenario god mode.
	// Returns the HP lost (never more than it had): 0 in god mode or when already dead. The first hit writes the
	// health note.
	int TakeHit(int dmg, const DamageMix& mix, WorldEvents& events, bool ignoreArmor = false);
	// A poisoned bite or sting: that tier (re)starts (stats.poison). Applied in god mode too; only the damage is not.
	// Poisons unless the worn amulet and a resistance potion ward it off (PlayerStats::PoisonResistPercent, rolled on
	// rng).
	void Poison(PoisonTier tier, WorldEvents& events, Rng& rng);
	void UpdatePoison(); // once a tick: the running tiers' damage, which can kill; the resistance potion's time
	void Reanimate();	 // full HP, standing, no poison
	void setModelState(ModelState s) { model.Enter(state, s, playback); }
	// Climb clip at phase 0..1 of its cycle, set by the caller instead of the clock (no-op without the file).
	void showClimb(float phase);
	[[nodiscard]] bool climbing() const { return state == ModelState::Climb; }
	// The fist nearer the camera facing dir (+1 right, -1 left), in the shown clip frame: where the weapon is held.
	// In the frame Draw() is called in, world units.
	[[nodiscard]] std::array<float, 3> Fist(int dir) const;
	void PlayJumpSound() const { model.jumpSound.Play(); }
};

#endif
