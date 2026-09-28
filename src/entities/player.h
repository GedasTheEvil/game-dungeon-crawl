#ifndef PLAYER_H
#define PLAYER_H

#include "character_model.h"
#include "player_stats.h"
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

// The player: stats, figure (model, animation, blood), jump and attack state. Drawn at the frame origin (the dungeon
// scrolls around it).
class Player {
  private:
	CharacterModel model;
	ModelState state = ModelState::Idle;
	ClipPlayback playback{};
	ParticleSystem blood{100};

  public:
	PlayerStats stats;
	float rotA = 0.f;
	float scale = 1.f;
	float depthOffset = 0.f; // moved towards the back wall (world units) while climbing
	Timer attackTimer{1000};
	JumpState jump;
	bool attacking = false;

	bool Load(const char* name, const Texture& texture);
	void Draw();
	[[nodiscard]] bool Alive() const { return stats.Alive(); }
	// Armour absorbs some of dmg unless ignoreArmor (at least 1 HP is lost). No damage in the scenario god mode.
	void TakeHit(int dmg, bool ignoreArmor = false);
	void Reanimate(); // full HP, standing
	void setModelState(ModelState s) { model.Enter(state, s, playback); }
	// Climb clip at phase 0..1 of its cycle, set by the caller instead of the clock (no-op without the file).
	void showClimb(float phase);
	[[nodiscard]] bool climbing() const { return state == ModelState::Climb; }
	void PlayAttackSound() const { model.attackSound.Play(); }
	void PlayJumpSound() const { model.jumpSound.Play(); }
};

#endif
