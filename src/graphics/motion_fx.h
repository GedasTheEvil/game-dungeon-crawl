#ifndef MOTION_FX_H
#define MOTION_FX_H

// The sprint's sense of speed: a wider view (FOV kick), a radial blur that streaks the screen edges while the player
// in the middle stays sharp, and darker edges (vignette). All three ease in and out together.
//
// Per frame: update() -> fov() for the projection -> begin() -> draw the 3D scene -> end() -> HUD.
// The blur needs a framebuffer and shaders, the vignette shaders; without them only the FOV kick is left.
namespace MotionFx {

// Eases towards on or off over EASE_MS, by the game clock. Once a frame, before the scene.
void update(bool on, int nowMs);
// 0 (off) .. 1 (full), eased.
[[nodiscard]] float strength();
// The view angle for a base angle: wider by up to FOV_KICK degrees.
[[nodiscard]] float fov(float baseDegrees);

// With blur, the scene goes to an offscreen target. Does nothing at strength 0 or without blur.
void begin(int width, int height, bool blur);
// The scene to the screen, blurred away from the centre (window share 0..1, y up), then the vignette over it.
void end(float centreX, float centreY);

} // namespace MotionFx

#endif
