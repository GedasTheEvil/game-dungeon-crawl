#ifndef MOTION_FX_H
#define MOTION_FX_H

// The sprint's sense of speed: a wider view (FOV kick), a radial blur that streaks the screen edges while the player
// in the middle stays sharp, and darker edges (vignette). All three ease in and out together.
// A cut-scene's look (plan cinematic-effects) goes the same way: a sepia tint, the vignette and letterbox bars.
//
// Per frame: update() and cinema() -> fov() for the projection -> begin() -> draw the 3D scene -> end() -> HUD.
// The blur needs a framebuffer and shaders, the vignette shaders; without them only the FOV kick is left.
namespace MotionFx {

// Eases towards on or off over EASE_MS, by the game clock. Once a frame, before the scene.
void update(bool on, int nowMs);
// 0 (off) .. 1 (full), eased.
[[nodiscard]] float strength();
// How far a cut-scene's look is in this frame, 0 (none) .. 1 (CutShot::look).
void cinema(float amount);
// The view angle for a base angle: wider by up to FOV_KICK degrees.
[[nodiscard]] float fov(float baseDegrees);

// With blur (false: no post-processing of the scene, toon mode), the scene goes to an offscreen target for the blur
// and the tint. Does nothing at strength 0 and no cut-scene, or without blur.
void begin(int width, int height, bool blur);
// The scene to the screen, blurred away from the centre (window share 0..1, y up) and tinted, then the vignette and
// the bars over it.
void end(float centreX, float centreY);

} // namespace MotionFx

#endif
