#include "settings_ini.h"
#include "input.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <sstream>

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
		{"display", "animation_rate", K::Int, 0, MAX_ANIMATION_RATE, nullptr, &s.display.animationRate},
		{"display", "frame_rate", K::Int, FRAME_RATE_UNLIMITED, MAX_FRAME_RATE, nullptr, &s.display.frameRate},
		{"display", "brightness", K::Int, MIN_BRIGHTNESS, MAX_BRIGHTNESS, nullptr, &s.display.brightness},
		{"graphics", "motion_effects", K::Bool, 0, 0, &s.graphics.motionEffects, nullptr},
		{"graphics", "toon", K::Bool, 0, 0, &s.graphics.toon, nullptr},
		{"graphics", "blood", K::Bool, 0, 0, &s.graphics.blood, nullptr},
		{"graphics", "light_flicker", K::Bool, 0, 0, &s.graphics.lightFlicker, nullptr},
		{"graphics", "cut_scenes", K::Bool, 0, 0, &s.graphics.cutScenes, nullptr},
		{"graphics", "camera_recenter", K::Bool, 0, 0, &s.graphics.cameraRecenter, nullptr},
		{"mouse", "sensitivity", K::Int, MIN_MOUSE_SENSITIVITY, MAX_MOUSE_SENSITIVITY, nullptr, &s.mouse.sensitivity},
		{"mouse", "invert_y", K::Bool, 0, 0, &s.mouse.invertY, nullptr},
		{"sound", "master_volume", K::Int, 0, 100, nullptr, &s.sound.master},
		{"sound", "music_volume", K::Int, 0, 100, nullptr, &s.sound.music},
		{"sound", "effects_volume", K::Int, 0, 100, nullptr, &s.sound.effects},
		{"sound", "mute", K::Bool, 0, 0, &s.sound.mute, nullptr},
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
				return name + ": " + error;
			settings.controls.Set(action, binding);
			return "";
		}
		return "unknown key '" + name + "' in [controls]";
	}
	for (const Key& key : keysOf(settings)) {
		if (section != key.section || name != key.name)
			continue;
		if (key.kind == Key::Kind::Bool && !parseBool(value, *key.boolean))
			return name + ": '" + value + "' is not on / off";
		if (key.kind == Key::Kind::Int && !parseInt(value, key.lo, key.hi, *key.integer))
			return name + ": '" + value + "' is not a number from " + std::to_string(key.lo) + " to " +
				   std::to_string(key.hi);
		return "";
	}
	return "unknown key '" + name + "' in [" + section + "]";
}
} // namespace

float brightenedChannel(float value, int percent) {
	return std::pow(std::clamp(value, 0.f, 1.f), 100.f / static_cast<float>(std::max(percent, 1)));
}

int playedVolume(int part, const Settings::Sound& sound) { return sound.mute ? 0 : part * sound.master / 100; }

MouseTurn mouseTurnPerPixel(const Settings::Mouse& mouse) {
	const float turn = MOUSE_LOOK_SENSITIVITY * static_cast<float>(mouse.sensitivity) / 100.f;
	return {turn, mouse.invertY ? -turn : turn};
}

std::vector<WindowSize> windowSizes(int desktopW, int desktopH) {
	constexpr std::array<WindowSize, 11> COMMON = {{
		{800, 500},
		{1024, 640},
		{1280, 720},
		{1366, 768},
		{1600, 900},
		{1920, 1080},
		{2560, 1440},
		{3200, 1800},
		{3840, 2160},
		{5120, 2880},
		{7680, 4320},
	}};
	const bool known = desktopW > 0 && desktopH > 0;
	std::vector<WindowSize> sizes;
	for (const WindowSize& size : COMMON)
		if (!known || (size.w <= desktopW && size.h <= desktopH))
			sizes.push_back(size);
	const WindowSize desktop = {std::min(desktopW, MAX_WINDOW_W), std::min(desktopH, MAX_WINDOW_H)};
	if (known && desktop.w >= MIN_WINDOW_W && desktop.h >= MIN_WINDOW_H &&
		std::ranges::find(sizes, desktop) == sizes.end())
		sizes.push_back(desktop);
	if (sizes.empty())
		sizes.push_back(COMMON.front());
	return sizes;
}

bool frameRedrawDue(int lastDrawMs, int nowMs, int frameRate) {
	if (frameRate <= 0)
		return true;
	return static_cast<long long>(nowMs - lastDrawMs) * frameRate >= 1000;
}

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
			if (section != "display" && section != "graphics" && section != "sound" && section != "mouse" &&
				section != "controls")
				warnings.push_back(where + "unknown section [" + section + "]");
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
		warnings.push_back(
			std::to_string(removed) + " key(s) bound to more than one action; only the first action keeps them"
		);
	return settings;
}

std::string writeSettings(const Settings& settings) {
	std::ostringstream out;
	out << "; Dungeon crawl settings. The game rewrites this file on every change: own comments and unknown keys "
		   "are lost.\n";
	out << "\n; Window size in pixels, fullscreen on/off, model poses a second (0: the key frames only), "
		   "frames a second (0: the monitor's, -1: unlimited), "
		   "brightness in percent (70-150).\n[display]\n";
	out << "window_width = " << settings.display.width << "\n";
	out << "window_height = " << settings.display.height << "\n";
	out << "fullscreen = " << onOff(settings.display.fullscreen) << "\n";
	out << "animation_rate = " << settings.display.animationRate << "\n";
	out << "frame_rate = " << settings.display.frameRate << "\n";
	out << "brightness = " << settings.display.brightness << "\n";
	out << "\n; Visual effects, each on/off.\n[graphics]\n";
	out << "motion_effects = " << onOff(settings.graphics.motionEffects) << "\n";
	out << "toon = " << onOff(settings.graphics.toon) << "\n";
	out << "blood = " << onOff(settings.graphics.blood) << "\n";
	out << "light_flicker = " << onOff(settings.graphics.lightFlicker) << "\n";
	out << "cut_scenes = " << onOff(settings.graphics.cutScenes) << "\n";
	out << "camera_recenter = " << onOff(settings.graphics.cameraRecenter) << "\n";
	out << "\n; Mouse look speed in percent (20-300), invert_y on/off.\n[mouse]\n";
	out << "sensitivity = " << settings.mouse.sensitivity << "\n";
	out << "invert_y = " << onOff(settings.mouse.invertY) << "\n";
	out << "\n; Volumes 0-100 (master scales the other two), mute on/off.\n[sound]\n";
	out << "master_volume = " << settings.sound.master << "\n";
	out << "music_volume = " << settings.sound.music << "\n";
	out << "effects_volume = " << settings.sound.effects << "\n";
	out << "mute = " << onOff(settings.sound.mute) << "\n";
	out << "\n; Up to two keys and one mouse button an action, \"none\" to clear. Names: docs/settings.md.\n"
		   "[controls]\n";
	for (int i = 0; i < BIND_ACTION_COUNT; i++) {
		auto action = static_cast<BindAction>(i);
		out << bindActionName(action) << " = " << formatBinding(settings.controls.Of(action)) << "\n";
	}
	return out.str();
}
