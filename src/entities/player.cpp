#include "player.h"

namespace {
int clampToRange(int value, int minV, int maxV) {
	if (value < minV)
		return minV;
	if (value > maxV)
		return maxV;
	return value;
}
} // namespace

PlayerEntity::PlayerEntity() : monster(), stamina(100), max_stamina(100) {}

PlayerEntity::PlayerEntity(float dx, float dy) : monster(dx, dy), stamina(100), max_stamina(100) {}

PlayerEntity::PlayerEntity(float nX, float nY, int nSpeed, int nHP, int nDamage, int nXP)
	: monster(nX, nY, nSpeed, nHP, nDamage, nXP), stamina(100), max_stamina(100) {}

int PlayerEntity::Stamina() const { return stamina; }

int PlayerEntity::MaxStamina() const { return max_stamina; }

void PlayerEntity::SetStamina(int value) { stamina = clampToRange(value, 0, max_stamina); }

void PlayerEntity::SetMaxStamina(int value) {
	if (value < 0)
		value = 0;
	max_stamina = value;
	if (stamina > max_stamina)
		stamina = max_stamina;
}

bool PlayerEntity::ConsumeStamina(int value) {
	if (value <= 0)
		return true;
	if (stamina < value)
		return false;

	stamina -= value;
	return true;
}

void PlayerEntity::AddStamina(int value) {
	if (value <= 0)
		return;
	SetStamina(stamina + value);
}

float PlayerEntity::staminaRatio() const {
	if (max_stamina <= 0)
		return 0.0f;

	float ratio = (float)stamina / (float)max_stamina;
	if (ratio < 0.0f)
		return 0.0f;
	if (ratio > 1.0f)
		return 1.0f;

	return ratio;
}
