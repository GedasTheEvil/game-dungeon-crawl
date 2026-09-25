#ifndef LEVEL_GEN_H
#define LEVEL_GEN_H

// Random level generator in the style of the hand-made levels: one-cell-high corridors (sometimes two-cell halls)
// stacked in the 40 x 47 grid, joined by ladders and drop shafts. The main route runs from the entrance to the exit;
// side branches off it hold treasure, keys and levers. Hazards (spike pits to jump, spikes, rock falls) and monsters
// sit on the corridors, more and nastier with the difficulty.
//
// Every candidate goes through checkLevel(); only levels that can be finished, with no softlock and every key and
// lever in reach, are returned. Among those, the one whose difficulty score is closest to the target wins.
// Same seed and difficulty = same level (own random generator, no rand()).

#include "level.h"
#include "level_check.h"
#include <cstdint>

struct GenOptions {
	uint32_t seed = 1;
	int difficulty = 3; // 1 (easy) .. 10 (brutal)
};

struct GenResult {
	LevelGrid grid;
	LevelReport report;
	int attempts = 0;	// candidates built
	float target = 0.f; // difficulty score aimed at
	bool ok = false;	// false only if no candidate passed the checks (should not happen)
};

constexpr int GEN_MIN_DIFFICULTY = 1;
constexpr int GEN_MAX_DIFFICULTY = 10;

[[nodiscard]] GenResult generateLevel(const GenOptions& options);

// The difficulty score generateLevel() aims at for a difficulty (on the checkLevel() scale).
[[nodiscard]] float targetScore(int difficulty);

#endif
