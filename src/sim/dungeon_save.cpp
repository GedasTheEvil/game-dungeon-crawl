#include "dungeon_save.h"
#include "../core/logger.h"
#include "../level/items.h"
#include "../level/save_slots.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <system_error>

namespace {
// The tagged lines of plan save-remembers-ui-state: a save from before them has none, the defaults stay.
void writeSettings(std::ostream& out, const SaveParts& parts) {
	const UiViewState& view = parts.view;
	out << "\nSTAMINASKILLS " << parts.player.staminaSkills << "\nFIREARROWS " << parts.player.fireArrows << "\nFACING "
		<< view.facing << "\nINVENTORY " << static_cast<int>(view.tab);
	for (const SortOrder s : view.sort)
		out << ' ' << static_cast<int>(s);
	out << "\nJOURNAL " << view.journalSection;
	for (const int s : view.spread)
		out << ' ' << s;
	out << '\n';
}

// True and past the tag if the next token is `tag`; else the stream is put back.
bool takeTag(std::istream& in, const char* tag) {
	const auto start = in.tellg();
	std::string token;
	if (in >> token && token == tag)
		return true;
	in.clear();
	in.seekg(start);
	return false;
}

void readSettings(std::istream& in, const SaveParts& parts) {
	UiViewState& view = parts.view;
	view = UiViewState{};
	if (takeTag(in, "STAMINASKILLS")) {
		int on = 0;
		in >> on;
		parts.player.staminaSkills = on != 0;
	}
	if (takeTag(in, "FIREARROWS")) {
		int shots = 0;
		in >> shots;
		parts.player.fireArrows = std::max(0, shots);
	}
	if (takeTag(in, "FACING")) {
		int facing = 1;
		in >> facing;
		view.facing = facing < 0 ? -1 : 1;
	}
	if (takeTag(in, "INVENTORY")) {
		int tab = 0;
		in >> tab;
		view.tab = static_cast<ItemGroup>(std::clamp(tab, 0, ITEM_GROUP_COUNT - 1));
		for (SortOrder& s : view.sort) {
			int order = 0;
			in >> order;
			s = static_cast<SortOrder>(std::clamp(order, 0, SORT_ORDER_COUNT - 1));
		}
	}
	if (takeTag(in, "JOURNAL")) {
		int section = UiViewState::JOURNAL_CREATURES;
		in >> section;
		view.journalSection = std::clamp(section, 0, UiViewState::JOURNAL_SECTIONS - 1);
		for (int& s : view.spread) {
			in >> s;
			s = std::max(0, s); // past a section's end: the screen clamps it
		}
	}
	if (!in) { // cut inside a line: the rest are defaults
		in.clear();
		view = UiViewState{};
	}
}
} // namespace

void writeSaveGame(std::ostream& out, const SaveParts& parts) {
	out << SaveSlots::FILE_TAG << ' ' << parts.dungeon.LevelNumber() << " ";
	parts.player.stats.Dump(out);
	parts.bag.Save(out);
	parts.dungeon.Dump(out);
	writeSettings(out, parts);
	parts.journal.Dump(out);
	out << '\n' << SaveSlots::END_TAG << '\n';
}

bool readSaveGame(std::istream& in, const SaveParts& parts) {
	std::string first;
	if (!(in >> first))
		return false;
	const bool tagged = first == SaveSlots::FILE_TAG;
	int levelNumber = 0;
	try {
		levelNumber = tagged ? -1 : std::stoi(first);
	} catch (const std::exception&) {
		return false;
	}
	if (tagged && !(in >> levelNumber))
		return false;
	parts.player.Reanimate();
	parts.dungeon.SetLevelNumber(levelNumber);
	PlayerStats& stats = parts.player.stats;
	stats.LoadDump(in);
	parts.bag.Load(in);
	stats.Wear(amuletBonus(parts.bag.Worn()), false); // the saved HP is already the worn one's
	stats.WearRings(ringBonus(parts.bag.Rings()));
	if (!in || !parts.dungeon.LoadDump(in))
		return false;
	readSettings(in, parts);
	parts.journal.Load(in);
	in.clear(); // an empty journal leaves the stream failed
	std::string end;
	return !tagged || (in >> end && end == SaveSlots::END_TAG);
}

bool loadSaveGame(const std::string& text, const SaveParts& parts) {
	std::ostringstream before;
	writeSaveGame(before, parts);
	std::istringstream in(text);
	if (readSaveGame(in, parts))
		return true;
	std::istringstream back(before.str());
	if (!readSaveGame(back, parts))
		LOG_ERROR("world", "The game before the broken save cannot be put back");
	return false;
}

bool writeFileSafely(const std::string& path, const std::string& text) {
	const std::string temp = path + ".tmp";
	{
		std::ofstream out(temp, std::ios::binary | std::ios::trunc);
		if (!(out << text) || !out.flush())
			return false;
	}
	std::error_code error;
	std::filesystem::rename(temp, path, error);
	if (error) {
		std::filesystem::remove(temp, error);
		return false;
	}
	return true;
}

bool readWholeFile(const std::string& path, std::string& text) {
	std::ifstream in(path, std::ios::binary);
	if (!in)
		return false;
	std::ostringstream all;
	all << in.rdbuf();
	text = all.str();
	return true;
}
