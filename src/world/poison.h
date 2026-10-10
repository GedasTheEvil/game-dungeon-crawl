#ifndef POISON_H
#define POISON_H

// The player's poison, without the player: three tiers, each running on its own timer
// (docs/plan/solved/poison-and-antidote.md). A hit of a running tier restarts its timer at the same strength; a hit of
// another tier runs beside it. Armour does not help and poison can kill. The antidote cures every tier.

#include <array>
#include <cstdint>
#include <iosfwd>

enum class PoisonTier : std::uint8_t { Weak, Medium, Strong };
constexpr int POISON_TIER_COUNT = 3;

// A second's damage is max(floorHpPerSecond, percent of the victim's max HP), docs/plan/stronger-poisons.md.
struct PoisonTierDef {
	int floorHpPerSecond;
	int percentOfMaxHp;
	int durationMs;
};
// Weak (scorpion) 1 HP/s, medium (cobra) 1% (at least 2) for 60 s, strong (scorpion queen) 5% (at least 3) for 19 s.
constexpr std::array<PoisonTierDef, POISON_TIER_COUNT> POISON_TIERS = {{{1, 0, 20000}, {2, 1, 60000}, {3, 5, 19000}}};

class Poison {
  private:
	struct TierTimer {
		int leftMs = 0;
		int sinceTickMs = 0;	 // a tier deals its damage once a full second has passed
		int carryHundredths = 0; // the fraction of a HP of the percent rate not dealt yet (not saved)
	};
	std::array<TierTimer, POISON_TIER_COUNT> tiers{};

  public:
	void Apply(PoisonTier tier); // (re)starts that tier's timer
	void Cure() { tiers = {}; }
	// Time passes: the HP the running tiers take in it, the percent tiers by the victim's max HP.
	int Advance(int ms, int maxHp);
	[[nodiscard]] int LeftMs(PoisonTier tier) const { return tiers[static_cast<size_t>(tier)].leftMs; }
	[[nodiscard]] bool Running(PoisonTier tier) const { return LeftMs(tier) > 0; }
	[[nodiscard]] bool Any() const;
	[[nodiscard]] int Mask() const; // bit per running tier: weak 1, medium 2, strong 4
	// Each tier's time left, after the player's stats in the save.
	void Save(std::ostream& out) const;
	void Load(std::istream& in); // a save from before poison has none: no poison
};

#endif
