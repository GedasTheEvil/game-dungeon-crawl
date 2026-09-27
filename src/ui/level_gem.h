#ifndef LEVEL_GEM_H
#define LEVEL_GEM_H

class Font;

// HUD badge in the top right corner: a gem in a gold bezel with the level number on it.
// The gem gets more precious every 5 levels: carnelian (1-5), turquoise (6-10), lapis lazuli (11 on),
// and the bezel gets more gold beads.
namespace LevelGem {
// Sets its own square-pixel ortho projection for a resX x resY window. Leaves texturing on and the
// HUD blend function (GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR) set, like the rest of the HUD expects.
void draw(int level, int resX, int resY, Font& font);
} // namespace LevelGem

#endif
