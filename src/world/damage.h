#ifndef DAMAGE_H
#define DAMAGE_H

// Damage types (plan damage-types-and-resistances): every weapon deals a mix of them, every monster type takes each at
// its own rate, so each monster has a weapon that works best against it.

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

// Fire (plan fire-damage) also sets its target burning (Burning) and goes through armour.
enum class DamageType : unsigned char { Blunt, Slash, Pierce, Fire };
constexpr int DAMAGE_TYPE_COUNT = 4;

constexpr std::array<const char*, DAMAGE_TYPE_COUNT> DAMAGE_TYPE_NAMES = {"smash", "cut", "stab", "fire"};
constexpr auto FIRE = static_cast<size_t>(DamageType::Fire);

// A weapon's or a monster's attack damage, in percent per type; adds up to 100.
using DamageMix = std::array<int, DAMAGE_TYPE_COUNT>;
// A monster type's (or the player's) share of each damage type it takes, in percent.
using Resistances = std::array<int, DAMAGE_TYPE_COUNT>;

constexpr int WEAK = 200;
constexpr int NORMAL = 100;
constexpr int RESISTS = 50;
constexpr int TOUGH = 25;
constexpr Resistances NO_RESISTANCES = {NORMAL, NORMAL, NORMAL, NORMAL};

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

// The fire share of a hit of `damage` after the resistances, in HP (not rounded); 0 for a hit without fire.
[[nodiscard]] inline double fireDamage(int damage, const DamageMix& mix, const Resistances& resist) {
	return static_cast<double>(damage) * mix[FIRE] * resist[FIRE] / 10000.0;
}

// How the traps hurt the player (plan monster-attack-damage-types).
constexpr DamageMix SPIKE_ATTACK_MIX = {0, 20, 80}; // spikes and the death trap
constexpr DamageMix ROCK_ATTACK_MIX = {100, 0, 0};
constexpr DamageMix DART_ATTACK_MIX = {0, 0, 100};
constexpr DamageMix ROAR_MIX = {100, 0, 0}; // a storm roar's blast of sand (RoarRules, plan hippo-boss)

// A hit on the player: the type resistances first, then the armour's flat amount (none if ignoreArmor); at least 1.
// Resistance first keeps its share worth the same on weak and strong hits. Armour acts on the non-fire shares only,
// never below 0 for them: fire goes through it (plan fire-damage).
[[nodiscard]] inline int playerHitDamage(int damage, const DamageMix& mix, const Resistances& resist, int armor,
										 bool ignoreArmor) {
	if (ignoreArmor)
		return resistedDamage(damage, mix, resist);
	long sum = 0;
	for (size_t t = 0; t < DAMAGE_TYPE_COUNT; t++)
		sum += t == FIRE ? 0 : static_cast<long>(mix[t]) * resist[t];
	const double other = static_cast<double>(damage) * static_cast<double>(sum) / 10000.0;
	const double dealt = fireDamage(damage, mix, resist) + std::max(0.0, other - armor);
	return std::max(1, static_cast<int>(std::lround(dealt)));
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

// The journal's sentence on what a creature's attack deals: the largest share as the main phrase, each smaller one
// as "a hint of"; an even split names both as the main.
[[nodiscard]] inline std::string attackSentence(const DamageMix& mix) {
	constexpr std::array<const char*, DAMAGE_TYPE_COUNT> MAIN = {"smashes me hard", "cuts me deep",
																 "stabs me to the bone", "sets me on fire"};
	constexpr std::array<const char*, DAMAGE_TYPE_COUNT> VERB = {"smashes", "cuts", "stabs", "burns"};
	constexpr std::array<const char*, DAMAGE_TYPE_COUNT> HINT = {"smashing pain", "cutting pain", "stabbing pain",
																 "burning pain"};
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
