#include "quick_potion.h"

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

	int percent = health ? QUICK_LARGE_HEALTH_PERCENT : QUICK_LARGE_STAMINA_PERCENT;
	return current * 100 <= max * percent ? large : small;
}
