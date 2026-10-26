#ifndef LEVEL_GEN_H
#define LEVEL_GEN_H

// Random level generator in the style of the hand-made levels: one-cell-high corridors (sometimes two-cell halls)
// stacked in the 40 x 47 grid, joined by ladders and drop shafts. The main route runs from the entrance to the exit;
// side branches off it hold treasure, keys and levers. Hazards (spike pits to jump, spikes, rock falls) and monsters
// sit on the corridors, more and nastier with the difficulty.
//
// Given a zone, it builds a level in the shape of the campaign's (plan campaign-levelgen): halls across the whole
// grid, one to three rows tall, stacked in bands from top to bottom (or climbed from the bottom), joined by ladders,
// drops and long shafts; pillar loops (two ways down to the same hall), hub halls a shaft runs through with a wing
// either side, ledges in the tall halls. The ends of the halls off the route (wings) hold the keys, levers and
// treasure.
//
// Every candidate goes through checkLevel(); only levels that can be finished, with no softlock and every key and
// lever in reach, are returned. Among those, the one whose difficulty score is closest to the target wins.
// Same seed and difficulty (and zone) = same level (own random generator, no rand()).

#include "level.h"
#include "level_check.h"
#include <cstdint>
#include <string>

struct GenOptions {
	uint32_t seed = 1;
	int difficulty = 3; // 1 (easy) .. 10 (brutal)
	int zone =
		NO_ZONE; // a ZoneId (campaign.h): a level in the campaign's shape, dressed as the zone; NO_ZONE: a test level
	int openCells = 0; // a zone level's target size, 0: ZONE_OPEN_CELLS
	// The zone's boss level (ZONE_BOSS, campaign.h): the boss in a room sealed under the halls, a teleporter to it, the
	// key for the last gate behind its boss gate. Not for a zone without a boss.
	bool boss = false;
};

// A zone level's size if GenOptions::openCells does not say (the campaign's levels 15-35 have 520-760 open cells).
constexpr int ZONE_OPEN_CELLS = 650;

// The campaign level a difficulty stands for, for the decorations by depth (decor.h): 1 -> 1, 4 -> 5, 10 -> 14.
constexpr int genDecorDepth(int difficulty) { return 1 + (3 * (difficulty - 1)) / 2; }

// What a zone level's shape is made of (plan campaign-levelgen), for the tests and levelgen's report.
struct GenShape {
	int halls = 0;
	int tallHalls = 0; // two or three rows
	int loops = 0; // a second ladder between two halls: two ways to the same place round a pillar
	int hubs = 0; // halls a shaft runs straight through, a wing either side
	int ledges = 0; // a raised floor at the end of a tall hall, up a ladder
	int shafts = 0; // links through three or more rows of rock
	// The zone's terrain (ZoneTerrain, campaign.h); sand, quicksand and light shafts: the report's counts.
	int wades = 0; // runs of half water on rock
	int pools = 0; // runs of half water over deep water
	int darkHalls = 0; // no wall torch but a few
	int braziers = 0;
	int sunDiscs = 0; // each with its mirror and light shaft
	int gauntlets = 0; // trap gauntlets (ZoneTraps, campaign.h)
	// The gates (ZoneGates, campaign.h).
	int riddleGates = 0;
	int nameGates = 0; // each with its cartouche and secret room
	int hiddenRooms = 0; // behind a plain teleporter
	int treasureTombs = 0; // a wing behind a second gate of a colour
	int chains = 0; // keys and levers behind an earlier gate
	bool bossRoom = false; // GenOptions::boss
	// Treasure and supplies (ZONE_TRAPPED_CHESTS, campaign.h).
	int trappedChests = 0;
	int hapiPotions = 0; // before the first half water on the way
	int resistancePotions = 0;
};

struct GenResult {
	LevelGrid grid;
	LevelReport report;
	GenShape shape;
	int attempts = 0; // candidates built
	float target = 0.f; // difficulty score aimed at
	bool ok = false; // false only if no candidate passed the checks (should not happen)
};

constexpr int GEN_MIN_DIFFICULTY = 1;
constexpr int GEN_MAX_DIFFICULTY = 10;

[[nodiscard]] GenResult generateLevel(const GenOptions& options);

// The difficulty score generateLevel() aims at for a difficulty (on the checkLevel() scale); openCells: a zone level's
// target size, which the score grows with, 0 for a test level.
[[nodiscard]] float targetScore(int difficulty, int openCells = 0);

// "gen:SEED:DIFFICULTY" (a test level), "gen:SEED:DIFFICULTY:ZONE" (ZONE: a zone's slug, campaign.h) or
// "gen:SEED:DIFFICULTY:ZONE:boss" (the zone's boss level) -> options. False for anything else.
[[nodiscard]] bool parseGenSpec(const std::string& spec, GenOptions& options);

#endif
