#include "leaderboard.h"
#include "../core/logger.h"
#include "../entities/player_stats.h"
#include "items.h"
#include "monster_kinds.h"
#include "skills.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <sstream>

namespace {
int sumCounts(const std::vector<std::pair<std::string, int>>& counts) {
	return std::accumulate(counts.begin(), counts.end(), 0,
						   [](int sum, const std::pair<std::string, int>& c) { return sum + c.second; });
}

constexpr const char* RECORD_START = "[run]";
constexpr const char* SLAIN_KEY = "slain.";
constexpr const char* POTION_KEY = "potion.";

bool parseInt(const std::string& text, int& out) {
	std::istringstream in(text);
	int value = 0;
	if (!(in >> value) || !(in >> std::ws).eof())
		return false;
	out = value;
	return true;
}

std::vector<std::string> splitList(const std::string& text) {
	std::vector<std::string> items;
	std::istringstream in(text);
	std::string item;
	while (std::getline(in, item, ','))
		if (!item.empty())
			items.push_back(item);
	return items;
}

// One key=value of a record; false when the line is broken (no '=', a number that is none).
bool readField(RunRecord& r, const std::string& key, const std::string& value) {
	if (key == "name") {
		r.name = value;
		return true;
	}
	if (key == "date") {
		r.date = value;
		return true;
	}
	if (key == "version") {
		r.version = value;
		return true;
	}
	if (key == "skills") {
		r.skills = splitList(value);
		return true;
	}
	if (key == "level_ms") {
		r.levelMs.clear();
		for (const std::string& item : splitList(value)) {
			int ms = 0;
			if (!parseInt(item, ms))
				return false;
			r.levelMs.push_back(ms);
		}
		return true;
	}
	int number = 0;
	if (!parseInt(value, number))
		return false;
	if (key.rfind(SLAIN_KEY, 0) == 0)
		r.slain.emplace_back(key.substr(std::string(SLAIN_KEY).size()), number);
	else if (key.rfind(POTION_KEY, 0) == 0)
		r.potions.emplace_back(key.substr(std::string(POTION_KEY).size()), number);
	const std::array<std::pair<const char*, int*>, 15> ints = {{
		{"time_ms", &r.playMs},
		{"xp", &r.xp},
		{"player_level", &r.playerLevel},
		{"max_hp", &r.maxHp},
		{"stamina", &r.stamina},
		{"armor", &r.armor},
		{"might", &r.might},
		{"inscriptions", &r.inscriptions},
		{"treasures", &r.treasures},
		{"riddles", &r.riddles},
		{"damage_dealt", &r.damageDealt},
		{"damage_taken", &r.damageTaken},
		{"biggest_hit", &r.biggestHit},
		{"poisoned", &r.poisoned},
		{"cursed", &r.cursed},
	}};
	for (const auto& [name, field] : ints)
		if (key == name)
			*field = number;
	return true; // a key this build does not know: a newer statistic, kept readable
}
} // namespace

int RunRecord::SlainTotal() const { return sumCounts(slain); }

int RunRecord::PotionsTotal() const { return sumCounts(potions); }

RunRecord recordOf(const PlayerStats& stats, std::string name, std::string date, std::string version) {
	const RunStats& run = stats.run;
	RunRecord r;
	r.name = std::move(name);
	r.date = std::move(date);
	r.version = std::move(version);
	r.playMs = run.PlayMs();
	r.xp = static_cast<int>(stats.CurrentXP());
	r.playerLevel = stats.CurrentLevel();
	r.maxHp = stats.CurrentMaxHP();
	r.stamina = stats.MaxStamina();
	r.armor = stats.CurrentArmor();
	r.might = stats.CurrentMight();
	for (int i = 0; i < SKILL_COUNT; i++)
		if (stats.heka.Has(static_cast<SkillId>(i)))
			r.skills.emplace_back(SKILLS[static_cast<size_t>(i)].name);
	for (int type = 1; type <= MONSTER_TYPE_MAX; type++)
		if (const MonsterKind* kind = monsterKind(type); kind != nullptr && run.Slain(type) > 0)
			r.slain.emplace_back(kind->name, run.Slain(type));
	for (int i = 0; i < ITEM_KIND_COUNT; i++)
		if (isPotion(itemAt(i)) && run.Drunk(itemAt(i)) > 0)
			r.potions.emplace_back(itemText(itemAt(i)).name, run.Drunk(itemAt(i)));
	r.levelMs = run.LevelMs();
	r.inscriptions = run.inscriptions;
	r.treasures = run.treasures;
	r.riddles = run.riddles;
	r.damageDealt = run.damageDealt;
	r.damageTaken = run.damageTaken;
	r.biggestHit = run.biggestHit;
	r.poisoned = run.poisoned;
	r.cursed = run.cursed;
	return r;
}

int explorationScore(const RunRecord& r) {
	return r.inscriptions * EXPLORE_INSCRIPTION_POINTS + r.treasures * EXPLORE_TREASURE_POINTS +
		   r.riddles * EXPLORE_RIDDLE_POINTS;
}

std::string formatPlayTime(int ms) {
	const int seconds = std::max(0, ms) / 1000;
	char text[24];
	std::snprintf(text, sizeof(text), "%d:%02d:%02d", seconds / 3600, seconds / 60 % 60, seconds % 60);
	return text;
}

std::string scoreText(const RunRecord& r, RecordOrder order) {
	if (r.placeholder)
		return "-";
	switch (order) {
	case RecordOrder::Time:
		return formatPlayTime(r.playMs);
	case RecordOrder::Xp:
		return std::to_string(r.xp);
	case RecordOrder::Explored:
		return std::to_string(explorationScore(r));
	}
	return "";
}

std::vector<int> rankRecords(const std::vector<RunRecord>& records, RecordOrder order) {
	std::vector<int> ranked(records.size());
	std::iota(ranked.begin(), ranked.end(), 0);
	auto value = [&](const RunRecord& r) {
		return order == RecordOrder::Xp ? r.xp : order == RecordOrder::Explored ? explorationScore(r) : 0;
	};
	std::stable_sort(ranked.begin(), ranked.end(), [&](int a, int b) {
		const RunRecord& ra = records[static_cast<size_t>(a)];
		const RunRecord& rb = records[static_cast<size_t>(b)];
		if (value(ra) != value(rb))
			return value(ra) > value(rb);
		return ra.playMs < rb.playMs;
	});
	return ranked;
}

RunRecord placeholderRecord() {
	RunRecord r;
	r.name = "Spirit of Exploration";
	r.playerLevel = 1;
	r.placeholder = true;
	return r;
}

std::vector<RunRecord> readRecords(std::istream& in, std::vector<std::string>& warnings) {
	std::vector<RunRecord> records;
	std::vector<RunRecord> read;
	std::string line;
	int lineNo = 0;
	bool inRecord = false;
	while (std::getline(in, line)) {
		lineNo++;
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (line.empty())
			continue;
		if (line == RECORD_START) {
			read.emplace_back();
			inRecord = true;
			continue;
		}
		const size_t eq = line.find('=');
		if (!inRecord || eq == std::string::npos || !readField(read.back(), line.substr(0, eq), line.substr(eq + 1)))
			warnings.push_back("line " + std::to_string(lineNo) + " skipped: " + line);
	}
	for (RunRecord& r : read)
		if (r.name.empty())
			warnings.emplace_back("a record without a name skipped");
		else
			records.push_back(std::move(r));
	return records;
}

void writeRecord(std::ostream& out, const RunRecord& r) {
	auto list = [](const auto& items, auto item) {
		std::string text;
		for (const auto& i : items)
			text += (text.empty() ? "" : ",") + item(i);
		return text;
	};
	out << RECORD_START << '\n'
		<< "name=" << r.name << '\n'
		<< "date=" << r.date << '\n'
		<< "version=" << r.version << '\n'
		<< "time_ms=" << r.playMs << '\n'
		<< "xp=" << r.xp << '\n'
		<< "player_level=" << r.playerLevel << '\n'
		<< "max_hp=" << r.maxHp << '\n'
		<< "stamina=" << r.stamina << '\n'
		<< "armor=" << r.armor << '\n'
		<< "might=" << r.might << '\n'
		<< "skills=" << list(r.skills, [](const std::string& s) { return s; }) << '\n'
		<< "inscriptions=" << r.inscriptions << '\n'
		<< "treasures=" << r.treasures << '\n'
		<< "riddles=" << r.riddles << '\n'
		<< "damage_dealt=" << r.damageDealt << '\n'
		<< "damage_taken=" << r.damageTaken << '\n'
		<< "biggest_hit=" << r.biggestHit << '\n'
		<< "poisoned=" << r.poisoned << '\n'
		<< "cursed=" << r.cursed << '\n'
		<< "level_ms=" << list(r.levelMs, [](int ms) { return std::to_string(ms); }) << '\n';
	for (const auto& [kind, count] : r.slain)
		out << SLAIN_KEY << kind << '=' << count << '\n';
	for (const auto& [kind, count] : r.potions)
		out << POTION_KEY << kind << '=' << count << '\n';
	out << '\n';
}

void Leaderboard::Load() {
	records.clear();
	std::ifstream in(path);
	if (!in)
		return;
	std::vector<std::string> warnings;
	records = readRecords(in, warnings);
	for (const std::string& warning : warnings)
		LOG_WARNING("leaderboard", path + ": " + warning);
}

bool Leaderboard::Add(const RunRecord& r) {
	std::error_code ec; // a play run's output directory may not exist yet
	if (const std::filesystem::path dir = std::filesystem::path(path).parent_path(); !dir.empty())
		std::filesystem::create_directories(dir, ec);
	std::ofstream out(path, std::ios::app);
	if (!out) {
		LOG_ERROR("leaderboard", "cannot write " + path);
		return false;
	}
	writeRecord(out, r);
	records.push_back(r);
	return static_cast<bool>(out);
}

std::vector<RunRecord> Leaderboard::Shown() const {
	return records.empty() ? std::vector<RunRecord>{placeholderRecord()} : records;
}

std::string Leaderboard::LastName() const { return records.empty() ? "" : records.back().name; }
