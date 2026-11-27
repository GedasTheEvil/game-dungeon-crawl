#include "pad_layout.h"
#include <array>
#include "../input/bindings.h"

namespace PadLayout {
namespace {
constexpr Point centreOf(const Rect& r) { return {r.x + r.w / 2, r.y + r.h / 2}; }

// Each column top to bottom in the order of the parts' heights, so the lines do not cross; the d-pad's right and the
// west face button (inner side) go round their neighbours.
constexpr std::array<Callout, 18> CALLOUTS = {{
	{PadButton::LEFT_TRIGGER, nullptr, false, 0, centreOf(LEFT_TRIGGER)},
	{PadButton::LEFT_SHOULDER, nullptr, false, 1, centreOf(LEFT_SHOULDER)},
	{PadButton::BACK, nullptr, false, 2, BACK_BUTTON},
	{PadButton::DPAD_UP, nullptr, false, 3, {DPAD.x, DPAD.y + DPAD_ARM}},
	{PadButton::DPAD_LEFT, nullptr, false, 4, {DPAD.x - DPAD_ARM, DPAD.y}},
	{PadButton::DPAD_DOWN, nullptr, false, 5, {DPAD.x, DPAD.y - DPAD_ARM}},
	{PadButton::DPAD_RIGHT, nullptr, false, 6, {DPAD.x + DPAD_ARM, DPAD.y}, true},
	{PadButton::LEFT_STICK, nullptr, false, 7, LEFT_STICK},
	{NO_BUTTON, "Walk, climb", false, 8, LEFT_STICK},
	{PadButton::RIGHT_TRIGGER, nullptr, true, 0, centreOf(RIGHT_TRIGGER)},
	{PadButton::RIGHT_SHOULDER, nullptr, true, 1, centreOf(RIGHT_SHOULDER)},
	{PadButton::START, "Menu", true, 2, START_BUTTON},
	{PadButton::NORTH, nullptr, true, 3, {FACE.x, FACE.y + FACE_STEP}},
	{PadButton::EAST, nullptr, true, 4, {FACE.x + FACE_STEP, FACE.y}},
	{PadButton::SOUTH, nullptr, true, 5, {FACE.x, FACE.y - FACE_STEP}},
	{PadButton::WEST, nullptr, true, 6, {FACE.x - FACE_STEP, FACE.y}, true},
	{PadButton::RIGHT_STICK, nullptr, true, 7, RIGHT_STICK},
	{NO_BUTTON, "Look", true, 8, RIGHT_STICK},
}};
} // namespace

std::span<const Callout> callouts() { return CALLOUTS; }

Rect labelRect(const Callout& callout) {
	const float cy = TOP_Y - static_cast<float>(callout.row) * ROW_STEP;
	return {callout.right ? RIGHT_X : LEFT_X, cy - LABEL_H / 2, LABEL_W, LABEL_H};
}

const Callout* calloutOf(int button) {
	for (const Callout& c : CALLOUTS)
		if (c.button == button && c.Bindable())
			return &c;
	return nullptr;
}

Point lineEnd(const Callout& callout) {
	const Rect r = labelRect(callout);
	return {callout.right ? r.x : r.x + r.w, r.cy()};
}
} // namespace PadLayout
