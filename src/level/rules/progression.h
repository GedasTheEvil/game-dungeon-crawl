#ifndef PROGRESSION_H
#define PROGRESSION_H

// Player levels and XP, without the player: PlayerStats keeps the numbers, these are the rules.

// XP total at which the player reaches `level` (level 2 at 1000).
[[nodiscard]] double levelXP(int level);
// 0..1 of the way from `level` to the next, at `xp` total.
[[nodiscard]] float levelProgress(int level, double xp);
// XP for a riddle answered at `level`: about a third of the way to the next level, at least 500.
[[nodiscard]] int riddleXP(int level);

#endif
