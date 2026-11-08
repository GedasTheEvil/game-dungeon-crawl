#ifndef BRIGHTNESS_H
#define BRIGHTNESS_H

// The picture's brightness (plan settings-extras): a gamma over the whole finished frame, the screens and the HUD
// too, so the Options calibration ankh shows what the game will look like. The curve is brightenedChannel
// (level/settings_ini.h).
namespace Brightness {
// In percent, MIN_BRIGHTNESS .. MAX_BRIGHTNESS; 100 draws nothing extra.
void set(int percent);
// Once a frame, after everything is drawn and before the swap: the back buffer to a texture and back through the
// gamma shader. Does nothing at 100, or without shaders.
void apply(int width, int height);
} // namespace Brightness

#endif
