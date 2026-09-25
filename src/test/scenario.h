#ifndef SCENARIO_H
#define SCENARIO_H

// Scripted test runs: `./game tests/scenarios/foo.txt`.
// Command reference and output layout: docs/testing.md.
namespace Scenario {
constexpr int TICK_MS = 16;

// Parses the script. Prints errors and returns false on a parse error.
bool load(const char* path);
bool active();

int resolutionX();
int resolutionY();
bool godMode();

// One fixed game step: runs due commands, Update() and Draw(). Exits the process when the script ends.
void tick();

// Call right before swapping buffers; saves a pending screenshot from the back buffer.
void onFrameRendered();
} // namespace Scenario

#endif
