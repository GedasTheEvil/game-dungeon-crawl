#ifndef SETTINGS_H
#define SETTINGS_H

#include "settings_ini.h"

// The player's choices from Options, kept between runs in saves/settings.ini (docs/settings.md). Scenarios neither
// read nor write it: they run with the defaults.
namespace SettingsFile {
// A missing file keeps the defaults; a bad line logs a warning. An old saves/settings.txt is read once and replaced.
void Load(Settings& settings);
// Rewrites the file when the settings differ from what it holds.
void Save(const Settings& settings);
} // namespace SettingsFile

#endif
