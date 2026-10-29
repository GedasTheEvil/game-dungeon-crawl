#include "save_slots.h"
#include "../core/logger.h"
#include <cstdio>
#include <filesystem>
#include <ctime>
#include <fstream>
#include <sys/stat.h>

namespace {
constexpr const char* NAME_LIST = "saves/gamelist.dat";

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

void SaveSlots::LoadNames() {
	std::ifstream f(NAME_LIST);
	if (!f) {
		LOG_WARNING("game", "Failed loading save list");
		return;
	}
	for (std::string& name : names) // line by line: an empty line is an unnamed slot
		if (!std::getline(f, name))
			break;
}

void SaveSlots::Record(int slot, int level) {
	time_t t = time(nullptr);
	const struct tm* lt = localtime(&t);
	char label[32];
	std::snprintf(
		label,
		sizeof(label),
		"%02d_%02d-%02d_%02d:%02d",
		level,
		lt->tm_mon + 1,
		lt->tm_mday,
		lt->tm_hour,
		lt->tm_min
	);
	names[static_cast<size_t>(slot)] = label;
	writeNames();
}

void SaveSlots::writeNames() const {
	std::ofstream f(NAME_LIST);
	for (const std::string& name : names)
		f << name << "\n";
}

// The name gives the level; the file time gives the full date.
SaveSlots::Info SaveSlots::Describe(int slot) const {
	Info info;
	if (!fileTime(FileName(slot), info.when))
		return info;
	info.used = true;
	if (std::sscanf(names[static_cast<size_t>(slot)].c_str(), "%d_", &info.level) != 1)
		info.level = 0;
	return info;
}

SaveSlots::Info SaveSlots::DescribeFile(const std::string& path) {
	Info info;
	if (!fileTime(path, info.when))
		return info;
	info.used = true;
	std::ifstream f(path);
	if (!(f >> info.level))
		info.level = 0;
	return info;
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
