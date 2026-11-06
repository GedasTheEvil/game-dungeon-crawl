#ifndef SKILLS_LAYOUT_H
#define SKILLS_LAYOUT_H

// Where the Heka screen's parts sit on the 160 x 100 canvas, and what a canvas point hits, without GL. SkillsScreen
// draws them (ui/skills_view.cpp).

#include "ui_layout.h"
#include "skills.h"

namespace SkillsLayout {
using ui::Rect;

constexpr Rect CARDS_PANEL = {4, 13, 98, 72};
constexpr Rect DETAIL_PANEL = {106, 13, 50, 72}; // papyrus
constexpr Rect LEARN_BUTTON = {111, 16, 40, 7};
// The heka left to spend, over the cards.
constexpr float POINTS_BASELINE = 78.f;
// The cards: COLUMNS a row, in SkillId order; VISIBLE_ROWS of the ROWS show, the list scrolls by whole rows (plan
// heka-surplus): `scroll` is the first row shown.
constexpr int COLUMNS = 2;
constexpr float CARD_W = 45.f;
constexpr float CARD_H = 11.f;
constexpr float CARD_GAP_X = 2.f;
constexpr float CARD_GAP_Y = 1.2f;
constexpr float TOP_CARD_Y = 64.f; // the first row's bottom
constexpr int ROWS = (SKILL_COUNT + COLUMNS - 1) / COLUMNS;
constexpr int VISIBLE_ROWS = 5;
constexpr int MAX_SCROLL = ROWS > VISIBLE_ROWS ? ROWS - VISIBLE_ROWS : 0;
// The scroll bar right of the cards: the track over the rows shown, the thumb where they are.
constexpr float SCROLL_BAR_W = 1.f;

[[nodiscard]] Rect cardRect(int card, int scroll = 0);
[[nodiscard]] bool cardVisible(int card, int scroll);
// The card shown under the canvas point, or -1.
[[nodiscard]] int cardAt(float x, float y, int scroll = 0);
// The scroll that shows the card's row, from `scroll` (moved as little as it can).
[[nodiscard]] int scrollTo(int card, int scroll);
[[nodiscard]] Rect scrollTrack();
[[nodiscard]] Rect scrollThumb(int scroll);
} // namespace SkillsLayout

#endif
