#ifndef VITALS_H
#define VITALS_H

// Shared vitals (plan shared-vitals): what the player and a monster both live through (HP, poison, fire, water, a trap,
// a knock out of action). A part both own by value (PlayerStats::vitals, Monster::vitals), not a parent class: the
// rules live here once, the owner keeps its own sounds, blood, journal notes, XP and god mode.

#include <algorithm>
#include <cstdint>
#include <optional>

#include "../core/timer.h"
#include "damage.h"
#include "poison.h"
#include "rng.h"
#include "trap_hurt.h"

// Who dealt a harm. Weapon: the player's doing (a weapon, an arrow, the venom amulet, a sunbeam): a kill by its poison
// or fire is the player's (Vitals::poisonByPlayer, burnByPlayer). Monster: a bite, a shot, a screech. Trap: no XP for
// the kill.
enum class HarmSource : std::uint8_t { Weapon, Monster, Trap };

// A hit: its damage before the resistances, its mix, who dealt it.
struct Harm {
	int damage = 0;
	DamageMix mix{};
	HarmSource source = HarmSource::Monster;
	bool ignoreArmor = false; // a falling rock, a trap crossing
	int burnMs = BURN_MS; // its fire share burns that long
	int factor = 1; // times the hit after the resistances (a stunned monster: CHARGE_STUN_DAMAGE_FACTOR)
};

// How the body takes a hit, the owner's numbers of the moment: the player's change with the amulet, the curses and
// the blessings, so they are passed in, not kept.
struct Guard {
	Resistances resist = NO_RESISTANCES;
	std::optional<int> armor; // the player's (playerHitDamage: armour on the non-fire shares); a monster has none
};

class Vitals {
  private:
	int trapCarry = 0; // thousandths of a HP of trap damage not taken yet (TrapLoss)
	int outUntilMs = -1; // out of action (KnockOut) until this GameClock time

  public:
	explicit Vitals(int startHp = 0) : hp(startHp) {}
	int hp;
	Poison poison;
	Burning burning; // not saved, it lasts seconds
	bool inWater = false; // standing in half water: a hit's fire share is cut (inWaterResist), a fire put out
	TrapHurt trapHurt; // the pace and ramp of a spike or death trap it stands in
	int blazeReadyMs = 0; // GameClock: its next hit standing in a burning floor (Dungeon::updateBlazes)
	bool poisonByPlayer = false; // a running tier is the player's doing: its kill is theirs
	bool burnByPlayer = false; // the fire is the player's doing: its kill is theirs, a trap's not

	[[nodiscard]] bool Alive() const { return hp > 0; }
	// HP off, armour or not (the caller worked the hit out). Returns the HP lost: never more than it had.
	int Lose(int dmg) {
		const int lost = std::clamp(dmg, 0, std::max(0, hp));
		hp -= dmg;
		return lost;
	}
	// The resistances it takes a hit with: in water the fire rate is cut (plan fire-in-water).
	[[nodiscard]] Resistances Taken(const Resistances& resist) const {
		return inWater ? inWaterResist(resist) : resist;
	}
	// The HP a hit costs: the resistances (in water: Taken), then the armour on the non-fire shares if the guard has
	// any (playerHitDamage), at least 1; times the harm's factor.
	[[nodiscard]] int Cost(const Harm& harm, const Guard& guard) const {
		const Resistances resist = Taken(guard.resist);
		const int hit = guard.armor ? playerHitDamage(harm.damage, harm.mix, resist, *guard.armor, harm.ignoreArmor)
									: resistedDamage(harm.damage, harm.mix, resist);
		return hit * harm.factor;
	}
	// The hit's fire share in HP (not rounded) after the resistances (in water: Taken); 0 without fire.
	[[nodiscard]] double FireOf(const Harm& harm, const Resistances& resist) const {
		return fireDamage(harm.damage, harm.mix, Taken(resist));
	}
	// Set burning by fire HP for ms (Burning::Ignite). The player's doing (Weapon) makes the fire's kill theirs; a
	// trap's leaves a running fire of the player's theirs.
	void Ignite(double fire, int ms, HarmSource source) {
		burnByPlayer = source == HarmSource::Weapon || (burnByPlayer && burning.Any());
		burning.Ignite(fire, ms);
	}
	// A trap's damage (plan same-hazards-for-all), hundredths of a HP before the body's share (per-mille,
	// FULL_TRAP_SHARE all of it, 0 immune): the whole HP due now, the fraction carried to the next (so the small trap
	// hits count too). Every trap hurts the player and the monsters through here.
	int TrapLoss(int hundredths, int permille) {
		const long long total = static_cast<long long>(hundredths) * std::max(0, permille) / 100 + trapCarry;
		trapCarry = static_cast<int>(total % 1000);
		return static_cast<int>(total / 1000);
	}
	// A poisoned hit: that tier (re)starts (halved: half the time, Serket's favour) unless the ward (percent, rolled on
	// rng only when > 0) shrugs it off. True: it took.
	bool TakePoison(PoisonTier tier, int wardPercent, Rng& rng, HarmSource source, bool halved = false) {
		if (wardPercent > 0 && rng.percent(wardPercent))
			return false;
		poisonByPlayer = source == HarmSource::Weapon || (poisonByPlayer && poison.Any());
		poison.Apply(tier, halved);
		return true;
	}
	// Time passes for the poison: the HP its running tiers take (armour does not help). Dead: cured, none.
	int PoisonTick(int ms, int maxHp, int fasterPercent = 0) {
		if (!Alive()) {
			poison.Cure();
			return 0;
		}
		return poison.Advance(ms, maxHp, fasterPercent);
	}
	// Time passes for the fire: in water or dead it goes out, else the HP it burns (armour does not help).
	int BurnTick(int ms) {
		if (inWater || !Alive()) {
			burning.Douse();
			return 0;
		}
		return burning.Advance(ms);
	}
	// Out of action for ms, at least what is left of an earlier knock: the player is dazed (Player::Daze), a monster
	// stunned (Monster::StunHit).
	void KnockOut(int ms) { outUntilMs = std::max(outUntilMs, GameClock::now() + ms); }
	void ComeTo() { outUntilMs = -1; }
	[[nodiscard]] int OutLeftMs() const { return std::max(0, outUntilMs - GameClock::now()); }
	[[nodiscard]] bool Out() const { return OutLeftMs() > 0; }
	// A knock on the head (plans head-bump, monster-head-bump): headBumpHp off the HP, at most what it has.
	[[nodiscard]] int HeadBumpLoss(int maxHp) const;
	// The fire, the poison, the knock and the trap carry gone (a death, a spawn, a new life); the HP stays.
	void Clear();
};

#endif
