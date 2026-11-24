#ifndef ARCHER_SKILLS_H
#define ARCHER_SKILLS_H

#include "damage.h"
#include "../tuning/gameplay_skills.h"
#include "poison.h"
#include <algorithm>

// The rules of the archer skills (plan archer-skills): Arrows of Satet (SkillId::SharpArrows), Sting of Serket
// (PoisonArrows) and Fang of Wadjet (PoisonBlade). Pure, so the unit tests roll them without a dungeon. Rank: 0 not
// learnt, 1, 2 (Heka::Rank).

// Arrows of Satet: the cut a bow shot carries on top of its damage, in total for the rank.
[[nodiscard]] constexpr int sharpArrowCut(int rank) {
	return rank == 2 ? SHARP_ARROW_CUT_2 : (rank == 1 ? SHARP_ARROW_CUT : 0);
}

// A shot of `damage` and `mix` with `cut` added: the total damage, the mix recomputed (the added points are all cut,
// the rounding goes to the cut).
[[nodiscard]] constexpr int addCut(int damage, DamageMix& mix, int cut) {
	if (cut <= 0)
		return damage;
	const int total = damage + cut;
	int rest = 100;
	for (size_t t = 0; t < DAMAGE_TYPE_COUNT; t++)
		if (t != static_cast<size_t>(DamageType::Slash)) {
			mix[t] = mix[t] * damage / total;
			rest -= mix[t];
		}
	mix[static_cast<size_t>(DamageType::Slash)] = rest;
	return total;
}

// Arrows of Satet's bleed: the first tier a bow hit starts (weak at rank I, medium at rank II), or -1 without the
// skill.
[[nodiscard]] constexpr int sharpBleedStart(int rank) { return rank == 2 ? 1 : (rank == 1 ? 0 : -1); }

// The base chance, before the monster's cut rate (bleedChancePercent), that `tier` starts from a bow hit: the start
// tier at SHARP_ARROW_BLEED_PERCENT, each tier above at half the one below (on a monster that bleeds the one below). A
// boss at BOSS_BLEED_PERCENT of it. 0: this tier does not start.
[[nodiscard]] constexpr int sharpBleedPercent(int rank, int tier, bool boss) {
	const int start = sharpBleedStart(rank);
	if (start < 0 || tier < start)
		return 0;
	int chance = SHARP_ARROW_BLEED_PERCENT;
	for (int up = start; up < tier; up++)
		chance = chance * BLEED_TIER_UP_PERCENT / 100;
	return boss ? chance * BOSS_BLEED_PERCENT / 100 : chance;
}

// Sting of Serket and Fang of Wadjet: the chance (percent) of the skill at that rank and the tier it poisons with.
[[nodiscard]] constexpr int poisonSkillPercent(int rank) { return rank > 0 ? POISON_SKILL_PERCENT : 0; }
[[nodiscard]] constexpr PoisonTier poisonSkillTier(int rank) {
	return rank == 2 ? PoisonTier::Strong : PoisonTier::Medium;
}

// A hit's poison: the venom amulet (percent, tier; none at 0) and the skill that fits the hit (rank, 0: none) roll as
// one: the chances add (at most 100), the stronger tier counts.
struct PoisonRoll {
	int percent;
	PoisonTier tier;
};
[[nodiscard]] constexpr PoisonRoll combinedPoison(int amuletPercent, PoisonTier amuletTier, int skillRank) {
	const int skill = poisonSkillPercent(skillRank);
	PoisonTier tier = amuletPercent > 0 ? amuletTier : poisonSkillTier(skillRank);
	if (amuletPercent > 0 && skill > 0)
		tier = std::max(amuletTier, poisonSkillTier(skillRank));
	return {std::min(100, amuletPercent + skill), tier};
}

#endif
