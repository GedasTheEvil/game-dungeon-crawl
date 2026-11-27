#ifndef PAD_LAYOUT_H
#define PAD_LAYOUT_H

// Options > Gamepad (plan gamepad-options-tab): the drawing of the pad on the 160 x 100 canvas and the label of each
// of its parts, without GL. A label sits left of the drawing for the pad's left half, right for its right half, a
// line from the part to it. The menu draws them (ui/menu/menu_gamepad.cpp).

#include <span>
#include "ui_layout.h"

namespace PadLayout {
using ui::Rect;

struct Point {
	float x, y;
};

// The drawing, centred on x 80 between the two label columns.
constexpr float CX = 80.f;
constexpr Point DPAD = {64.f, 52.5f};
constexpr float DPAD_ARM = 2.6f; // the centre to the middle of an arm
constexpr float DPAD_ARM_W = 2.f;
constexpr Point FACE = {96.f, 52.5f};
constexpr float FACE_STEP = 2.9f; // the centre to a face button's
constexpr float FACE_R = 1.4f;
constexpr Point LEFT_STICK = {71.5f, 45.f};
constexpr Point RIGHT_STICK = {88.5f, 45.f};
constexpr float STICK_R = 3.f;
constexpr Point BACK_BUTTON = {75.5f, 56.5f};
constexpr Point START_BUTTON = {84.5f, 56.5f};
constexpr float SMALL_BUTTON_R = 1.f;
constexpr Rect LEFT_SHOULDER = {58.5f, 60.4f, 13.f, 2.4f};
constexpr Rect RIGHT_SHOULDER = {88.5f, 60.4f, 13.f, 2.4f};
constexpr Rect LEFT_TRIGGER = {60.f, 63.4f, 10.f, 4.f};
constexpr Rect RIGHT_TRIGGER = {90.f, 63.4f, 10.f, 4.f};

// The labels: two columns of ROWS, the first row's centre at TOP_Y.
constexpr int ROWS = 9;
constexpr float TOP_Y = 70.f;
constexpr float ROW_STEP = 3.9f;
constexpr float LABEL_W = 36.f;
constexpr float LABEL_H = 3.3f;
constexpr float LEFT_X = 11.f;
constexpr float RIGHT_X = 149.f - LABEL_W;

constexpr int NO_BUTTON = -1; // a stick's move: no PadButton

// A part of the pad with a label: a bindable button (fixed null), or a fixed part with what it does.
struct Callout {
	int button; // PadButton, or NO_BUTTON
	const char* fixed; // the fixed part's action ("Look"); null: the button's binding
	bool right; // the right column
	int row;
	Point anchor; // where the line starts, on the drawing (under the part)
	bool elbow = false; // the line goes down from the part to the label's height first, round its neighbours
	[[nodiscard]] bool Bindable() const { return fixed == nullptr; }
};

[[nodiscard]] std::span<const Callout> callouts();
[[nodiscard]] Rect labelRect(const Callout& callout);
// The label of a bindable button, or null.
[[nodiscard]] const Callout* calloutOf(int button);
// The label's end the line meets: its edge facing the drawing, at its middle.
[[nodiscard]] Point lineEnd(const Callout& callout);
} // namespace PadLayout

#endif
