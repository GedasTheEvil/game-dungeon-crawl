#ifndef OFFERING_GIFT_H
#define OFFERING_GIFT_H

// Offering gifts (plan offering-gift): now and then a shrine's god answers an offering with a gift on top of the XP,
// a permanent stat (an inscription's boon, at half its size) or an item. A lottery: nothing hints at it beforehand.
// Merges are no offerings and give none.

#include "inscriptions.h"
#include "items.h"
#include "shrines.h"
#include <array>
#include <cstdint>
#include <optional>
#include <utility>

// Percent of the offerings that bring a gift, whatever is offered.
constexpr int OFFERING_GIFT_PERCENT = 10;
// The stat gifts: max health and max stamina in percent of the max (half an inscription's boon), Might and Armor.
constexpr int GIFT_LIFE_PERCENT = 5;
constexpr int GIFT_STAMINA_PERCENT = 5;
constexpr int GIFT_MIGHT = 1;
constexpr int GIFT_ARMOR = 1;

enum class GiftKind : std::uint8_t { Might, Armor, Life, Stamina, Item };
constexpr int GIFT_KIND_COUNT = 5;

// Each god's chances of each gift, in percent (each row sums to 100), in God order: the god leans to its own field.
constexpr std::array<std::array<int, GIFT_KIND_COUNT>, GOD_COUNT> GIFT_WEIGHTS = {{
	// Might, Armor, Life, Stamina, Item
	{5, 15, 50, 15, 15}, // Isis
	{50, 10, 15, 15, 10}, // Montu
	{10, 50, 15, 10, 15}, // Ptah
	{10, 10, 10, 10, 60}, // Amun
	{15, 10, 15, 50, 10}, // Serket
}};
[[nodiscard]] constexpr int giftWeight(God god, GiftKind kind) {
	return GIFT_WEIGHTS[std::to_underlying(god)][std::to_underlying(kind)];
}

struct OfferingGift {
	Boon boon; // a stat gift
	std::optional<ItemKind> item; // an item gift, for the bag
	[[nodiscard]] bool Any() const { return boon.Any() || item.has_value(); }
};

// The gift (none, mostly) of an offering to `god` on campaign level `level`. Seeded by the run (RunStats::nameSeed)
// and the offering's number in the run (RunStats::offerings, before it counts this one): a quick load gives the same
// gift again. always: every offering brings one (the play scripts' `gifts`); the god's weights still pick which.
// The item is a generated treasure chest's for the level's depth (RollTreasureItem).
[[nodiscard]] OfferingGift
RollOfferingGift(God god, int level, std::uint32_t runSeed, int offering, bool always = false);

#endif
