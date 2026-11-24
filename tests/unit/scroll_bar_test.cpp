// The shared scroll bar (plan scrollbar-mouse-drag): the thumb drags with the mouse, the track pages, on every screen.
#include "../../external/doctest/doctest.h"
#include "../../src/level/ui/inventory_layout.h"
#include "../../src/level/ui/scroll_bar.h"
#include "../../src/level/ui/skills_layout.h"
#include "../../tools/editor/palette_scroll.h"

namespace {
// 10 rows, 4 shown: a 40 high track, 4 a row.
const ui::ScrollBar BAR = {{50.f, 10.f, 1.f, 40.f}, 10, 4};
} // namespace

TEST_CASE("scrollbar-mouse-drag: the thumb follows the mouse, by whole rows, within the list") {
	int scroll = 0;
	ui::ScrollDrag drag;
	REQUIRE(BAR.Thumb(0).y == doctest::Approx(34.f)); // rows 0..3: 50 down to 34
	REQUIRE(drag.Down(BAR, 50.5f, 40.f, scroll)); // 10 under the thumb's top
	CHECK(drag.Dragging());
	CHECK(scroll == 0); // a press on the thumb does not move it
	CHECK(drag.Move(BAR, 32.f, scroll)); // 8 down: two rows
	CHECK(scroll == 2);
	drag.Move(BAR, 30.5f, scroll); // 9.5 down: nearest is two rows still
	CHECK(scroll == 2);
	drag.Move(BAR, 29.f, scroll); // 11 down: three
	CHECK(scroll == 3);
	drag.Move(BAR, -100.f, scroll); // far below the track: the last rows
	CHECK(scroll == BAR.MaxScroll());
	drag.Move(BAR, 200.f, scroll); // far above: the first
	CHECK(scroll == 0);
	drag.Up();
	CHECK_FALSE(drag.Move(BAR, 20.f, scroll)); // released: the mouse no longer moves it
	CHECK(scroll == 0);
}

TEST_CASE("scrollbar-mouse-drag: a click on the track pages towards it, off the bar does nothing") {
	int scroll = 0;
	ui::ScrollDrag drag;
	CHECK(drag.Down(BAR, 50.5f, 12.f, scroll)); // under the thumb
	CHECK_FALSE(drag.Dragging());
	CHECK(scroll == 4);
	CHECK(drag.Down(BAR, 50.5f, 12.f, scroll));
	CHECK(scroll == BAR.MaxScroll()); // clamped
	CHECK(drag.Down(BAR, 50.5f, 49.f, scroll)); // over it
	CHECK(scroll == BAR.MaxScroll() - 4);
	CHECK_FALSE(drag.Down(BAR, 45.f, 30.f, scroll)); // beside the bar
	CHECK(scroll == BAR.MaxScroll() - 4);
	CHECK(drag.Down(BAR, 50.f - ui::ScrollBar::GRAB_PAD / 2, 49.f, scroll)); // just beside the thin track still hits
	const ui::ScrollBar fits = {BAR.track, 4, 4};
	CHECK_FALSE(drag.Down(fits, 50.5f, 30.f, scroll)); // no bar when every row shows
}

TEST_CASE("scrollbar-mouse-drag: every screen's bar sits in its panel, clear of the slots it scrolls") {
	namespace inv = InventoryLayout;
	for (int group = 0; group < ITEM_GROUP_COUNT; group++) {
		const ui::ScrollBar bar = inv::scrollBar(static_cast<ItemGroup>(group));
		CHECK(bar.track.within(inv::ITEMS_PANEL));
		const inv::GridOrder order = tabOrder(ItemBag{}, static_cast<ItemGroup>(group), SortOrder::Found);
		for (ItemKind kind : order) {
			const ui::Rect r = inv::slotRect(order, itemIndex(kind), 0);
			CHECK(r.x + r.w <= bar.track.x - ui::ScrollBar::GRAB_PAD);
		}
	}

	const ui::Rect panel = {84.f, 53.f, 72.f, 27.f}; // the editor's TILES_PANEL
	constexpr int DECOR_SLOTS = 34;
	const ui::ScrollBar palette = paletteScrollBar(DECOR_SLOTS, panel);
	CHECK(palette.Shown());
	CHECK(palette.track.within(panel));
	for (int slot = 0; slot < DECOR_SLOTS; slot++) {
		const ui::Rect r = paletteSlotRect(slot, 0, panel);
		CHECK(r.x + r.w <= palette.track.x - ui::ScrollBar::GRAB_PAD);
		CHECK(r.x >= panel.x);
	}
	CHECK(palette.Thumb(0).y + palette.Thumb(0).h == doctest::Approx(paletteSlotRect(0, 0, panel).y + ::SLOT_H));
	CHECK(palette.track.y == doctest::Approx(paletteSlotRect(2 * PALETTE_COLUMNS, 0, panel).y));
	CHECK(SkillsLayout::scrollBar().MaxScroll() == SkillsLayout::MAX_SCROLL);
}
