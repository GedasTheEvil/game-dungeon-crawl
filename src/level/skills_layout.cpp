#include "skills_layout.h"
#include <algorithm>

namespace SkillsLayout {
namespace {
constexpr float GRID_W = COLUMNS * CARD_W + (COLUMNS - 1) * CARD_GAP_X;
constexpr float ROW_STEP = CARD_H + CARD_GAP_Y;
} // namespace

Rect cardRect(int card, int scroll) {
	const float x0 = CARDS_PANEL.cx() - GRID_W / 2;
	const int column = card % COLUMNS;
	const int row = card / COLUMNS - scroll;
	return {
		x0 + static_cast<float>(column) * (CARD_W + CARD_GAP_X),
		TOP_CARD_Y - static_cast<float>(row) * ROW_STEP,
		CARD_W,
		CARD_H
	};
}

bool cardVisible(int card, int scroll) {
	const int row = card / COLUMNS;
	return card >= 0 && card < SKILL_COUNT && row >= scroll && row < scroll + VISIBLE_ROWS;
}

int cardAt(float x, float y, int scroll) {
	for (int card = 0; card < SKILL_COUNT; card++)
		if (cardVisible(card, scroll) && cardRect(card, scroll).contains(x, y))
			return card;
	return -1;
}

int scrollTo(int card, int scroll) {
	const int row = card / COLUMNS;
	return std::clamp(std::clamp(scroll, row - VISIBLE_ROWS + 1, row), 0, MAX_SCROLL);
}

Rect scrollTrack() {
	const float top = TOP_CARD_Y + CARD_H;
	const float bottom = TOP_CARD_Y - (VISIBLE_ROWS - 1) * ROW_STEP;
	const float gridRight = CARDS_PANEL.cx() + GRID_W / 2;
	const float x = (gridRight + CARDS_PANEL.x + CARDS_PANEL.w) / 2 - SCROLL_BAR_W / 2;
	return {x, bottom, SCROLL_BAR_W, top - bottom};
}

Rect scrollThumb(int scroll) {
	const Rect track = scrollTrack();
	const float rowH = track.h / static_cast<float>(ROWS);
	const float top = track.y + track.h - rowH * static_cast<float>(scroll);
	return {track.x, top - rowH * VISIBLE_ROWS, track.w, rowH * VISIBLE_ROWS};
}

} // namespace SkillsLayout
