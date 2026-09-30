#ifndef HUD_H
#define HUD_H

// Flat bar with a white outline: the model viewer's loop progress. The game's HUD is src/ui/player_hud.h.
namespace Hud {
void drawBar(float left, float bottom, float width, float height, float ratio, float red, float green, float blue);
} // namespace Hud

#endif
