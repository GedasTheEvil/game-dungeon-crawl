#ifndef GAME_LOOP_H
#define GAME_LOOP_H

// One game tick (every 16 ms, or one scenario tick): loads the assets first, then, while the game screen is shown,
// moves the world, the player and the attack on and advances the animations. Draw (graphics/draw.h) shows the result.
void Update();

#endif
