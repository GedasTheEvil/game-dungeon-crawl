#ifndef SKILLS_LAYOUT_H
#define SKILLS_LAYOUT_H

// Where the Heka screen's parts sit on the 160 x 100 canvas, and what a canvas point hits, without GL. SkillsScreen
// draws them (ui/skills_view.cpp).

#include "ui_layout.h"
#include "../world/skills.h"

namespace SkillsLayout {
using ui::Rect;

constexpr Rect CARDS_PANEL = {4, 13, 98, 72};
constexpr Rect DETAIL_PANEL = {106, 13, 50, 72}; // papyrus
constexpr Rect LEARN_BUTTON = {111, 16, 40, 7};
// The heka left to spend, over the cards.
constexpr float POINTS_BASELINE = 78.f;
// The cards: COLUMNS a row, in SkillId order.
constexpr int COLUMNS = 2;
constexpr float CARD_W = 45.f;
constexpr float CARD_H = 13.f;
constexpr float CARD_GAP_X = 2.f;
constexpr float CARD_GAP_Y = 1.6f;
constexpr float TOP_CARD_Y = 62.f; // the first row's bottom
constexpr int ROWS = (SKILL_COUNT + COLUMNS - 1) / COLUMNS;

[[nodiscard]] Rect cardRect(int card);
// The card under the canvas point, or -1.
[[nodiscard]] int cardAt(float x, float y);
} // namespace SkillsLayout

#endif
