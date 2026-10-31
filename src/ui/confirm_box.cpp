#include "confirm_box.h"
#include "../input/input.h"
#include "ui_draw.h"
#include <utility>
#include <vector>

namespace {
using namespace ui;

constexpr Rect BOX = {CANVAS_W / 2 - 44, 34, 88, 34};
constexpr float BUTTON_W = 26.f;
constexpr float BUTTON_H = 8.f;
constexpr float BUTTON_GAP = 6.f;
constexpr float LINE_STEP = 5.f;

Rect buttonRect(int target) {
	const float x = BOX.cx() + (target == 0 ? -BUTTON_GAP / 2 - BUTTON_W : BUTTON_GAP / 2);
	return {x, BOX.y + 7.f, BUTTON_W, BUTTON_H};
}
} // namespace

void ConfirmBox::Ask(const std::string& text, std::function<void()> yes) {
	question = text;
	onYes = std::move(yes);
	pointer.Clear();
	pointer.Move(YES);
}

void ConfirmBox::Cancel() {
	onYes = nullptr;
	pointer.Clear();
}

void ConfirmBox::answer(int target) {
	if (target == NO) {
		Cancel();
		return;
	}
	if (target != YES)
		return;
	// The answer may ask again (or close the screen that asked): the box is closed first.
	std::function<void()> yes = std::move(onYes);
	Cancel();
	yes();
}

int ConfirmBox::targetAt(float cx, float cy) {
	for (int target : {YES, NO})
		if (buttonRect(target).contains(cx, cy))
			return target;
	return NONE;
}

void ConfirmBox::Move(float cx, float cy) {
	const int at = targetAt(cx, cy);
	if (at != NONE)
		pointer.Move(at);
}

void ConfirmBox::Button(int button, bool down, float cx, float cy) {
	if (button != MOUSE_LEFT_BUTTON)
		return;
	const int at = targetAt(cx, cy);
	if (down)
		pointer.Down(at, button);
	else
		answer(pointer.Up(at, button));
}

void ConfirmBox::Navigate(NavKey key) {
	if (key == NavKey::Activate)
		answer(pointer.hovered);
	else if (key == NavKey::Left || key == NavKey::Right)
		pointer.Move(pointer.hovered == YES ? NO : YES);
}

void ConfirmBox::Draw(Font& heading, Font& body, Font& small, const Rect& area) {
	beginShapes();
	fillRect(area, BLACK, BLACK, 0.6f);
	panel(BOX, 0.97f);
	const std::vector<std::string> lines = wrap(body, question, BOX.w - 10.f);
	const float firstY = BOX.y + BOX.h - 9.f;
	for (int target : {YES, NO}) {
		const bool hovered = pointer.hovered == target;
		const Rect r = tile(
			buttonRect(target),
			target == YES ? TileStyle::Lapis : TileStyle::Stone,
			hovered,
			pointer.Held(target)
		);
		beginText();
		textCenteredIn(heading, r, target == YES ? "Yes" : "No", hovered ? TEXT_HOVER : LABEL);
		beginShapes();
	}
	beginText();
	for (size_t i = 0; i < lines.size(); i++)
		textCentered(body, BOX.cx(), firstY - static_cast<float>(i) * LINE_STEP, lines[i].c_str(), LABEL);
	textCentered(small, BOX.cx(), BOX.y + 2.5f, "Arrows: move    Enter: pick    Esc: no", LABEL_DIM);
}
