#ifndef HUD_H
#define HUD_H

namespace Hud {
void drawBar(float left, float bottom, float width, float height, float ratio, float red, float green, float blue);
void drawPlayerBars(float healthRatio, float staminaRatio);
}

#endif
