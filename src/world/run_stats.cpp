#include "run_stats.h"
#include <algorithm>
#include <numeric>
#include <string>

void RunStats::Slay(int monsterType) {
	if (monsterType >= 0 && monsterType <= MONSTER_TYPE_MAX)
		slain[static_cast<size_t>(monsterType)]++;
}

void RunStats::Drink(ItemKind potion) { drunk[static_cast<size_t>(potion)]++; }

void RunStats::Hit(int dealt) {
	damageDealt += dealt;
	biggestHit = std::max(biggestHit, dealt);
}

void RunStats::AddPlayMs(int ms, int level) {
	if (level < 1)
		return;
	if (levelMs.size() < static_cast<size_t>(level))
		levelMs.resize(static_cast<size_t>(level), 0);
	levelMs[static_cast<size_t>(level - 1)] += ms;
}

int RunStats::SlainTotal() const { return std::accumulate(slain.begin(), slain.end(), 0); }

int RunStats::DrunkTotal() const { return std::accumulate(drunk.begin(), drunk.end(), 0); }

int RunStats::PlayMs() const { return std::accumulate(levelMs.begin(), levelMs.end(), 0); }

namespace {
// " n index count ..." of the non-zero counts.
template <size_t N> void dumpCounts(std::ostream& out, const std::array<int, N>& counts) {
	out << ' ' << std::count_if(counts.begin(), counts.end(), [](int n) { return n != 0; });
	for (size_t i = 0; i < N; i++)
		if (counts[i] != 0)
			out << ' ' << i << ' ' << counts[i];
}

template <size_t N> bool loadCounts(std::istream& in, std::array<int, N>& counts) {
	int n = 0;
	if (!(in >> n))
		return false;
	for (int k = 0; k < n; k++) {
		size_t i = 0;
		int count = 0;
		if (!(in >> i >> count))
			return false;
		if (i < N)
			counts[i] = count;
	}
	return true;
}
} // namespace

// "RUN campaign inscriptions treasures riddles dealt taken biggest  slain...  drunk...  n levelMs..."
void RunStats::Dump(std::ostream& out) const {
	out << "RUN " << (campaign ? 1 : 0) << ' ' << inscriptions << ' ' << treasures << ' ' << riddles << ' '
		<< damageDealt << ' ' << damageTaken << ' ' << biggestHit;
	dumpCounts(out, slain);
	dumpCounts(out, drunk);
	out << ' ' << levelMs.size();
	for (int ms : levelMs)
		out << ' ' << ms;
	out << '\n';
}

void RunStats::LoadDump(std::istream& in) {
	*this = RunStats{};
	const auto start = in.tellg();
	std::string tag;
	if (!(in >> tag) || tag != "RUN") {
		in.clear();
		in.seekg(start);
		return;
	}
	int isCampaign = 0;
	size_t levels = 0;
	RunStats loaded;
	bool ok = static_cast<bool>(in >> isCampaign >> loaded.inscriptions >> loaded.treasures >> loaded.riddles >>
								loaded.damageDealt >> loaded.damageTaken >> loaded.biggestHit) &&
			  loadCounts(in, loaded.slain) && loadCounts(in, loaded.drunk) && static_cast<bool>(in >> levels);
	for (size_t i = 0; ok && i < levels; i++) {
		int ms = 0;
		ok = static_cast<bool>(in >> ms);
		loaded.levelMs.push_back(ms);
	}
	if (!ok) {
		in.clear();
		return;
	}
	loaded.campaign = isCampaign != 0;
	*this = loaded;
}
