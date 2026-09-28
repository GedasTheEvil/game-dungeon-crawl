#ifndef PLAYER_H
#define PLAYER_H

#include "character_model.h"
#include "../graphics/particles.h"
#include "../core/timer.h"
#include <memory>

struct JumpState {
	bool jumping = false;
	bool falling = false;
	float dir_x = 0.f;
	float speed = 0.f;
	float velocity = 0.f;
	float fall_velocity = 0.f;
	float start_y = 0.f;
	int counter = 0;
	std::unique_ptr<Timer> jump_timer;
	std::unique_ptr<Timer> jump_up_timer;
	std::unique_ptr<Timer> jump_inc;
	std::unique_ptr<Timer> fall_inc;
};

// The player's figure: model, animation, blood, stamina. Drawn at the frame origin (the dungeon scrolls around it).
// HP, XP and the other stats are in PlayerStats, which keeps health / maxHealth in step.
class Player {
  private:
	CharacterModel model;
	ModelState state = ModelState::Idle;
	ClipPlayback playback{};
	ParticleSystem blood{100};
	int stamina = 100;
	int max_stamina = 100;

  public:
	int health = 1;
	int maxHealth = 1;
	float rotA = 0.f;
	float scale = 1.f;
	float depthOffset = 0.f; // moved towards the back wall (world units) while climbing
	Timer attackTimer{1000};
	JumpState jump;
	bool attacking = false;

	bool Load(const char* name, const Texture& texture);
	void Draw();
	[[nodiscard]] bool Alive() const { return health > 0; }
	bool takeHit(int dmg);
	void Reanimate();
	void setModelState(ModelState s) { model.Enter(state, s, playback); }
	// Climb clip at phase 0..1 of its cycle, set by the caller instead of the clock (no-op without the file).
	void showClimb(float phase);
	[[nodiscard]] bool climbing() const { return state == ModelState::Climb; }
	void PlayAttackSound() const { model.attackSound.Play(); }
	void PlayJumpSound() const { model.jumpSound.Play(); }
	[[nodiscard]] float healthRatio() const;

	[[nodiscard]] int Stamina() const { return stamina; }
	[[nodiscard]] int MaxStamina() const { return max_stamina; }
	void SetStamina(int value);
	void SetMaxStamina(int value);
	bool ConsumeStamina(int value);
	void AddStamina(int value);
	[[nodiscard]] float staminaRatio() const;
};

#endif
