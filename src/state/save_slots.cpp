#include "save_slots.h"
#include "../core/logger.h"
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sys/stat.h>

namespace {
constexpr const char* NAME_LIST = "saves/gamelist.dat";
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
	std::snprintf(label, sizeof(label), "%02d_%02d-%02d_%02d:%02d", level, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour,
				  lt->tm_min);
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
	struct stat st = {};
	if (stat(FileName(slot).c_str(), &st) != 0)
		return info;
	info.used = true;
	if (std::sscanf(names[static_cast<size_t>(slot)].c_str(), "%d_", &info.level) != 1)
		info.level = 0;
	time_t t = st.st_mtime;
	char when[40];
	std::strftime(when, sizeof(when), "%d %b %Y, %H:%M", std::localtime(&t));
	info.when = when;
	return info;
}
