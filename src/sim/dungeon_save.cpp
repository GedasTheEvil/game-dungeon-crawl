#include "dungeon_save.h"
#include "../core/logger.h"
#include "../level/items.h"
#include "../level/save_slots.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <system_error>

void writeSaveGame(std::ostream& out, const SaveParts& parts) {
	out << SaveSlots::FILE_TAG << ' ' << parts.dungeon.LevelNumber() << " ";
	parts.player.stats.Dump(out);
	parts.bag.Save(out);
	parts.dungeon.Dump(out);
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
