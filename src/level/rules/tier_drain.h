#ifndef TIER_DRAIN_H
#define TIER_DRAIN_H

// A condition in three tiers that drains HP each second, each tier on its own timer: poison (plan poison-and-antidote)
// and bleeding (plan bleeding) share it. A hit of a running tier restarts its timer at the same strength; a hit of
// another tier runs beside it.

#include <array>
#include <iosfwd>

constexpr int DRAIN_TIER_COUNT = 3;

// A second's damage is max(floorHpPerSecond, percent of the victim's max HP), plan stronger-poisons.
struct DrainTierDef {
	int floorHpPerSecond;
	int percentOfMaxHp;
	int durationMs;
};
using DrainTierDefs = std::array<DrainTierDef, DRAIN_TIER_COUNT>;

class TierDrain {
  private:
	struct TierTimer {
		int leftMs = 0;
		int sinceTickMs = 0; // a tier deals its damage once a full second has passed
		int carryHundredths = 0; // the fraction of a HP of the percent rate not dealt yet (not saved)
	};
	std::array<TierTimer, DRAIN_TIER_COUNT> tiers{};

  public:
	// (Re)starts that tier's timer at durationMs.
	void Apply(size_t tier, int durationMs);
	void Stop() { tiers = {}; }
	// Time passes: the HP the running tiers take in it, the percent tiers by the victim's max HP. pacePercent: the time
	// left runs down at that pace (150: half again as fast, 50: half as fast); the damage a second stays.
	int Advance(int ms, int maxHp, const DrainTierDefs& defs, int pacePercent = 100);
	[[nodiscard]] int LeftMs(size_t tier) const { return tiers[tier].leftMs; }
	[[nodiscard]] bool Any() const { return Mask() != 0; }
	[[nodiscard]] int Mask() const; // bit per running tier: weak 1, medium 2, strong 4
	// "<tag> left since left since left since" on a line.
	void Save(std::ostream& out, const char* tag) const;
	void Load(std::istream& in, const char* tag); // no line with that tag (an older save): none running
};

#endif
