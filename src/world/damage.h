#ifndef DAMAGE_H
#define DAMAGE_H

// Damage types (docs/plan/solved/damage-types-and-resistances.md): every weapon deals a mix of them, every monster type
// takes each at its own rate, so each monster has a weapon that works best against it.

#include <algorithm>
#include <array>
#include <cmath>

enum class DamageType : unsigned char { Blunt, Slash, Pierce };
constexpr int DAMAGE_TYPE_COUNT = 3;

constexpr std::array<const char*, DAMAGE_TYPE_COUNT> DAMAGE_TYPE_NAMES = {"blunt", "slash", "pierce"};

// A weapon's damage, in percent per type; adds up to 100.
using DamageMix = std::array<int, DAMAGE_TYPE_COUNT>;
// A monster type's share of each damage type it takes, in percent.
using Resistances = std::array<int, DAMAGE_TYPE_COUNT>;

constexpr int WEAK = 200;
constexpr int NORMAL = 100;
constexpr int RESISTS = 50;
constexpr int TOUGH = 25;
constexpr Resistances NO_RESISTANCES = {NORMAL, NORMAL, NORMAL};

// How the journal writes a rate down: "weak", "normal", "resists", "tough".
[[nodiscard]] constexpr const char* resistanceWord(int rate) {
	return rate > NORMAL ? "weak" : rate == NORMAL ? "normal" : rate >= RESISTS ? "resists" : "tough";
}

// The type the weapon deals most of: the one the journal learns from its hit.
[[nodiscard]] constexpr DamageType mainType(const DamageMix& mix) {
	int best = 0;
	for (int t = 1; t < DAMAGE_TYPE_COUNT; t++)
		if (mix[t] > mix[best])
			best = t;
	return static_cast<DamageType>(best);
}

// A weapon hit of `damage` after the monster's resistances: sum(damage * share * rate), rounded, at least 1.
[[nodiscard]] inline int resistedDamage(int damage, const DamageMix& mix, const Resistances& resist) {
	long sum = 0;
	for (int t = 0; t < DAMAGE_TYPE_COUNT; t++)
		sum += static_cast<long>(mix[t]) * resist[t];
	const double dealt = static_cast<double>(damage) * static_cast<double>(sum) / 10000.0;
	return std::max(1, static_cast<int>(std::lround(dealt)));
}

#endif
