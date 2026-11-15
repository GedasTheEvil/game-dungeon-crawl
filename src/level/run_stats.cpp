#include "run_stats.h"
#include <algorithm>
#include <functional>
#include <string>
#include <utility>

void RunStats::Slay(int monsterType) {
	if (monsterType >= 0 && monsterType <= MONSTER_TYPE_MAX)
		slain[static_cast<size_t>(monsterType)]++;
}

void RunStats::Drink(ItemKind potion) { drunk[std::to_underlying(potion)]++; }

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

int RunStats::SlainTotal() const { return std::ranges::fold_left(slain, 0, std::plus{}); }

int RunStats::DrunkTotal() const { return std::ranges::fold_left(drunk, 0, std::plus{}); }

int RunStats::PlayMs() const { return std::ranges::fold_left(levelMs, 0, std::plus{}); }

namespace {
// " n index count ..." of the non-zero counts.
template <size_t N> void dumpCounts(std::ostream& out, const std::array<int, N>& counts) {
	out << ' ' << std::count_if(counts.begin(), counts.end(), [](int n) { return n != 0; });
	for (size_t i = 0; i < N; i++)
		if (counts[i] != 0)
			out << ' ' << i << ' ' << counts[i];
}

// " n type id count ..." of the potions drunk, by file id (RUN2).
void dumpDrunk(std::ostream& out, const std::array<int, ITEM_KIND_COUNT>& drunk) {
	out << ' ' << std::count_if(drunk.begin(), drunk.end(), [](int n) { return n != 0; });
	for (int i = 0; i < ITEM_KIND_COUNT; i++)
		if (drunk[static_cast<size_t>(i)] != 0) {
			const ItemFileId file = fileIdOf(itemAt(i));
			out << ' ' << file.type << ' ' << file.id << ' ' << drunk[static_cast<size_t>(i)];
		}
}

bool loadDrunk(std::istream& in, std::array<int, ITEM_KIND_COUNT>& drunk) {
	int n = 0;
	if (!(in >> n))
		return false;
	for (int k = 0; k < n; k++) {
		int type = 0;
		int id = 0;
		int count = 0;
		if (!(in >> type >> id >> count))
			return false;
		if (const std::optional<ItemKind> kind = itemFromFile(type, id)) // else: an item of a newer build
			drunk[static_cast<size_t>(itemIndex(*kind))] = count;
	}
	return true;
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

// "RUN2 campaign inscriptions treasures riddles dealt taken biggest poisoned cursed  slain...  drunk...  n levelMs..."
// (slain: n index count..., drunk: n type id count... by file id; the old "RUN" has drunk as n index count... by
// ItemKind position, still read)
void RunStats::Dump(std::ostream& out) const {
	out << "RUN2 " << (campaign ? 1 : 0) << ' ' << inscriptions << ' ' << treasures << ' ' << riddles << ' '
		<< damageDealt << ' ' << damageTaken << ' ' << biggestHit << ' ' << poisoned << ' ' << cursed;
	dumpCounts(out, slain);
	dumpDrunk(out, drunk);
	out << ' ' << levelMs.size();
	for (int ms : levelMs)
		out << ' ' << ms;
	out << "\nNAMES " << nameSeed << '\n';
	out << "DEATHS " << deaths << '\n';
	out << "SECRETS " << secrets << '\n';
	out << "HINTS " << hints << '\n';
	out << "OFFERINGS " << offerings << '\n';
}

void RunStats::LoadDump(std::istream& in) {
	*this = RunStats{};
	const auto start = in.tellg();
	std::string tag;
	if (!(in >> tag) || (tag != "RUN" && tag != "RUN2")) {
		in.clear();
		in.seekg(start);
		return;
	}
	int isCampaign = 0;
	size_t levels = 0;
	RunStats loaded;
	bool ok = static_cast<bool>(
				  in >> isCampaign >> loaded.inscriptions >> loaded.treasures >> loaded.riddles >> loaded.damageDealt >>
				  loaded.damageTaken >> loaded.biggestHit >> loaded.poisoned >> loaded.cursed
			  ) &&
			  loadCounts(in, loaded.slain) &&
			  (tag == "RUN2" ? loadDrunk(in, loaded.drunk) : loadCounts(in, loaded.drunk)) &&
			  static_cast<bool>(in >> levels);
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
	// From the secret name gates on (plan secret-name-gates).
	const auto names = in.tellg();
	if (!(in >> tag) || tag != "NAMES" || !(in >> nameSeed)) {
		in.clear();
		in.seekg(names);
		nameSeed = 0;
		return;
	}
	// From the autosave on (plan autosave).
	const auto deathsAt = in.tellg();
	if (!(in >> tag) || tag != "DEATHS" || !(in >> deaths)) {
		in.clear();
		in.seekg(deathsAt);
		deaths = 0;
		return;
	}
	// From the leaderboard's secrets on (plan leaderboard-secrets).
	const auto secretsAt = in.tellg();
	if (!(in >> tag) || tag != "SECRETS" || !(in >> secrets)) {
		in.clear();
		in.seekg(secretsAt);
		secrets = 0;
		return;
	}
	// From the first hints on (plan controls-help).
	const auto hintsAt = in.tellg();
	if (!(in >> tag) || tag != "HINTS" || !(in >> hints)) {
		in.clear();
		in.seekg(hintsAt);
		hints = 0;
		return;
	}
	// From the offering gifts on (plan offering-gift).
	const auto offeringsAt = in.tellg();
	if (!(in >> tag) || tag != "OFFERINGS" || !(in >> offerings)) {
		in.clear();
		in.seekg(offeringsAt);
		offerings = 0;
	}
}

bool RunStats::SetDeaths(std::string& save, int count) {
	const std::string line = "DEATHS " + std::to_string(count) + '\n';
	size_t at = save.find("\nDEATHS ");
	if (at != std::string::npos) {
		const size_t end = save.find('\n', at + 1);
		save.replace(at + 1, end == std::string::npos ? std::string::npos : end - at, line);
		return true;
	}
	at = save.find("\nNAMES ");
	if (at == std::string::npos)
		return false;
	const size_t end = save.find('\n', at + 1);
	if (end == std::string::npos)
		save += '\n' + line;
	else
		save.insert(end + 1, line);
	return true;
}
