#include "quick_potion.h"
#include "../core/gameplay_config.h"
#include <algorithm>

std::optional<ItemKind> quickPotion(QuickKind kind, int current, int max, int smallCount, int largeCount) {
	bool health = kind == QuickKind::Health;
	ItemKind small = health ? ItemKind::SmallHealth : ItemKind::SmallStamina;
	ItemKind large = health ? ItemKind::LargeHealth : ItemKind::LargeStamina;
	if (current >= max)
		return std::nullopt;
	if (smallCount <= 0)
		return largeCount > 0 ? std::optional(large) : std::nullopt;
	if (largeCount <= 0)
		return small;

	int smallGain = max * (health ? PotionEffect::SMALL_HEAL_PERCENT : PotionEffect::SMALL_STAMINA_PERCENT) / 100;
	int afterSmall = std::min(current + smallGain, max);
	int danger = health ? max * QUICK_HEAL_DANGER_PERCENT / 100 : JUMP_STAMINA_COST;
	return afterSmall < danger ? large : small;
}
