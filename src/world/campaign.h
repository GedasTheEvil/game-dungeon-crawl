#ifndef CAMPAIGN_H
#define CAMPAIGN_H

// Order of the levels in a game: levels/lvl1 .. lvl<CAMPAIGN_LEVELS>. Each exit loads the next level,
// the last one holds the ankh that wins the game. Level numbers start at 1 (Dungeon::LevelNumber).

#include <string>

constexpr int CAMPAIGN_LEVELS = 30;

// The level file, also the seed of its decorations.
[[nodiscard]] std::string campaignLevelFile(int number);

#endif
