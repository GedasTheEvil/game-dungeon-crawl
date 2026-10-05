#ifndef SETTINGS_H
#define SETTINGS_H

struct RenderSettings;

// The player's choices from Options, kept between runs in saves/settings.txt. Scenarios neither read nor write it:
// they run with the defaults.
namespace Settings {
void Load(RenderSettings& render); // a missing or broken file keeps the defaults
void Save(const RenderSettings& render);
} // namespace Settings

#endif
