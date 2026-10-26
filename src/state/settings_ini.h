#ifndef SETTINGS_INI_H
#define SETTINGS_INI_H

#include "../input/bindings.h"
#include <istream>
#include <string>
#include <vector>

// What saves/settings.ini holds (docs/settings.md): the Options choices. GL-free; src/state/settings.h reads and
// writes the file.
struct Settings {
	struct Display {
		int width = 800; // the window, when not fullscreen
		int height = 500;
		bool fullscreen = false;
		// Poses a second a model shows, blended between its key frames (plan model-frame-interpolation); 0: the key
		// frames only.
		int animationRate = 60;
	} display;
	struct Graphics {
		bool motionEffects = true; // sprint blur, FOV kick, vignette
		bool toon = false; // cel bands and ink outlines (F1)
		bool blood = true; // the blood splashes
		bool lightFlicker = true; // torches, braziers and lamps flicker
		bool cutScenes = true; // the camera shows a gate a lever opened off the screen (plan cinematic-effects)
	} graphics;
	struct Sound {
		int music = 80; // volume 0-100
		int effects = 100;
	} sound;
	Bindings controls;
};

constexpr int MIN_WINDOW_W = 320;
constexpr int MIN_WINDOW_H = 200;
constexpr int MAX_WINDOW_W = 7680;
constexpr int MAX_WINDOW_H = 4320;
constexpr int MAX_ANIMATION_RATE = 240;

// `[section]`, `key = value`, `;` and `#` comments. A bad value, an unknown key or section adds a warning and keeps
// the default.
Settings parseSettings(std::istream& in, std::vector<std::string>& warnings);
// The whole file: every key in a fixed order, a comment before each section.
std::string writeSettings(const Settings& settings);

#endif
