// Plan gamepad-options-tab: every bindable pad button has a label on Options > Gamepad, the labels do not overlap,
// sit in the panel and keep clear of the drawing.
#include "../../external/doctest/doctest.h"
#include "../../src/level/input/bindings.h"
#include "../../src/level/ui/pad_layout.h"

using namespace PadLayout;

TEST_CASE("pad layout: every bindable button has one label, Start none") {
	for (int button = 0; button < PadButton::COUNT; button++) {
		if (!keyName(InputKey::Pad(button)).empty() && !isReservedKey(InputKey::Pad(button)))
			CHECK_MESSAGE(calloutOf(button) != nullptr, keyName(InputKey::Pad(button)));
	}
	CHECK(calloutOf(PadButton::START) == nullptr);
	int bindable = 0;
	for (const Callout& c : callouts())
		bindable += c.Bindable() ? 1 : 0;
	CHECK(bindable == PadButton::COUNT - 2); // all but Start and the unused 5 (Guide)
}

TEST_CASE("pad layout: the labels do not overlap and stay inside the options panel, beside the drawing") {
	constexpr Rect PANEL_INSIDE = {11, 21, 138, 52}; // OPTIONS_PANEL inside its margin, under the tabs' rule
	constexpr Rect DRAWING = {CX - 28.f, 34.f, 56.f, 34.f}; // the body's grips reach x 53 to 107
	const auto all = callouts();
	for (size_t i = 0; i < all.size(); i++) {
		const Rect r = labelRect(all[i]);
		CHECK(r.within(PANEL_INSIDE));
		CHECK_FALSE(r.overlaps(DRAWING));
		for (size_t j = i + 1; j < all.size(); j++)
			CHECK_FALSE(r.overlaps(labelRect(all[j])));
		// A label on the side of its part: the line runs from the part outwards.
		const Point end = lineEnd(all[i]);
		CHECK((all[i].right ? end.x > all[i].anchor.x : end.x < all[i].anchor.x));
	}
}
