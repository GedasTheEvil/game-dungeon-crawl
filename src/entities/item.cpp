#include "item.h"
#include <GL/gl.h>
#include "../graphics/ink.h"
#include "../graphics/lighting.h"
#include "../core/timer.h"
#include <cmath>
#include <algorithm>
#include <string>
#include "../core/paths.h"

void Item::Draw(float pose) { drawScaled(scale * Ink::figureScale(), pose); }

void Item::DrawHeld(float pose) { drawScaled(scale * Ink::heldWeaponScale(), pose); }

void Item::drawScaled(float drawScale, float pose) {
	if (!mdl)
		return;

	glPushMatrix();
	glTranslatef(0, 0, -DRAW_DEPTH);
	glScalef(drawScale, drawScale, drawScale);
	tex.Bind();
	glRotatef(rotA, 0, 1, 0);
	AnimPlayback playback;
	playback.frame = std::clamp(pose, 0.f, 1.f) * static_cast<float>(mdl->FrameCount() - 1);
	mdl->Show(playback, tex.ID());
	glPopMatrix();
}

bool Item::loadModel(const char* name, const char* texture) {
	tex.LoadPNG((MEDIA_DIR "textures/items/" + std::string(texture != nullptr ? texture : name) + ".png").c_str());
	mdl = std::make_shared<AnimatedModel>();
	mdl->Load((MEDIA_DIR "models/items/" + std::string(name) + ".md3").c_str());
	norm = mdl->Centrify();
	mdl->BindTexture(tex.ID());
	mdl->Compile();
	return true;
}

void Item::shareModel(const Item& owner, const char* texture) {
	tex.LoadPNG((MEDIA_DIR "textures/items/" + std::string(texture) + ".png").c_str());
	mdl = owner.mdl;
	norm = owner.norm;
}

void Item::loadPart(const char* name, Rgb tint, float alpha) {
	part.tex = std::make_shared<Texture>();
	part.tex->LoadPNG((MEDIA_DIR "textures/items/" + std::string(name) + ".png").c_str());
	part.mdl = std::make_shared<AnimatedModel>();
	part.mdl->Load((MEDIA_DIR "models/items/" + std::string(name) + ".md3").c_str());
	part.mdl->Normalize(norm);
	const auto [low, high] = part.mdl->YRange(0);
	part.centreY = (low + high) / 2.f;
	part.mdl->BindTexture(part.tex->ID());
	part.mdl->Compile();
	part.tint = tint;
	part.alpha = alpha;
}

void Item::sharePart(const Item& owner, Rgb tint) {
	part = owner.part;
	part.tint = tint;
}

namespace {
constexpr float GLINT_PERIOD_MS = 1800.f; // the gem's glint swells and fades
constexpr float GLINT_MIN = 0.15f, GLINT_MAX = 0.55f; // its additive strength
constexpr float GLINT_GROW = 1.06f; // a halo a little bigger than the gem
} // namespace

void Item::DrawPart() {
	if (!part.mdl)
		return;
	drawPartScaled(scale * Ink::figureScale());
}

void Item::drawPartScaled(float drawScale) {
	float colour[4];
	glGetFloatv(GL_CURRENT_COLOR, colour);
	const Rgb c = {colour[0] * part.tint.r, colour[1] * part.tint.g, colour[2] * part.tint.b};
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);
	Lighting::setEmissive(true); // it shines in the dark, a lamp of its own
	glPushMatrix();
	glTranslatef(0, 0, -DRAW_DEPTH);
	glScalef(drawScale, drawScale, drawScale);
	glRotatef(rotA, 0, 1, 0);
	AnimPlayback playback;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(c.r, c.g, c.b, part.alpha);
	part.mdl->Show(playback, part.tex->ID());
	// The glint: the gem again, a little bigger, added on, swelling and fading.
	const float phase = static_cast<float>(GameClock::now() % static_cast<int>(GLINT_PERIOD_MS)) / GLINT_PERIOD_MS;
	const float glint = GLINT_MIN + (GLINT_MAX - GLINT_MIN) * 0.5f * (1.f - std::cos(phase * 2.f * 3.14159265f));
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glColor4f(c.r, c.g, c.b, glint);
	glTranslatef(0, part.centreY, 0);
	glScalef(GLINT_GROW, GLINT_GROW, GLINT_GROW);
	glTranslatef(0, -part.centreY, 0);
	part.mdl->Show(playback, part.tex->ID());
	glPopMatrix();
	Lighting::setEmissive(false);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glColor4f(colour[0], colour[1], colour[2], colour[3]);
}
