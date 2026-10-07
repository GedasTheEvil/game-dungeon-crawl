#ifndef DAMAGE_H
#define DAMAGE_H

// Damage types (docs/plan/solved/damage-types-and-resistances.md): every weapon deals a mix of them, every monster type
// takes each at its own rate, so each monster has a weapon that works best against it.

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

enum class DamageType : unsigned char { Blunt, Slash, Pierce };
constexpr int DAMAGE_TYPE_COUNT = 3;

constexpr std::array<const char*, DAMAGE_TYPE_COUNT> DAMAGE_TYPE_NAMES = {"blunt", "slash", "pierce"};

// A weapon's or a monster's attack damage, in percent per type; adds up to 100.
using DamageMix = std::array<int, DAMAGE_TYPE_COUNT>;
// A monster type's (or the player's) share of each damage type it takes, in percent.
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

// How the traps hurt the player (docs/plan/solved/monster-attack-damage-types.md).
constexpr DamageMix SPIKE_ATTACK_MIX = {0, 20, 80}; // spikes and the death trap
constexpr DamageMix ROCK_ATTACK_MIX = {100, 0, 0};
constexpr DamageMix DART_ATTACK_MIX = {0, 0, 100};

// A hit on the player: the type resistances first, then the armour's flat amount (none if ignoreArmor); at least 1.
// Resistance first keeps its share worth the same on weak and strong hits.
[[nodiscard]] inline int playerHitDamage(int damage, const DamageMix& mix, const Resistances& resist, int armor,
										 bool ignoreArmor) {
	const int resisted = resistedDamage(damage, mix, resist);
	return std::max(1, ignoreArmor ? resisted : resisted - armor);
}

// The journal's sentence on what a creature's attack deals: the largest share as the main phrase, each smaller one
// as "a hint of"; an even split names both as the main.
[[nodiscard]] inline std::string attackSentence(const DamageMix& mix) {
	constexpr std::array<const char*, DAMAGE_TYPE_COUNT> MAIN = {"hits me bluntly hard", "cuts me deep",
																 "pierces me to the bone"};
	constexpr std::array<const char*, DAMAGE_TYPE_COUNT> VERB = {"crushes", "cuts", "pierces"};
	constexpr std::array<const char*, DAMAGE_TYPE_COUNT> HINT = {"crushing pain", "cutting pain", "piercing pain"};
	const int top = mix[static_cast<size_t>(mainType(mix))];
	std::string mains, hints;
	int mainCount = 0;
	for (size_t t = 0; t < DAMAGE_TYPE_COUNT; t++) {
		if (mix[t] == 0)
			continue;
		if (mix[t] == top) {
			mains += std::string(mainCount++ == 0 ? "" : " and ") + VERB[t];
		} else {
			hints += std::string(hints.empty() ? "" : " and ") + HINT[t];
		}
	}
	std::string s = "It ";
	s += mainCount == 1 ? MAIN[static_cast<size_t>(mainType(mix))] : mains + " me alike";
	if (!hints.empty())
		s += ", with a hint of " + hints;
	return s + ".";
}

#endif
