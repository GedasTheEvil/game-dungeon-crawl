#ifndef POISON_H
#define POISON_H

// The player's poison, without the player: three tiers, each running on its own timer (plan poison-and-antidote,
// TierDrain). A hit of a running tier restarts its timer at the same strength; a hit of another tier runs beside it.
// Armour does not help and poison can kill. The antidote cures every tier.

#include <algorithm>
#include <array>
#include <cstdint>
#include <iosfwd>
#include <utility>

#include "tier_drain.h"

enum class PoisonTier : std::uint8_t { Weak, Medium, Strong };
constexpr int POISON_TIER_COUNT = DRAIN_TIER_COUNT;

using PoisonTierDef = DrainTierDef;
// Weak (scorpion) 1 HP/s, medium (cobra) 1% (at least 2) for 60 s, strong (scorpion queen) 5% (at least 3) for 19 s.
constexpr DrainTierDefs POISON_TIERS = {{{1, 0, 20000}, {2, 1, 60000}, {3, 5, 19000}}};

class Poison {
  private:
	TierDrain drain;

  public:
	// (Re)starts that tier's timer; halved: half the time at the same damage a second (Serket's favour).
	void Apply(PoisonTier tier, bool halved = false) {
		const int duration = POISON_TIERS[std::to_underlying(tier)].durationMs;
		drain.Apply(std::to_underlying(tier), halved ? duration / 2 : duration);
	}
	void Cure() { drain.Stop(); }
	// Time passes: the HP the running tiers take in it, the percent tiers by the victim's max HP. fasterPercent (the
	// feldspar rings): the time left runs down that much faster, the damage a second stays.
	int Advance(int ms, int maxHp, int fasterPercent = 0) {
		return drain.Advance(ms, maxHp, POISON_TIERS, 100 + std::max(0, fasterPercent));
	}
	[[nodiscard]] int LeftMs(PoisonTier tier) const { return drain.LeftMs(std::to_underlying(tier)); }
	[[nodiscard]] bool Running(PoisonTier tier) const { return LeftMs(tier) > 0; }
	[[nodiscard]] bool Any() const { return drain.Any(); }
	[[nodiscard]] int Mask() const { return drain.Mask(); } // bit per running tier: weak 1, medium 2, strong 4
	// Each tier's time left, after the player's stats in the save.
	void Save(std::ostream& out) const { drain.Save(out, "POISON"); }
	void Load(std::istream& in) { drain.Load(in, "POISON"); } // a save from before poison has none: no poison
};

#endif
