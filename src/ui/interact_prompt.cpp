#include "interact_prompt.h"
#include "player_hud.h"
#include "ui_layout.h"
#include "../core/timer.h"
#include "../graphics/font.h"
#include <GL/gl.h>

using namespace ui;

namespace {
constexpr float VERB_DROP = 2.5f; // box centre to the body font's pen y, as the HUD's health numbers

InteractPrompt::Tracker tracker;
} // namespace

namespace InteractPrompt {
void draw(Interaction verb, const std::string& key, int resX, int resY, Font& body, Font& small) {
	const float alpha = tracker.Update(verb, GameClock::now());
	if (alpha <= 0.f)
		return;
	using namespace PlayerHud;
	const char* word = interactionVerb(verb);
	// Font::print resets the modelview, so the canvas is set through the projection (as PlayerHud::draw).
	const float canvasW = beginSquareCanvas(100.f / SCALE, resX, resY, 200.f);
	glLoadIdentity();
	const float capW = key.empty() ? 0.f : small.TextWidth(key.c_str()) + CAP_PAD;
	const float gap = key.empty() ? 0.f : GAP;
	const Rect box = Box(canvasW, capW + gap + body.TextWidth(word), PANEL);
	const Rect cap = {box.x + PAD_X, box.cy() - CAP_H / 2, capW, CAP_H};

	beginShapes();
	fillRect({box.x + 0.7f, box.y - 0.9f, box.w, box.h}, BLACK, BLACK, 0.45f * alpha); // drop shadow
	panel(box, 0.9f * alpha, alpha);
	if (!key.empty()) {
		fillRect({cap.x, cap.y - 0.25f, cap.w, cap.h}, BLACK, BLACK, 0.5f * alpha);
		fillRect(cap, CAP_TOP, CAP_BOTTOM, alpha);
		strokeRect(cap, GOLD_DIM, alpha, 1.f);
	}

	beginText();
	if (!key.empty())
		textCentered(small, cap.cx(), cap.cy() - CAP_PEN_DROP + 0.5f, key.c_str(), GOLD, alpha);
	const float x = box.x + PAD_X + capW + gap;
	const float y = box.cy() - VERB_DROP;
	text(body, x + 0.3f, y - 0.3f, word, BLACK, alpha);
	text(body, x, y, word, GOLD_BRIGHT, alpha);

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glColor3f(1, 1, 1);
}
} // namespace InteractPrompt
