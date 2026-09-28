#ifndef INK_H
#define INK_H

// Comic ink outlines for toon mode (F1): the 3D scene is drawn into an offscreen colour + depth target, then
// copied to the screen with dark lines where the depth jumps (silhouettes) or bends (creases). Things drawn with
// depth writes off (fire, particles, decals) get no lines.
//
// Per frame: begin() -> draw the 3D scene -> end() -> HUD. Both do nothing outside toon mode or without FBOs.
namespace Ink {

// zNear / zFar of the scene projection, to turn the depth buffer back into distances.
void begin(float zNear, float zFar);
void end();

// Player, monsters and items are drawn this much larger in toon mode: the outlines eat into their silhouettes.
float figureScale();

} // namespace Ink

#endif
