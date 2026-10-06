#include "poison.h"
#include <istream>
#include <string>
#include <ostream>

void Poison::Apply(PoisonTier tier) {
	TierTimer& t = tiers[static_cast<size_t>(tier)];
	if (t.leftMs <= 0)
		t.sinceTickMs = 0;
	t.leftMs = POISON_TIERS[static_cast<size_t>(tier)].durationMs;
}

int Poison::Advance(int ms) {
	int hp = 0;
	for (size_t i = 0; i < tiers.size(); i++) {
		TierTimer& t = tiers[i];
		if (t.leftMs <= 0)
			continue;
		const int spent = ms < t.leftMs ? ms : t.leftMs;
		t.leftMs -= spent;
		t.sinceTickMs += spent;
		while (t.sinceTickMs >= 1000) {
			t.sinceTickMs -= 1000;
			hp += POISON_TIERS[i].hpPerSecond;
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
