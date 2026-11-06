// What the checker's files share (plan split-large-sources): the walker, the position text, the checks that live
// in level_check_dressing.cpp and level_check_mechanisms.cpp. Internal to the checker.
#ifndef LEVEL_CHECK_INTERNAL_H
#define LEVEL_CHECK_INTERNAL_H

#include "level_walker.h"
#include <string>

namespace levelcheck {
bool isGoal(const Tile& t);
std::string at(int cell);

// level_check_dressing.cpp
void checkDressing(const LevelGrid& grid, LevelReport& r);

// level_check_mechanisms.cpp
void checkLocks(const LevelGrid& grid, LevelReport& r);
void checkDeadGates(const LevelGrid& grid, LevelReport& r);
void checkTeleporters(const LevelGrid& grid, LevelReport& r);
void checkNameGates(const LevelGrid& grid, int startState, LevelReport& r);
int checkSunBeams(const LevelGrid& grid, LevelReport& r);
void checkWallSpikes(const LevelGrid& grid, LevelReport& r);
void checkSandGusts(const LevelGrid& grid, LevelReport& r);
} // namespace levelcheck

#endif
