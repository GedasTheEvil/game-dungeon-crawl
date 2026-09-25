#ifndef CAMPAIGN_H
#define CAMPAIGN_H

// Order of the levels in a game: the hand-made opening, generated levels getting harder, the hand-made finale.
// Level numbers start at 1 (GameState::curMap).

#include <cstdint>
#include <string>

constexpr int CAMPAIGN_OPENING = 4;	  // Levels/lvl1 .. lvl4
constexpr int CAMPAIGN_GENERATED = 6; // then generated levels
constexpr int CAMPAIGN_FIRST_DIFFICULTY = 3; // of the first generated level, +1 per level after it
// The finale is Levels/lvl<CAMPAIGN_OPENING + 1> (the ankh).
constexpr int CAMPAIGN_LEVELS = CAMPAIGN_OPENING + CAMPAIGN_GENERATED + 1;

struct CampaignLevel {
	bool generated = false;
	std::string file; // hand-made: the level file
	uint32_t seed = 0; // generated: generator seed and difficulty
	int difficulty = 0;
	std::string name; // decorations are seeded by it (the file for hand-made levels)
};

// runSeed: picked at New Game and kept in the save, so every game gets its own generated levels.
[[nodiscard]] CampaignLevel campaignLevel(int number, uint32_t runSeed);

#endif
