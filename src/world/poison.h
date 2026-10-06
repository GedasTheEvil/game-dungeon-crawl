#ifndef POISON_H
#define POISON_H

// The player's poison, without the player: three tiers, each running on its own timer
// (docs/plan/poison-and-antidote.md). A hit of a running tier restarts its timer at the same strength; a hit of another
// tier runs beside it. Armour does not help and poison can kill. The antidote cures every tier.

#include <array>
#include <cstdint>
#include <iosfwd>

enum class PoisonTier : std::uint8_t { Weak, Medium, Strong };
constexpr int POISON_TIER_COUNT = 3;

struct PoisonTierDef {
	int hpPerSecond;
	int durationMs;
};
// Weak (scorpion) 20 HP in all, medium (cobra) 75, strong (scorpion queen) 150.
constexpr std::array<PoisonTierDef, POISON_TIER_COUNT> POISON_TIERS = {{{1, 20000}, {3, 25000}, {5, 30000}}};

class Poison {
  private:
	struct TierTimer {
		int leftMs = 0;
		int sinceTickMs = 0; // a tier deals its damage once a full second has passed
	};
	std::array<TierTimer, POISON_TIER_COUNT> tiers{};

  public:
	void Apply(PoisonTier tier); // (re)starts that tier's timer
	void Cure() { tiers = {}; }
	// Time passes: the HP the running tiers take in it.
	int Advance(int ms);
	[[nodiscard]] int LeftMs(PoisonTier tier) const { return tiers[static_cast<size_t>(tier)].leftMs; }
	[[nodiscard]] bool Running(PoisonTier tier) const { return LeftMs(tier) > 0; }
	[[nodiscard]] bool Any() const;
	[[nodiscard]] int Mask() const; // bit per running tier: weak 1, medium 2, strong 4
	// Each tier's time left, after the player's stats in the save.
	void Save(std::ostream& out) const;
	void Load(std::istream& in); // a save from before poison has none: no poison
};

#endif
