#include "poison.h"
#include <algorithm>
#include <istream>
#include <string>
#include <ostream>

void Poison::Apply(PoisonTier tier, bool halved) {
	TierTimer& t = tiers[static_cast<size_t>(tier)];
	if (t.leftMs <= 0)
		t = {};
	const int duration = POISON_TIERS[static_cast<size_t>(tier)].durationMs;
	t.leftMs = halved ? duration / 2 : duration;
}

int Poison::Advance(int ms, int maxHp, int fasterPercent) {
	int hp = 0;
	const int pace = 100 + std::max(0, fasterPercent);
	for (size_t i = 0; i < tiers.size(); i++) {
		TierTimer& t = tiers[i];
		if (t.leftMs <= 0)
			continue;
		const int drain = ms * pace / 100; // of the time left
		const int spent = drain < t.leftMs ? ms : t.leftMs * 100 / pace; // of the clock, for the ticks
		t.leftMs = std::max(0, t.leftMs - drain);
		t.sinceTickMs += spent;
		while (t.sinceTickMs >= 1000) {
			t.sinceTickMs -= 1000;
			const PoisonTierDef& def = POISON_TIERS[i];
			const int hundredths = std::max(def.floorHpPerSecond * 100, maxHp * def.percentOfMaxHp) + t.carryHundredths;
			t.carryHundredths = hundredths % 100;
			hp += hundredths / 100;
		}
		if (t.leftMs <= 0)
			t = {};
	}
	return hp;
}

bool Poison::Any() const { return Mask() != 0; }

int Poison::Mask() const {
	int mask = 0;
	for (size_t i = 0; i < tiers.size(); i++)
		if (tiers[i].leftMs > 0)
			mask |= 1 << i;
	return mask;
}

void Poison::Save(std::ostream& out) const {
	out << "POISON";
	for (const TierTimer& t : tiers)
		out << " " << t.leftMs << " " << t.sinceTickMs;
	out << "\n";
}

void Poison::Load(std::istream& in) {
	Cure();
	const auto start = in.tellg();
	std::string tag;
	if (!(in >> tag) || tag != "POISON") {
		in.clear();
		in.seekg(start);
		return;
	}
	for (TierTimer& t : tiers)
		in >> t.leftMs >> t.sinceTickMs;
}
