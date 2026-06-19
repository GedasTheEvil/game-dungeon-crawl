#ifndef PLAYER_H
#define PLAYER_H

#include "monster.h"
#include "../core/timer.h"
#include <memory>

struct JumpState {
	bool jumping = false;
	bool falling = false;
	float dir_x = 0.f;
	float speed = 0.f;
	float velocity = 0.f;
	float start_y = 0.f;
	int counter = 0;
	std::unique_ptr<timer> jump_timer;
	std::unique_ptr<timer> jump_up_timer;
	std::unique_ptr<timer> jump_inc;
	std::unique_ptr<timer> fall_inc;
};

class PlayerEntity : public monster {
  private:
	int stamina;
	int max_stamina;

  public:
	JumpState jump;
	bool attacking = false;

	PlayerEntity();
	PlayerEntity(float dx, float dy);
	PlayerEntity(float nX, float nY, int nSpeed, int nHP, int nDamage, int nXP);

	int Stamina() const;
	int MaxStamina() const;
	void SetStamina(int value);
	void SetMaxStamina(int value);
	bool ConsumeStamina(int value);
	void AddStamina(int value);
	float staminaRatio() const;
};

#endif
