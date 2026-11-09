#ifndef DAMAGE_H
#define DAMAGE_H

// Damage types (plan damage-types-and-resistances): every weapon deals a mix of them, every monster type takes each at
// its own rate, so each monster has a weapon that works best against it.

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

#include "gameplay_config.h"

// Fire (plan fire-damage) also sets its target burning (Burning) and goes through armour.
enum class DamageType : unsigned char { Blunt, Slash, Pierce, Fire };
constexpr int DAMAGE_TYPE_COUNT = 4;

constexpr std::array<const char*, DAMAGE_TYPE_COUNT> DAMAGE_TYPE_NAMES = {"smash", "cut", "stab", "fire"};
constexpr auto FIRE = static_cast<size_t>(DamageType::Fire);

// A weapon's or a monster's attack damage, in percent per type; adds up to 100.
using DamageMix = std::array<int, DAMAGE_TYPE_COUNT>;
// A monster type's (or the player's) share of each damage type it takes, in percent. Not whole: the amethyst rings
// move a rate a share of a tier (amethystResist: 37.5).
using Resistances = std::array<float, DAMAGE_TYPE_COUNT>;

// The tiers (plan resistance-tiers), the most damage taken first.
constexpr int VULNERABLE = 200;
constexpr int WEAK = 150;
constexpr int NORMAL = 100;
constexpr int RESISTS = 50;
constexpr int TOUGH = 25;
constexpr int IMMUNE = 0; // the type does no damage: no minimum of 1 (resistedDamage)
constexpr std::array<int, 6> RESIST_TIERS = {IMMUNE, TOUGH, RESISTS, NORMAL, WEAK, VULNERABLE};
constexpr Resistances NO_RESISTANCES = {NORMAL, NORMAL, NORMAL, NORMAL};

// How the journal writes a rate down: "vulnerable", "weak", "normal", "resists", "tough", "immune". A rate between
// two tiers (an amethyst's) takes the word of the tier under it.
[[nodiscard]] constexpr const char* resistanceWord(float rate) {
	return rate >= VULNERABLE ? "vulnerable"
		   : rate >= WEAK	  ? "weak"
		   : rate >= NORMAL	  ? "normal"
		   : rate >= RESISTS  ? "resists"
		   : rate > IMMUNE	  ? "tough"
							  : "immune";
}

// The type the weapon deals most of: the one the journal learns from its hit.
[[nodiscard]] constexpr DamageType mainType(const DamageMix& mix) {
	int best = 0;
	for (int t = 1; t < DAMAGE_TYPE_COUNT; t++)
		if (mix[t] > mix[best])
			best = t;
	return static_cast<DamageType>(best);
}

// A hit's damage rounded, at least 1, unless every type it deals is an immune one (plan resistance-tiers): then 0.
[[nodiscard]] inline int hitDamage(double dealt, bool hurts) {
	return hurts ? std::max(1, static_cast<int>(std::lround(dealt))) : 0;
}

// A weapon hit of `damage` after the monster's resistances: sum(damage * share * rate), rounded, at least 1 (0 if it
// only deals types it is immune to).
[[nodiscard]] inline int resistedDamage(int damage, const DamageMix& mix, const Resistances& resist) {
	double sum = 0.0;
	for (size_t t = 0; t < DAMAGE_TYPE_COUNT; t++)
		sum += static_cast<double>(mix[t]) * resist[t];
	return hitDamage(static_cast<double>(damage) * sum / 10000.0, sum > 0.0);
}

// The amethyst rings (plan resistance-tiers): each rate moves `percent` of a tier up the ladder (RESIST_TIERS), the
// step the gap to the next tier: rate + (next - rate) * share. Over 100% it goes on into the tier after; VULNERABLE is
// the top. IMMUNE stays (only a whole-tier ring may move it: sky-iron-rings). Gold (50%) on TOUGH: 37.5.
[[nodiscard]] constexpr Resistances amethystResist(Resistances resist, int percent) {
	for (float& rate : resist) {
		if (rate <= IMMUNE || percent <= 0)
			continue;
		float left = static_cast<float>(percent) / 100.f; // tiers still to climb
		for (size_t i = 1; i < RESIST_TIERS.size() && left > 0.f; i++) {
			const auto next = static_cast<float>(RESIST_TIERS[i]);
			if (rate >= next)
				continue;
			const auto below = static_cast<float>(RESIST_TIERS[i - 1]);
			const float toNext = (next - rate) / (next - below); // the share of this tier left to climb
			if (left < toNext) {
				rate += left * (next - below);
				left = 0.f;
			} else {
				rate = next;
				left -= toNext;
			}
		}
	}
	return resist;
}

// The resistances of one standing in water (plan fire-in-water): its fire rate is WATER_FIRE_DAMAGE_PCT % of the
// usual, the other types as they are.
[[nodiscard]] inline Resistances inWaterResist(Resistances resist) {
	resist[FIRE] = resist[FIRE] * WATER_FIRE_DAMAGE_PCT / 100.f;
	return resist;
}

// The fire share of a hit of `damage` after the resistances, in HP (not rounded); 0 for a hit without fire.
[[nodiscard]] inline double fireDamage(int damage, const DamageMix& mix, const Resistances& resist) {
	return static_cast<double>(damage) * mix[FIRE] * static_cast<double>(resist[FIRE]) / 10000.0;
}

// How the traps hurt the player (plan monster-attack-damage-types).
constexpr DamageMix SPIKE_ATTACK_MIX = {0, 20, 80}; // spikes and the death trap
constexpr DamageMix ROCK_ATTACK_MIX = {100, 0, 0};
constexpr DamageMix DART_ATTACK_MIX = {0, 0, 100};
constexpr DamageMix SCREECH_MIX = {100, 0, 0}; // a screech's blast of sound (ScreechRules, plan screech-bite)
constexpr DamageMix ROAR_MIX = {100, 0, 0}; // a storm roar's blast of sand (RoarRules, plan hippo-boss)
constexpr DamageMix DUNG_BALL_MIX = {100, 0, 0}; // a scarab's dung ball (RollRules, plan scarab-dung-ball)
constexpr DamageMix FIRE_JET_MIX = {0, 0, 0, 100}; // a fire vent's jet (plan fire-jet-trap)
constexpr DamageMix DRIP_DROP_MIX = {100, 0, 0}; // a poison drip's falling lump (plan poison-drip-trap)
constexpr DamageMix BLAZE_MIX = {0, 0, 0, 100}; // a floor set burning by Babi's big coal (plan babi-boss)

// The armour that halves a hit (plan monster-balance): armour takes armor / (armor + ARMOR_HALVES) of it, 17% at 10,
// half at 50, never all. A share, not a flat amount: a flat one cut every hit under it to 1, so a common monster
// stopped hurting once the potions and amulets had stacked the armour up (the balance sim, 2026-10-26).
constexpr int ARMOR_HALVES = 50;

// A hit on the player: the type resistances first, then the armour's share (none if ignoreArmor); at least 1.
// Resistance first keeps its share worth the same on weak and strong hits. Armour acts on the non-fire shares only:
// fire goes through it (plan fire-damage).
[[nodiscard]] inline int
playerHitDamage(int damage, const DamageMix& mix, const Resistances& resist, int armor, bool ignoreArmor) {
	if (ignoreArmor)
		return resistedDamage(damage, mix, resist);
	double sum = 0.0;
	for (size_t t = 0; t < DAMAGE_TYPE_COUNT; t++)
		sum += t == FIRE ? 0.0 : static_cast<double>(mix[t]) * resist[t];
	const double other = static_cast<double>(damage) * sum / 10000.0;
	const double kept = static_cast<double>(ARMOR_HALVES) / static_cast<double>(ARMOR_HALVES + std::max(0, armor));
	const double fire = fireDamage(damage, mix, resist);
	return hitDamage(fire + other * kept, sum > 0.0 || fire > 0.0);
}

// Fire sets its target burning (plan fire-damage): for BURN_MS it takes BURN_TICK_PERCENT % of the hit's fire share a
// second, armour or not. Not poison: no antidote or poison ward helps; wading into half water puts it out.
constexpr int BURN_MS = 3000;
constexpr int BURN_TICK_PERCENT = 20;

class Burning {
  private:
	int leftMs = 0;
	int sinceTickMs = 0; // it burns once a full second has passed
	int hundredthsPerSecond = 0; // HP a second, in hundredths
	int carryHundredths = 0; // the fraction of a HP not dealt yet

  public:
	// A hit's fire share of fire HP (fireDamage) lands: burning (again) for ms. A stronger fire takes over, a weaker
	// one only restarts the time.
	void Ignite(double fire, int ms = BURN_MS) {
		if (fire <= 0.0)
			return;
		if (leftMs <= 0)
			*this = {};
		const auto rate = static_cast<int>(std::lround(fire * BURN_TICK_PERCENT));
		hundredthsPerSecond = std::max({hundredthsPerSecond, rate, 1});
		leftMs = std::max(leftMs, ms);
	}
	void Douse() { *this = {}; }
	// Time passes: the HP it burns in it.
	int Advance(int ms) {
		if (leftMs <= 0)
			return 0;
		const int spent = std::min(ms, leftMs);
		leftMs -= spent;
		sinceTickMs += spent;
		int hp = 0;
		while (sinceTickMs >= 1000) {
			sinceTickMs -= 1000;
			const int hundredths = hundredthsPerSecond + carryHundredths;
			carryHundredths = hundredths % 100;
			hp += hundredths / 100;
		}
		if (leftMs <= 0)
			Douse();
		return hp;
	}
	[[nodiscard]] bool Any() const { return leftMs > 0; }
	[[nodiscard]] int LeftMs() const { return leftMs; }
};

#endif
