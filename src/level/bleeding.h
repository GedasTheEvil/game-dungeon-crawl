#ifndef BLEEDING_H
#define BLEEDING_H

// Bleeding (plan bleeding): cut damage's own condition, the same for the player and the monsters (Vitals), in three
// tiers like poison (TierDrain). A tick deals cut damage through neither armour nor resistance: the victim's cut
// resistance only decides whether the bleed lands (bleedChancePercent). Any heal stops it, regeneration does not (it
// gives no HP while bleeding). In water its time runs at BLEED_WATER_PACE_PERCENT: it lasts longer.

#include <algorithm>
#include <cstdint>
#include <iosfwd>
#include <utility>

#include "damage.h"
#include "gameplay_config.h"
#include "tier_drain.h"

enum class BleedTier : std::uint8_t { Weak, Medium, Strong };

// A bit weaker than poison's tiers (POISON_TIERS), since any heal stops it: weak 1 HP/s for 10 s, medium 1% (at least
// 2) for 20 s, strong 3% (at least 3) for 12 s.
constexpr DrainTierDefs BLEED_TIERS = {{{1, 0, 10000}, {2, 1, 20000}, {3, 3, 12000}}};
constexpr int BLEED_WATER_PACE_PERCENT = 50; // in water (half or deep) a bleed's time runs at half pace

// A bleed source (plan bleeding): its tier and its base chance (a bite's before its cut share, biteBleedPercent).
struct BleedRoll {
	BleedTier tier = BleedTier::Weak;
	int percent = BLEED_BITE_PERCENT;
};
constexpr BleedRoll SPIKES_BLEED = {BleedTier::Weak, SPIKES_BLEED_PERCENT}; // floor, wall and ceiling spikes
constexpr BleedRoll DEATH_TRAP_BLEED = {BleedTier::Medium, DEATH_TRAP_BLEED_PERCENT}; // the floor and ceiling ones

// A source's base chance of a bleed on a body: times its cut rate (resist[Slash]: resists halves it, immune stops it,
// vulnerable doubles it) and its share of a trap (sharePermille, FULL_TRAP_SHARE all, 0 none: the trap ward, a
// monster's trapPermille); at most 100.
[[nodiscard]] inline int
bleedChancePercent(int basePercent, const Resistances& resist, int sharePermille = FULL_TRAP_SHARE) {
	const double rate = static_cast<double>(resist[std::to_underlying(DamageType::Slash)]) / NORMAL;
	const double share = static_cast<double>(std::max(0, sharePermille)) / FULL_TRAP_SHARE;
	return std::clamp(static_cast<int>(std::lround(basePercent * rate * share)), 0, 100);
}

// A cutting bite's chance per hit (MonsterKind::bleed): its base percent times the attack's cut share.
[[nodiscard]] inline int biteBleedPercent(int basePercent, const DamageMix& mix) {
	return basePercent * mix[std::to_underlying(DamageType::Slash)] / 100;
}

class Bleeding {
  private:
	TierDrain drain;

  public:
	void Apply(BleedTier tier) {
		drain.Apply(std::to_underlying(tier), BLEED_TIERS[std::to_underlying(tier)].durationMs);
	}
	void Stop() { drain.Stop(); }
	// Time passes: the HP the running tiers take in it; in water the time left runs at BLEED_WATER_PACE_PERCENT.
	int Advance(int ms, int maxHp, bool inWater) {
		return drain.Advance(ms, maxHp, BLEED_TIERS, inWater ? BLEED_WATER_PACE_PERCENT : 100);
	}
	[[nodiscard]] int LeftMs(BleedTier tier) const { return drain.LeftMs(std::to_underlying(tier)); }
	[[nodiscard]] bool Any() const { return drain.Any(); }
	[[nodiscard]] int Mask() const { return drain.Mask(); } // bit per running tier: weak 1, medium 2, strong 4
	void Save(std::ostream& out) const { drain.Save(out, "BLEED"); }
	void Load(std::istream& in) { drain.Load(in, "BLEED"); } // a save from before bleeding: none
};

#endif
