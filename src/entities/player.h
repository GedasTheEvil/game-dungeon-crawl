#ifndef PLAYER_H
#define PLAYER_H

#include "monster.h"

class PlayerEntity : public monster {
  private:
	int stamina;
	int max_stamina;

  public:
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
