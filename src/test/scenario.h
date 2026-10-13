#ifndef SCENARIO_H
#define SCENARIO_H

// Scripted test runs: `./game tests/scenarios/foo.txt`.
// Command reference and output layout: docs/testing.md.
#include "../core/gameplay_config.h"
#include <string>

namespace Scenario {
constexpr int TICK_MS = UPDATE_TICK_MS;

// Parses the script. Prints errors and returns false on a parse error.
bool load(const char* path);
bool active();

int resolutionX();
int resolutionY();
// Real time between ticks: 0 (as fast as possible, the game clock is virtual), TICK_MS with SCENARIO_DRAW_ALL.
int tickDelayMs();

// One fixed game step: runs due commands, Update() and Draw(). Exits the process when the script ends.
void tick();

// Play mode: a normal hand-played game (real clock, sound, settings) that starts on a script's setup.
// `--play SCRIPT`: its setup commands (see playSetup in scenario_script.h). `--level ARG`: a level, as the `level`
// command takes it. Both print the error and return false on a bad script. Not active(): the script is no run.
bool loadPlay(const char* path);
bool loadPlayLevel(const char* levelArg);
// Call after every Update(): runs the setup once, in the first frame after the assets are loaded.
void runPlaySetup();

// Where a scenario or a play run writes its files (tests/out/<name>); empty for the plain game.
std::string outputDir();

// Call right before swapping buffers; saves a pending screenshot from the back buffer.
void onFrameRendered();
} // namespace Scenario

#endif
