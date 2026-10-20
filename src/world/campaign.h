#ifndef CAMPAIGN_H
#define CAMPAIGN_H

// Order of the levels in a game: content/levels/lvl1 .. lvl<CAMPAIGN_LEVELS>. Each exit loads the next level,
// the last one holds the ankh that wins the game. Level numbers start at 1 (Dungeon::LevelNumber).

#include <string>

constexpr int CAMPAIGN_LEVELS = 35;
// The desert chapter (plan desert-chapter): Seth's sands, sandstone instead of the tombs' limestone.
constexpr int DESERT_FIRST = 21;
constexpr int DESERT_LAST = 25;
[[nodiscard]] constexpr bool desertLevel(int number) { return number >= DESERT_FIRST && number <= DESERT_LAST; }

// The level file, also the seed of its decorations.
[[nodiscard]] std::string campaignLevelFile(int number);

#endif
