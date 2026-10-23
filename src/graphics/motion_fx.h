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

// The centre of the blur and the vignette (window share 0..1, y up): the player's chest. Once a frame, before end().
void setCentre(float centreX, float centreY);
// Toon mode (plan sprint-toon-blur-and-extras): the blur comes before the ink lines (true, Ink::end calls blurPass,
// the lines stay sharp) or after them (false, the lines streak with the picture). Both stay until the user picks one.
void setToonBlurFirst(bool first);
[[nodiscard]] bool toonBlurFirst();
// True when this frame's blur is done inside Ink::end (toon mode, blur first, an effect is on).
[[nodiscard]] bool blurInInk();
// Draws the blurred, tinted texture into the bound framebuffer, the viewport the window's.
void blurPass(unsigned sceneTexture);

// With the scene going to an offscreen target for the blur and the tint (not when toon mode blurs before the ink lines,
// that is Ink's target). Does nothing at strength 0 and no cut-scene.
void begin(int width, int height, bool toon);
// The scene to the screen, blurred away from the centre and tinted, then the vignette and the bars over it.
void end();

} // namespace MotionFx

#endif
