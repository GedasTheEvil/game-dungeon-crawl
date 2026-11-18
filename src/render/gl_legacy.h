#ifndef GL_LEGACY_H
#define GL_LEGACY_H

#include "default_shaders.h"

// The bridge between the ported drawing and the not yet ported (plan gl-core-foundation): while the context stays a
// compatibility one, the port goes file by file, and one side calls the other. Every legacy GL call the bridge needs
// lives here; plan gl-core-profile deletes the file with the last legacy caller.
namespace GlLegacy {
// Ported code about to call legacy code ("syncLegacy"): gProj and gMv's tops into GL_PROJECTION and GL_MODELVIEW
// (GL_MODELVIEW stays the current matrix mode).
void syncMatrices();
// Ported code called by legacy code: GL's current projection and model view into the tops of gProj and gMv, so a
// Batch draws where the caller's glTranslatef and the rest put it.
void adoptMatrices();
// adoptMatrices(), unless inside a PortedScope: for ported code that only reads the matrices.
void adoptUnlessPorted();
// The legacy current colour back to white, as a ported draw used to leave it for its legacy callers.
void resetColour();
// The legacy current colour: the tint of a legacy draw that follows (the model viewer's potion glass, plan
// gl-core-tools).
void colour(float r, float g, float b, float a);
// Fixed-function texturing on or off (glEnable / glDisable(GL_TEXTURE_2D)), as the legacy draws that follow expect it.
void texturing(bool on);
// The legacy current colour (glColor), for a ported draw a legacy caller tints that way (Font::print).
void currentColour(float rgba[4]);
// The legacy current texture coordinate (glTexCoord2f), as a legacy draw left it for the legacy draws after it.
void texCoord(float u, float v);
// GL_LIGHT0 from the lit shader's light, under the legacy model view (sync it first), with no specular and no global
// ambient: the journal's photo of a creature, whose model (AnimatedModel) is not yet ported (plan gl-core-ui).
void setLight(const DefaultShader::Light& light);
// The fixed-function lighting of setLight on (GL_LIGHTING, GL_LIGHT0, GL_NORMALIZE, GL_COLOR_MATERIAL for ambient and
// diffuse) or off.
void lighting(bool on);

// Ported code entered from anywhere (plan gl-core-world): while one is alive, gMv and gProj are the matrices and the
// world batch's current values (Lighting::batch) the colour, normal and texture coordinate; GL's are stale. The first
// one, entered from legacy code (the inventory's and the journal's models, the model viewer, the not yet ported
// scene), adopts GL's and gives its current values back to GL at its end; one inside another changes nothing. Ported
// code that calls legacy code holds a LegacyScope over the call (GL gets the matrices and current values, and back).
class PortedScope {
  public:
	PortedScope();
	~PortedScope();
	PortedScope(const PortedScope&) = delete;
	PortedScope& operator=(const PortedScope&) = delete;
  private:
	bool outer;
};
// Ported code calling legacy code: syncMatrices() and, while alive, the matrices are GL's again.
class LegacyScope {
  public:
	LegacyScope();
	~LegacyScope();
	LegacyScope(const LegacyScope&) = delete;
	LegacyScope& operator=(const LegacyScope&) = delete;

  private:
	bool outer;
};
// adoptMatrices(), unless inside a PortedScope: for ported code that only reads the matrices.
void adoptUnlessPorted();
// The legacy current colour back to white, as a ported draw used to leave it for its legacy callers.
void resetColour();

// While the ported world draws (plan gl-core-world, drawGameplay), gMv and gProj are the matrices and GL's are stale.
// Code that the ported world and legacy callers share (a model's Show, an item's Draw, Lighting::begin and add: the
// inventory's and the journal's models, the model viewer) adopts GL's matrices and colour only outside it.
void setPortedScene(bool on);
[[nodiscard]] bool portedScene();
// adoptMatrices(), unless in the ported scene.
void adoptUnlessPorted();
// adoptMatrices(), unless inside a PortedScope: for ported code that only reads the matrices.
void adoptUnlessPorted();
} // namespace GlLegacy

#endif
