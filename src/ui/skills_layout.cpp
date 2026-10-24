#include "skills_layout.h"

namespace SkillsLayout {

Rect cardRect(int card) {
	constexpr float GRID_W = COLUMNS * CARD_W + (COLUMNS - 1) * CARD_GAP_X;
	const float x0 = CARDS_PANEL.cx() - GRID_W / 2;
	const int column = card % COLUMNS;
	const int row = card / COLUMNS;
	return {
		x0 + static_cast<float>(column) * (CARD_W + CARD_GAP_X),
		TOP_CARD_Y - static_cast<float>(row) * (CARD_H + CARD_GAP_Y),
		CARD_W,
		CARD_H
	};
}

int cardAt(float x, float y) {
	for (int card = 0; card < SKILL_COUNT; card++)
		if (cardRect(card).contains(x, y))
			return card;
	return -1;
}

} // namespace SkillsLayout
