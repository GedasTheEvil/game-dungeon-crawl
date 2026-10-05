#include "settings_ini.h"
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <sstream>
#include <string_view>

namespace {
std::string trim(const std::string& s) {
	size_t begin = s.find_first_not_of(" \t\r");
	if (begin == std::string::npos)
		return "";
	return s.substr(begin, s.find_last_not_of(" \t\r") - begin + 1);
}

std::string lower(std::string s) {
	for (char& c : s)
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	return s;
}

bool parseBool(const std::string& value, bool& out) {
	std::string v = lower(value);
	if (v == "on" || v == "1" || v == "true" || v == "yes")
		out = true;
	else if (v == "off" || v == "0" || v == "false" || v == "no")
		out = false;
	else
		return false;
	return true;
}

bool parseInt(const std::string& value, int lo, int hi, int& out) {
	int v = 0;
	char rest = 0;
	if (std::sscanf(value.c_str(), "%d%c", &v, &rest) != 1 || v < lo || v > hi)
		return false;
	out = v;
	return true;
}

// a + b + c... without the temporaries of operator+.
std::string join(std::initializer_list<std::string_view> parts) {
	std::string out;
	for (std::string_view part : parts)
		out += part;
	return out;
}

const char* onOff(bool on) { return on ? "on" : "off"; }

// One key of a section: how it reads its value into the settings.
struct Key {
	const char* section;
	const char* name;
	enum class Kind : std::uint8_t { Bool, Int } kind;
	int lo, hi;
	bool* boolean;
	int* integer;
};

std::vector<Key> keysOf(Settings& s) {
	using K = Key::Kind;
	return {
		{"display", "window_width", K::Int, MIN_WINDOW_W, MAX_WINDOW_W, nullptr, &s.display.width},
		{"display", "window_height", K::Int, MIN_WINDOW_H, MAX_WINDOW_H, nullptr, &s.display.height},
		{"display", "fullscreen", K::Bool, 0, 0, &s.display.fullscreen, nullptr},
		{"graphics", "motion_effects", K::Bool, 0, 0, &s.graphics.motionEffects, nullptr},
		{"graphics", "toon", K::Bool, 0, 0, &s.graphics.toon, nullptr},
		{"graphics", "blood", K::Bool, 0, 0, &s.graphics.blood, nullptr},
		{"graphics", "light_flicker", K::Bool, 0, 0, &s.graphics.lightFlicker, nullptr},
		{"sound", "music_volume", K::Int, 0, 100, nullptr, &s.sound.music},
		{"sound", "effects_volume", K::Int, 0, 100, nullptr, &s.sound.effects},
	};
}

// Empty when the value was taken, else the warning.
std::string readKey(Settings& settings, const std::string& section, const std::string& name, const std::string& value) {
	if (section == "controls") {
		for (int i = 0; i < BIND_ACTION_COUNT; i++) {
			auto action = static_cast<BindAction>(i);
			if (name != bindActionName(action))
				continue;
			Binding binding = settings.controls.Of(action);
			std::string error = parseBinding(value, binding);
			if (!error.empty())
				return join({name, ": ", error});
			settings.controls.Set(action, binding);
			return "";
		}
		return join({"unknown key '", name, "' in [controls]"});
	}
	for (const Key& key : keysOf(settings)) {
		if (section != key.section || name != key.name)
			continue;
		if (key.kind == Key::Kind::Bool && !parseBool(value, *key.boolean))
			return join({name, ": '", value, "' is not on / off"});
		if (key.kind == Key::Kind::Int && !parseInt(value, key.lo, key.hi, *key.integer))
			return join({name, ": '", value, "' is not a number from ", std::to_string(key.lo), " to ",
						 std::to_string(key.hi)});
		return "";
	}
	return join({"unknown key '", name, "' in [", section, "]"});
}
} // namespace

Settings parseSettings(std::istream& in, std::vector<std::string>& warnings) {
	Settings settings;
	std::string section;
	std::string line;
	for (int number = 1; std::getline(in, line); number++) {
		line = trim(line);
		if (line.empty() || line[0] == ';' || line[0] == '#')
			continue;
		std::string where = "line " + std::to_string(number) + ": ";
		if (line[0] == '[') {
			if (line.back() != ']') {
				warnings.push_back(where + "a section needs a closing ]");
				continue;
			}
			section = lower(trim(line.substr(1, line.size() - 2)));
			if (section != "display" && section != "graphics" && section != "sound" && section != "controls")
				warnings.push_back(join({where, "unknown section [", section, "]"}));
			continue;
		}
		size_t eq = line.find('=');
		if (eq == std::string::npos) {
			warnings.push_back(where + "expected key = value");
			continue;
		}
		std::string error = readKey(settings, section, lower(trim(line.substr(0, eq))), trim(line.substr(eq + 1)));
		if (!error.empty())
			warnings.push_back(where + error);
	}
	if (int removed = settings.controls.RemoveDuplicates(); removed > 0)
		warnings.push_back(std::to_string(removed) +
						   " key(s) bound to more than one action; only the first action keeps them");
	return settings;
}

std::string writeSettings(const Settings& settings) {
	std::ostringstream out;
	out << "; Dungeon crawl settings. The game rewrites this file on every change: own comments and unknown keys "
		   "are lost.\n";
	out << "\n; Window size in pixels, fullscreen on/off.\n[display]\n";
	out << "window_width = " << settings.display.width << "\n";
	out << "window_height = " << settings.display.height << "\n";
	out << "fullscreen = " << onOff(settings.display.fullscreen) << "\n";
	out << "\n; Visual effects, each on/off.\n[graphics]\n";
	out << "motion_effects = " << onOff(settings.graphics.motionEffects) << "\n";
	out << "toon = " << onOff(settings.graphics.toon) << "\n";
	out << "blood = " << onOff(settings.graphics.blood) << "\n";
	out << "light_flicker = " << onOff(settings.graphics.lightFlicker) << "\n";
	out << "\n; Volumes 0-100.\n[sound]\n";
	out << "music_volume = " << settings.sound.music << "\n";
	out << "effects_volume = " << settings.sound.effects << "\n";
	out << "\n; Up to two keys and one mouse button an action, \"none\" to clear. Names: docs/settings.md.\n"
		   "[controls]\n";
	for (int i = 0; i < BIND_ACTION_COUNT; i++) {
		auto action = static_cast<BindAction>(i);
		out << bindActionName(action) << " = " << formatBinding(settings.controls.Of(action)) << "\n";
	}
	return out.str();
}
