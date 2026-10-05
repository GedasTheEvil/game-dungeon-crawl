#include "settings.h"
#include "../core/logger.h"
#include "../test/scenario.h"
#include <cstdio>
#include <fstream>
#include <string>

namespace {
constexpr const char* PATH = "saves/settings.ini";
constexpr const char* OLD_PATH = "saves/settings.txt"; // "name value" lines, before the ini

std::string gWritten; // the file's text as last read or written, so an unchanged Save does not write

// The old file only had the motion effects switch.
bool loadOld(Settings& settings) {
	std::ifstream f(OLD_PATH);
	if (!f)
		return false;
	std::string name;
	int value = 0;
	while (f >> name >> value)
		if (name == "motion_effects")
			settings.graphics.motionEffects = value != 0;
	return true;
}
} // namespace

void SettingsFile::Load(Settings& settings) {
	if (Scenario::active())
		return;
	std::ifstream f(PATH);
	if (!f) {
		gWritten = writeSettings(settings);
		if (loadOld(settings)) {
			LOG_INFOF("game", "Moving %s to %s", OLD_PATH, PATH);
			Save(settings);
			if (std::remove(OLD_PATH) != 0)
				LOG_WARNINGF("game", "Cannot remove %s", OLD_PATH);
		}
		return;
	}
	std::vector<std::string> warnings;
	settings = parseSettings(f, warnings);
	for (const std::string& warning : warnings)
		LOG_WARNINGF("game", "%s: %s", PATH, warning.c_str());
	// The text the file would have: hand-written comments or a bad value rewrite it on the first change only.
	gWritten = writeSettings(settings);
}

void SettingsFile::Save(const Settings& settings) {
	if (Scenario::active())
		return;
	std::string text = writeSettings(settings);
	if (text == gWritten)
		return;
	std::ofstream f(PATH);
	f << text;
	if (!f) {
		LOG_WARNINGF("game", "Cannot write %s", PATH);
		return;
	}
	gWritten = text;
}
