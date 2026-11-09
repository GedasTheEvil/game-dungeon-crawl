#include "offering_gift.h"
#include "campaign.h"
#include "loot.h"
#include "rng.h"

OfferingGift RollOfferingGift(God god, int level, std::uint32_t runSeed, int offering, bool always) {
	// Mixed once more, so the next offering's stream does not start one draw into this one's (Rng's seeds are steps).
	Rng mix((static_cast<std::uint64_t>(runSeed) << 32) | static_cast<std::uint32_t>(offering));
	const std::uint64_t high = mix.next();
	const std::uint64_t low = mix.next();
	Rng rng((high << 32) | low);
	OfferingGift gift;
	if (!rng.percent(OFFERING_GIFT_PERCENT) && !always)
		return gift;
	int pick = rng.below(100);
	int k = 0;
	while (k < GIFT_KIND_COUNT - 1 && pick >= giftWeight(god, static_cast<GiftKind>(k))) {
		pick -= giftWeight(god, static_cast<GiftKind>(k));
		k++;
	}
	switch (static_cast<GiftKind>(k)) {
	case GiftKind::Might:
		gift.boon = {BoonKind::Might, GIFT_MIGHT};
		break;
	case GiftKind::Armor:
		gift.boon = {BoonKind::Armor, GIFT_ARMOR};
		break;
	case GiftKind::Life:
		gift.boon = {BoonKind::Life, GIFT_LIFE_PERCENT};
		break;
	case GiftKind::Stamina:
		gift.boon = {BoonKind::Stamina, GIFT_STAMINA_PERCENT};
		break;
	case GiftKind::Item:
		gift.item = RollTreasureItem(campaignDifficulty(level), rng);
		break;
	}
	return gift;
}
