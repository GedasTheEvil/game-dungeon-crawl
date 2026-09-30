#include "quick_potion.h"
#include "inventory.h"
#include "../core/gameplay_config.h"
#include <algorithm>

int quickPotion(QuickKind kind, int current, int max, int smallCount, int largeCount) {
	bool health = kind == QuickKind::Health;
	int small = health ? PotionId::SMALL_HEALTH : PotionId::SMALL_STAMINA;
	int large = health ? PotionId::LARGE_HEALTH : PotionId::LARGE_STAMINA;
	if (current >= max)
		return NO_POTION;
	if (smallCount <= 0)
		return largeCount > 0 ? large : NO_POTION;
	if (largeCount <= 0)
		return small;

	int smallGain = max * (health ? PotionEffect::SMALL_HEAL_PERCENT : PotionEffect::SMALL_STAMINA_PERCENT) / 100;
	int afterSmall = std::min(current + smallGain, max);
	int danger = health ? max * QUICK_HEAL_DANGER_PERCENT / 100 : JUMP_STAMINA_COST;
	return afterSmall < danger ? large : small;
}
