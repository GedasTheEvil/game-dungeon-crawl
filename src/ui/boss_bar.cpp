#include "boss_bar.h"
#include "../render/ui_draw.h"
#include "player_hud.h"
#include "../render/font.h"
#include <GL/gl.h>
#include <algorithm>

using namespace ui;

namespace {
constexpr float WIDTH = 70.f;
constexpr float BAR_BOTTOM = 5.f;
constexpr float BAR_H = 2.4f;
constexpr float NAME_GAP = 1.6f; // bar top to the name's pen y
constexpr float HUD_GAP = 2.f; // to the player's HUD panel; closer and the bar moves up over the panel
// The status cards (plan monster-status-icons): the player's size on this canvas, over the bar's left end.
constexpr float CARD = 6.f * PlayerHud::SCALE;
constexpr float CARD_GAP = 1.6f; // bar top to the cards, and between two cards
} // namespace

namespace BossBar {
void draw(const char* name, float ratio, bool poisoned, const StatusList& statuses, const Look& look) {
	Font& font = *look.font;
	float canvasW = beginSquareCanvas(100.f, look.resX, look.resY);

	Rect bar = {(canvasW - WIDTH) / 2, BAR_BOTTOM, WIDTH, BAR_H};
	const Rect& hud = PlayerHud::PANEL;
	if (bar.x < (hud.x + hud.w) * PlayerHud::SCALE + HUD_GAP)
		bar.y = (hud.y + hud.h) * PlayerHud::SCALE + HUD_GAP;
	beginShapes();
	fillRect({bar.x - 0.6f, bar.y - 0.6f, bar.w + 1.2f, bar.h + 1.2f}, BLACK, BLACK, 0.6f);
	using namespace PlayerHud;
	fillRect(
		{bar.x, bar.y, bar.w * std::clamp(ratio, 0.f, 1.f), bar.h},
		poisoned ? POISON_TOP : BLOOD_TOP,
		poisoned ? POISON_BOTTOM : BLOOD_BOTTOM,
		1.f
	);
	strokeRect(bar, GOLD_DIM, 1.f, 1.5f);
	diamond(bar.x, bar.cy(), 1.2f, GOLD, 1.f);
	diamond(bar.x + bar.w, bar.cy(), 1.2f, GOLD, 1.f);
	for (int i = 0; i < statuses.count; i++)
		drawStatusCard(
			statuses.items[static_cast<size_t>(i)],
			{bar.x + 1.f + static_cast<float>(i) * (CARD + CARD_GAP), bar.y + bar.h + CARD_GAP, CARD, CARD},
			look.icons,
			look.cards,
			true
		);

	beginText();
	float y = bar.y + bar.h + NAME_GAP;
	textCentered(font, bar.cx() + 0.3f, y - 0.3f, name, BLACK);
	textCentered(font, bar.cx(), y, name, GOLD_BRIGHT);

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glColor3f(1, 1, 1);
}
} // namespace BossBar
