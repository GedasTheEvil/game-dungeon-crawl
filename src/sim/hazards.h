// What lies or hangs on the level for a while (plan dungeon-subsystems): webs, burning floors, chest gas, the holes,
// sprays and dust of burrowers and sand, landing puffs, snuffed flames. Not saved: a load clears them. Their effect on
// the player and the monsters is the Dungeon's (dungeon_webs.cpp, dungeon_blaze.cpp, dungeon_chest_traps.cpp,
// dungeon_burrow.cpp, dungeon_base.cpp); the draw only reads them (plan render-reads-sim).
#ifndef HAZARDS_H
#define HAZARDS_H
#include "mechanisms.h"
#include <vector>

struct Hazards {
	// A puff of poison gas over a chest (ChestTrap::Gas), for CHEST_GAS_MS.
	struct ChestGas {
		int cell; // map index
		int startMs; // GameClock
		int nextPoisonMs; // the next weak poison for a player in it
	};
	// A web over the floor of cell (map index), spun at laidMs; cut or burnt at goneMs (< 0: whole), gone WEB_CUT_MS
	// or WEB_BURN_MS later. A load starts the boss fight over.
	struct Web {
		int cell;
		int laidMs;
		int goneMs = -1;
		bool burnt = false;
	};
	// A floor burning where Babi's big coal landed (plan babi-boss, dungeon_blaze.cpp): the cell (map index) from
	// startMs to endMs. Whoever stands in it, not in a jump, takes BLAZE_DAMAGE every BLAZE_TICK_MS and burns on.
	struct Blaze {
		int cell;
		int startMs;
		int endMs;
	};
	// Where something came up out of or went down into the floor, or kicked it up, from startMs.
	struct Hole {
		float x; // map x
		int row;
		int startMs;
	};
	// A fall onto a landing heap or sand (plan soft-landing): a puff of its matter (heap: DECOR_HEAP_*).
	struct Landing {
		float x; // map x
		int row;
		int startMs;
		int heap;
	};

	std::vector<ChestGas> chestGas;
	std::vector<Web> webs;
	std::vector<CellMotion> snuffs; // the torches and lamps just put out, their flames dying down (SNUFF_MS)
	std::vector<Blaze> blazes;
	std::vector<Hole> holes; // a burrower went down or came up (Monster::Dive): sand thrown up, for BURROW_HOLE_MS
	// A sand burrower's spray of sand as it came up (RoarRules spray, plan worm-boss), for BURROW_SPRAY_MS: drawn
	// with the holes.
	std::vector<Hole> sprays;
	std::vector<Hole> puffs; // dust kicked up by a step in sand (plan desert-chapter), for SAND_PUFF_MS
	std::vector<Hole> pours; // sand falls pouring down, from the warning's end, for SAND_POUR_MS + SAND_POUR_TAIL_MS
	std::vector<Landing> landings; // for LANDING_PUFF_MS

	void Clear(); // a level or save was loaded
	// Drops the holes, sprays, puffs, landings and pours that ran out by now.
	void ExpireMarks(int now);
	[[nodiscard]] int WebAt(int cell) const; // the whole web over the floor of cell (map index): its index, -1 none
	[[nodiscard]] bool Blazing(int cell) const; // the floor of cell (map index) burns
};

#endif
