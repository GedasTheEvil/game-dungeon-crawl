#ifndef PATH_SCRIPT_H
#define PATH_SCRIPT_H

#include "level.h"
#include "level_check.h"
#include <string>

// levelcheck --script (plan subcell-checker-mismatches moved it here, for the unit tests): the path the checker found,
// as a scenario (docs/testing.md) that plays it in the game in god mode and expects the level finished at the end. It
// kills the boss (killboss) where the path reaches it. levelPath: the scenario's `level` line; startX: where the game
// puts the player (the entrance cell's left edge, Dungeon::Load).
[[nodiscard]] std::string
pathScript(const LevelGrid& grid, const LevelReport& report, const std::string& levelPath, float startX);

#endif
