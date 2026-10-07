#ifndef BOSS_BAR_H
#define BOSS_BAR_H

class Font;

// HUD bar of the boss in a fight: its name over a long health bar at the bottom centre (the status message owns
// the top centre).
namespace BossBar {
// ratio: health left, 0..1; poisoned: the bar is green, like the player's. Sets its own square-pixel ortho projection
// (100 high) for a resX x resY window. Leaves texturing on and the HUD blend function (GL_SRC_COLOR,
// GL_ONE_MINUS_SRC_COLOR) set.
void draw(const char* name, float ratio, bool poisoned, int resX, int resY, Font& font);
} // namespace BossBar

#endif
