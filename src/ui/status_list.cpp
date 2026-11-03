#include "status_list.h"
#include "../entities/player_stats.h"

namespace {
// The drops, weak to strong: pale lime, venom green, deep malachite.
constexpr Rgb POISON_DROPS[POISON_TIER_COUNT] = {{0.72f, 0.9f, 0.3f}, {0.35f, 0.78f, 0.2f}, {0.08f, 0.5f, 0.18f}};
constexpr Rgb RESIST_DROPS[2] = {{0.35f, 0.62f, 0.92f}, {0.12f, 0.22f, 0.78f}}; // lesser, greater (ITEMS)
constexpr Rgb WATER_DROP = {0.08f, 0.52f, 0.68f}; // the Blessing of Hapi's, as its potion (ITEMS)
constexpr Rgb WHITE = {1, 1, 1};
} // namespace

StatusList statusList(const PlayerStats& stats, int burningLeftMs, int dazeMs) {
	using PlayerHud::Icon;
	StatusList list;
	if (stats.PotionResistLeftMs() > 0) {
		const bool greater = stats.PotionResistPercent() >= PotionEffect::GREATER_RESIST_PERCENT;
		list.Add(
			{StatusKind::Blessing,
			 Icon::Drop,
			 RESIST_DROPS[greater ? 1 : 0],
			 greater ? 2 : 1,
			 stats.PotionResistLeftMs()}
		);
	}
	if (stats.WaterBlessingLeftMs() > 0)
		list.Add({StatusKind::Blessing, Icon::Drop, WATER_DROP, 0, stats.WaterBlessingLeftMs()});
	if (stats.IsisWardLeftMs() > 0)
		list.Add({StatusKind::Blessing, Icon::IsisWard, WHITE, 0, stats.IsisWardLeftMs()});
	for (int t = 0; t < POISON_TIER_COUNT; t++)
		if (const int left = stats.poison.LeftMs(static_cast<PoisonTier>(t)); left > 0)
			list.Add({StatusKind::Condition, Icon::Drop, POISON_DROPS[t], t + 1, left});
	if (burningLeftMs > 0)
		list.Add({StatusKind::Condition, Icon::Burning, WHITE, 0, burningLeftMs});
	if (dazeMs > 0)
		list.Add({StatusKind::Condition, Icon::Daze, WHITE, 0, dazeMs});
	for (int c = 0; c < CURSE_COUNT; c++) {
		const auto curse = static_cast<Curse>(c);
		if (!stats.curses.Has(curse))
			continue;
		list.Add(
			{StatusKind::Curse,
			 PlayerHud::curseIcon(curse),
			 WHITE,
			 stats.curses.Greater(curse) ? 2 : 1,
			 stats.curses.LeftMs(curse)}
		);
	}
	return list;
}
