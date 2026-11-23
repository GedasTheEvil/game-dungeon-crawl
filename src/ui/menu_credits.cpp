#include <algorithm>
#include <string>
#include <vector>
#include "menu.h"
#include "menu_internal.h"

// The credits' second page (plan credits-second-section): music, fonts and agents from content/credits, then the game
// version, as rows of name, author and licence in the shared panel look, scrolled with the arrows or the wheel.

#ifndef GAME_RELEASE // the makefile passes .version
#define GAME_RELEASE "unknown"
#endif

namespace {
using namespace ui;

struct Row {
	bool heading = false;
	CreditsLine line;
};

// Every group's heading and rows, then the "Game" group with the version.
std::vector<Row> flatten(const std::vector<CreditsGroup>& groups) {
	std::vector<Row> rows;
	for (const CreditsGroup& group : groups) {
		rows.push_back({true, {{group.heading}}});
		for (const CreditsLine& line : group.lines)
			rows.push_back({false, line});
	}
	rows.push_back({true, {{"Game"}}});
	rows.push_back({false, {{"Version", GAME_RELEASE}}});
	return rows;
}

// The text cut with ".." to fit the width: nothing leaves the panel.
std::string fitted(const Font& font, std::string text, float width) {
	if (font.TextWidth(text.c_str()) <= width)
		return text;
	while (!text.empty() && font.TextWidth((text + "..").c_str()) > width)
		text.pop_back();
	return text + "..";
}
} // namespace
using namespace menu_internal;

void MainMenu::ScrollCredits(int rows) {
	creditsScroll = std::clamp(creditsScroll + rows, 0, CreditsBar().MaxScroll());
}

// Right of the rows, over the rows shown; drawn and dragged only when they do not all fit.
ScrollBar MainMenu::CreditsBar() const {
	const float right = CREDITS_TEXT_PANEL.x + CREDITS_TEXT_PANEL.w - 7.f;
	const float top = CREDITS_TEXT_PANEL.y + CREDITS_TEXT_PANEL.h - 5.f;
	const float barH = static_cast<float>(CREDITS_VISIBLE_ROWS) * CREDITS_ROW_H;
	return {{right + 3.f, top - barH, 1.6f, barH}, static_cast<int>(flatten(credits).size()), CREDITS_VISIBLE_ROWS};
}

void MainMenu::DrawCreditsText() {
	const std::vector<Row> rows = flatten(credits);
	const float left = CREDITS_TEXT_PANEL.x + 5.f;
	const float right = CREDITS_TEXT_PANEL.x + CREDITS_TEXT_PANEL.w - 7.f; // room for the scroll bar
	const float top = CREDITS_TEXT_PANEL.y + CREDITS_TEXT_PANEL.h - 5.f;
	const int total = static_cast<int>(rows.size());
	const ScrollBar bar = CreditsBar();
	creditsScroll = std::clamp(creditsScroll, 0, bar.MaxScroll());
	for (int i = 0; i < CREDITS_VISIBLE_ROWS && creditsScroll + i < total; i++) {
		const Row& row = rows[static_cast<size_t>(creditsScroll) + static_cast<size_t>(i)];
		const Rect r = {left, top - static_cast<float>(i + 1) * CREDITS_ROW_H, right - left, CREDITS_ROW_H};
		const std::vector<std::string>& cells = row.line.fields;
		if (row.heading) {
			line(r.x, r.y + 0.2f, r.x + r.w, r.y + 0.2f, BRONZE, 1.f, 1.f);
		} else if (i % 2 == 0) {
			fillRect(r, {1, 1, 1}, {1, 1, 1}, 0.035f);
		}
		beginText();
		if (row.heading) {
			text(small, r.x, r.y + 0.8f, cells[0].c_str(), GOLD);
		} else {
			const float authorX = r.x + CREDITS_AUTHOR_X;
			const bool three = cells.size() > 2;
			const float licenceW = three ? small.TextWidth(cells[2].c_str()) : 0.f;
			text(small, r.x + 1.f, r.y + 0.5f, fitted(small, cells[0], CREDITS_AUTHOR_X - 2.f).c_str(), LABEL);
			if (cells.size() > 1) {
				const float authorW = (three ? r.x + r.w - licenceW - 2.f : r.x + r.w) - authorX;
				text(small, authorX, r.y + 0.5f, fitted(small, cells[1], authorW).c_str(), three ? LABEL_DIM : GOLD);
			}
			if (three)
				text(small, r.x + r.w - 1.f - licenceW, r.y + 0.5f, cells[2].c_str(), GOLD);
		}
		beginShapes();
	}
	if (bar.Shown()) { // the scroll bar: a track and a lapis thumb, lighter while dragged
		fillRect(bar.track, WELL, WELL, 0.9f);
		fillRect(bar.Thumb(creditsScroll), creditsDrag.Dragging() ? LAPIS_HOVER_TOP : LAPIS, LAPIS_DARK, 1.f);
	}
}
