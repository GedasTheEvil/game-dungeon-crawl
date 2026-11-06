#include "save_slots.h"
#include "run_stats.h"
#include <algorithm>
#include <map>
#include <sstream>
#include <cstdio>
#include <filesystem>
#include <ctime>
#include <fstream>
#include <sys/stat.h>

namespace {
// The file's date and time; false if there is no file.
bool fileTime(const std::string& path, std::string& when) {
	struct stat st = {};
	if (stat(path.c_str(), &st) != 0)
		return false;
	time_t t = st.st_mtime;
	char text[40];
	std::strftime(text, sizeof(text), "%d %b %Y, %H:%M", std::localtime(&t));
	when = text;
	return true;
}
} // namespace

std::string SaveSlots::FileName(int slot) { return "saves/save" + std::to_string(slot) + ".sav"; }

SaveSlots::Info SaveSlots::DescribeFile(const std::string& path) {
	struct Read {
		std::filesystem::file_time_type time;
		std::uintmax_t size = 0;
		Info info;
	};
	static std::map<std::string, Read> cache;
	std::error_code error;
	const auto time = std::filesystem::last_write_time(path, error);
	const std::uintmax_t size = error ? 0 : std::filesystem::file_size(path, error);
	if (error) {
		cache.erase(path);
		return Info{};
	}
	if (const auto at = cache.find(path); at != cache.end() && at->second.time == time && at->second.size == size)
		return at->second.info;
	Info info;
	if (!fileTime(path, info.when))
		return info;
	info.used = true;
	std::ifstream f(path);
	std::string first;
	f >> first;
	if (first != FILE_TAG) { // a save from before the tag: the level comes first
		f.clear();
		f.seekg(0);
	}
	if (!(f >> info.level >> info.characterLevel)) {
		info.level = std::max(info.level, 0);
		info.characterLevel = 0;
	}
	// The player's stats come first: the run's line is a few lines in.
	constexpr int HEAD_LINES = 40;
	std::string line;
	for (int i = 0; i < HEAD_LINES && std::getline(f, line); i++)
		if (line.starts_with("RUN ")) {
			std::istringstream in(line);
			RunStats run;
			run.LoadDump(in);
			info.playMs = run.PlayMs();
			break;
		}
	cache[path] = {time, size, info};
	return info;
}

std::string SaveSlots::PlayTime(int ms) {
	const int minutes = ms / 60000;
	char text[24];
	if (minutes < 60)
		std::snprintf(text, sizeof(text), "%dm", minutes);
	else
		std::snprintf(text, sizeof(text), "%dh %02dm", minutes / 60, minutes % 60);
	return text;
}

std::string SaveSlots::Latest::Label() const {
	switch (kind) {
	case Kind::Slot:
		return "Slot " + std::to_string(slot + 1);
	case Kind::Quicksave:
		return "Quick Save";
	case Kind::Autosave:
		break;
	}
	return "Autosave";
}

std::optional<SaveSlots::Latest> SaveSlots::PickLatest(
	const std::string& autosave,
	const std::array<std::string, COUNT>& slots,
	const std::string& quicksave
) {
	std::optional<Latest> best;
	std::filesystem::file_time_type bestTime;
	const auto consider = [&](Latest::Kind kind, int slot, const std::string& path) {
		std::error_code error;
		const auto time = std::filesystem::last_write_time(path, error);
		if (error || (best && time < bestTime))
			return;
		best = Latest{kind, slot, path};
		bestTime = time;
	};
	consider(Latest::Kind::Autosave, 0, autosave);
	for (int slot = 0; slot < COUNT; slot++)
		consider(Latest::Kind::Slot, slot, slots[static_cast<size_t>(slot)]);
	consider(Latest::Kind::Quicksave, 0, quicksave);
	return best;
}
