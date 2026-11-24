#include "tier_drain.h"
#include <algorithm>
#include <istream>
#include <ostream>
#include <string>

void TierDrain::Apply(size_t tier, int durationMs) {
	TierTimer& t = tiers[tier];
	if (t.leftMs <= 0)
		t = {};
	t.leftMs = durationMs;
}

int TierDrain::Advance(int ms, int maxHp, const DrainTierDefs& defs, int pacePercent) {
	int hp = 0;
	const int pace = std::max(1, pacePercent);
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
			const DrainTierDef& def = defs[i];
			const int hundredths = std::max(def.floorHpPerSecond * 100, maxHp * def.percentOfMaxHp) + t.carryHundredths;
			t.carryHundredths = hundredths % 100;
			hp += hundredths / 100;
		}
		if (t.leftMs <= 0)
			t = {};
	}
	return hp;
}

int TierDrain::Mask() const {
	int mask = 0;
	for (size_t i = 0; i < tiers.size(); i++)
		if (tiers[i].leftMs > 0)
			mask |= 1 << i;
	return mask;
}

void TierDrain::Save(std::ostream& out, const char* tag) const {
	out << tag;
	for (const TierTimer& t : tiers)
		out << " " << t.leftMs << " " << t.sinceTickMs;
	out << "\n";
}

void TierDrain::Load(std::istream& in, const char* tag) {
	Stop();
	const auto start = in.tellg();
	std::string word;
	if (!(in >> word) || word != tag) {
		in.clear();
		in.seekg(start);
		return;
	}
	for (TierTimer& t : tiers)
		in >> t.leftMs >> t.sinceTickMs;
}
