#ifndef DRAW_H
#define DRAW_H

// One frame of whatever screen is shown: the game (the dungeon, the player, the HUD) or a UI screen. Changes no game
// state; the tick is Update (state/game_loop.h).
void Draw();

#endif
