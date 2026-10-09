// The UI layouts as data (docs/plan/scenarios-to-unit-tests.md technique 5): at 4:3, 16:9 and 21:9 every part fits
// the canvas without overlap, a click on a part's centre hits it, and the fades run as they should.
#include "../../external/doctest/doctest.h"
#include "../../src/ui/inventory_layout.h"
#include "../../src/ui/page_turn.h"
#include "../../src/ui/ui_layout.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

using ui::Rect;

namespace {
struct Window {
	int w, h;
};
constexpr Window WINDOWS[] = {{1024, 768}, {1280, 720}, {2560, 1080}, {800, 1000}}; // 4:3, 16:9, 21:9, taller

constexpr Rect CANVAS = {0, 0, ui::CANVAS_W, ui::CANVAS_H};

// A click on the window pixel nearest canvas point (x, y), back on the canvas: where the screen's code finds it.
void click(const Window& win, float x, float y, float& cx, float& cy) {
	const Rect area = ui::visibleArea(ui::CANVAS_W, ui::CANVAS_H, win.w, win.h);
	int mx = 0, my = 0;
	ui::toWindow(area, win.w, win.h, x, y, mx, my);
	ui::toCanvas(area, win.w, win.h, mx, my, cx, cy);
}

// No two of them share any area.
void checkApart(const std::vector<Rect>& rects) {
	for (size_t a = 0; a < rects.size(); a++)
		for (size_t b = a + 1; b < rects.size(); b++) {
			CAPTURE(a);
			CAPTURE(b);
			CHECK_FALSE(rects[a].overlaps(rects[b]));
		}
}

ItemBag allFound() {
	ItemBag bag;
	for (int k = 0; k < ITEM_KIND_COUNT; k++)
		bag.Add(itemAt(k));
	return bag;
}
} // namespace

TEST_CASE("the canvas keeps its aspect, centred, whole in every window") {
	for (const Window& win : WINDOWS) {
		CAPTURE(win.w);
		const Rect area = ui::visibleArea(ui::CANVAS_W, ui::CANVAS_H, win.w, win.h);
		CHECK(CANVAS.within(area));
		CHECK(area.w / area.h == doctest::Approx(static_cast<float>(win.w) / static_cast<float>(win.h)));
		CHECK(area.cx() == doctest::Approx(CANVAS.cx()));
		CHECK(area.cy() == doctest::Approx(CANVAS.cy()));
		float x = 0.f, y = 0.f;
		ui::toCanvas(area, win.w, win.h, 0, 0, x, y); // the window's top left
		CHECK(x == doctest::Approx(area.x));
		CHECK(y == doctest::Approx(area.y + area.h));
		ui::toCanvas(area, win.w, win.h, win.w, win.h, x, y);
		CHECK(x == doctest::Approx(area.x + area.w));
		CHECK(y == doctest::Approx(area.y));
	}
}

TEST_CASE("the screen tabs sit in the canvas, apart, and a click on one opens it") {
	std::vector<Rect> tabs;
	for (int t = 0; t < ui::SCREEN_TAB_COUNT; t++) {
		tabs.push_back(ui::screenTabRect(t));
		CHECK(tabs.back().within(CANVAS));
	}
	checkApart(tabs);
	for (const Window& win : WINDOWS)
		for (int t = 0; t < ui::SCREEN_TAB_COUNT; t++) {
			float x = 0.f, y = 0.f;
			click(win, tabs[static_cast<size_t>(t)].cx(), tabs[static_cast<size_t>(t)].cy(), x, y);
			CHECK(ui::screenTabAt(x, y) == t);
		}
	CHECK(ui::screenTabAt(1.f, 1.f) == -1);
}

TEST_CASE("the inventory's panels, tabs, sort buttons and use buttons fit the canvas, apart") {
	using namespace InventoryLayout;
	CHECK(ITEMS_PANEL.within(CANVAS));
	CHECK(DETAIL_PANEL.within(CANVAS));
	checkApart({ITEMS_PANEL, DETAIL_PANEL});
	for (const Rect& b : {WIDE_BUTTON, EQUIP_BUTTON, UPGRADE_BUTTON})
		CHECK(b.within(DETAIL_PANEL));
	checkApart({EQUIP_BUTTON, UPGRADE_BUTTON});
	std::vector<Rect> row;
	for (int g = 0; g < ITEM_GROUP_COUNT; g++)
		row.push_back(tabRect(g));
	for (int o = 1; o < SORT_ORDER_COUNT; o++)
		row.push_back(sortRect(o));
	for (const Rect& r : row)
		CHECK(r.within(ITEMS_PANEL));
	checkApart(row);
	for (const ui::ScreenTab tab : {ui::ScreenTab::Inventory, ui::ScreenTab::Map, ui::ScreenTab::Journal}) {
		CHECK_FALSE(ui::screenTabRect(static_cast<int>(tab)).overlaps(ITEMS_PANEL));
		CHECK_FALSE(ui::screenTabRect(static_cast<int>(tab)).overlaps(DETAIL_PANEL));
	}
}

TEST_CASE("every tab's visible slots fit the items panel, apart, under its tabs, in every sort and scroll") {
	using namespace InventoryLayout;
	for (const ItemBag& bag : {ItemBag{}, allFound()})
		for (int g = 0; g < ITEM_GROUP_COUNT; g++) {
			const auto group = static_cast<ItemGroup>(g);
			const int scrolls = std::max(1, rowsOf(group) - VISIBLE_ROWS + 1);
			for (int s = 0; s < SORT_ORDER_COUNT; s++)
				for (int scroll = 0; scroll < scrolls; scroll++) {
					CAPTURE(g);
					CAPTURE(s);
					CAPTURE(scroll);
					const auto sort = static_cast<SortOrder>(s);
					std::vector<Rect> shown;
					int visible = 0;
					for (int slot = 0; slot < ITEM_KIND_COUNT; slot++) {
						if (itemGroup(itemAt(slot)) != group || !slotVisible(bag, sort, slot, scroll))
							continue;
						visible++;
						const Rect r = slotRect(bag, sort, slot, scroll);
						CHECK(r.within(ITEMS_PANEL));
						CHECK(r.y + r.h <= TAB_Y);
						shown.push_back(r);
					}
					CHECK(visible == std::min(groupItems(group).count - scroll * COLUMNS, VISIBLE_ROWS * COLUMNS));
					checkApart(shown);
				}
		}
}

TEST_CASE("a click on the centre of a slot, a tab, a sort or a use button hits it, in every window") {
	using namespace InventoryLayout;
	const ItemBag bag = allFound();
	for (const Window& win : WINDOWS) {
		CAPTURE(win.w);
		for (int g = 0; g < ITEM_GROUP_COUNT; g++) {
			const auto group = static_cast<ItemGroup>(g);
			for (int slot = 0; slot < ITEM_KIND_COUNT; slot++) {
				if (itemGroup(itemAt(slot)) != group || !slotVisible(bag, SortOrder::Name, slot, 0))
					continue;
				const Rect r = slotRect(bag, SortOrder::Name, slot, 0);
				float x = 0.f, y = 0.f;
				click(win, r.cx(), r.cy(), x, y);
				const Hit hit = hitAt(bag, group, SortOrder::Name, 0, true, x, y);
				CHECK(hit.slot == slot);
				CHECK(hit.tab == NO_HIT);
				CHECK(hit.button == Button::None);
			}
			float x = 0.f, y = 0.f;
			click(win, tabRect(g).cx(), tabRect(g).cy(), x, y);
			CHECK(hitAt(bag, group, SortOrder::Found, 0, true, x, y).tab == g);
		}
		for (int o = 1; o < SORT_ORDER_COUNT; o++) {
			float x = 0.f, y = 0.f;
			click(win, sortRect(o).cx(), sortRect(o).cy(), x, y);
			CHECK(hitAt(bag, ItemGroup::Weapons, SortOrder::Found, 0, true, x, y).sort == o);
		}
		float x = 0.f, y = 0.f;
		click(win, WIDE_BUTTON.cx(), WIDE_BUTTON.cy(), x, y);
		CHECK(hitAt(bag, ItemGroup::Potions, SortOrder::Found, 0, false, x, y).button == Button::Use);
		click(win, EQUIP_BUTTON.cx(), EQUIP_BUTTON.cy(), x, y);
		CHECK(hitAt(bag, ItemGroup::Weapons, SortOrder::Found, 0, true, x, y).button == Button::Use);
		click(win, UPGRADE_BUTTON.cx(), UPGRADE_BUTTON.cy(), x, y);
		CHECK(hitAt(bag, ItemGroup::Weapons, SortOrder::Found, 0, true, x, y).button == Button::Upgrade);
	}
}

TEST_CASE("the status box fades in, holds and fades out, centred under the top, one line per line") {
	CHECK(StatusBox::Alpha(0, 3000) == 0.f);
	CHECK(StatusBox::Alpha(75, 3000) == doctest::Approx(0.5f));
	CHECK(StatusBox::Alpha(150, 3000) == 1.f);
	CHECK(StatusBox::Alpha(1500, 3000) == 1.f);
	CHECK(StatusBox::Alpha(2750, 3000) == doctest::Approx(0.5f));
	CHECK(StatusBox::Alpha(3000, 3000) == 0.f);
	CHECK(StatusBox::Alpha(4000, 3000) == 0.f);

	CHECK(StatusBox::SplitLines("Now you are level 2\n") == std::vector<std::string>{"Now you are level 2"});
	CHECK(StatusBox::SplitLines("a\nb").size() == 2);
	CHECK(StatusBox::SplitLines("").empty());

	for (const Window& win : WINDOWS) {
		const float canvasW = StatusBox::CANVAS_H * static_cast<float>(win.w) / static_cast<float>(win.h);
		const Rect canvas = {0, 0, canvasW, StatusBox::CANVAS_H};
		const Rect one = StatusBox::Box(canvasW, 30.f, 1);
		const Rect three = StatusBox::Box(canvasW, 30.f, 3);
		CHECK(one.within(canvas));
		CHECK(three.within(canvas));
		CHECK(one.cx() == doctest::Approx(canvasW / 2));
		CHECK(one.y + one.h == doctest::Approx(three.y + three.h)); // grows down from the top
		CHECK(three.h > one.h);
		for (int i = 0; i < 3; i++) { // every line's pen inside the box
			CHECK(StatusBox::LineY(i) > three.y);
			CHECK(StatusBox::LineY(i) < three.y + three.h);
		}
		CHECK(StatusBox::Box(canvasW, 1.f, 1).w == doctest::Approx(StatusBox::Box(canvasW, 2.f, 1).w)); // its least
	}
}

TEST_CASE("the level gem gets more precious every five levels, the last one from there on") {
	CHECK(LevelGem::GemOf(1) == 0);
	CHECK(LevelGem::GemOf(5) == 0);
	CHECK(LevelGem::GemOf(6) == 1);
	CHECK(LevelGem::GemOf(11) == 2);
	CHECK(LevelGem::GemOf(26) == LevelGem::GEM_COUNT - 1);
	CHECK(LevelGem::GemOf(99) == LevelGem::GEM_COUNT - 1);
	CHECK(LevelGem::GemOf(0) == 0);
}

TEST_CASE("the health bar's lost part holds after a hit, then drains; a heal jumps past it") {
	PlayerHud::DamageTrail trail;
	CHECK(trail.update(1.f, 0) == 1.f);
	CHECK(trail.update(0.6f, 100) == 1.f); // the hit
	CHECK(trail.update(0.6f, 100 + PlayerHud::TRAIL_HOLD_MS) == 1.f);
	const float draining = trail.update(0.6f, 200 + PlayerHud::TRAIL_HOLD_MS);
	CHECK(draining < 1.f);
	CHECK(draining > 0.6f);
	CHECK(trail.update(0.6f, 5000) == 0.6f); // drained to the health
	CHECK(trail.update(0.9f, 5016) == doctest::Approx(0.9f));
}

TEST_CASE("the feedback toast shows, then fades over its last FADE_MS") {
	ui::Toast toast;
	CHECK(toast.Alpha(0) == 0.f);
	toast.Show("Saved to slot 2", 1000);
	CHECK(toast.Alpha(1000) == 1.f);
	CHECK(toast.Alpha(1000 + ui::Toast::MS - ui::Toast::FADE_MS / 2) == doctest::Approx(0.5f));
	CHECK(toast.Alpha(1000 + ui::Toast::MS) == 0.f);
}

TEST_CASE("a turning page stays on its spine: flat at 0, turned over at 1") {
	PageCurl c{40.f, 60.f, 40.f, 0.f};
	CHECK(curlProgress(c) == 0.f);
	c.cornerX = -40.f;
	CHECK(curlProgress(c) == 1.f);
	c.cornerX = 0.f;
	CHECK(curlProgress(c) == doctest::Approx(0.5f));
	for (const auto& [x, y] : {std::pair{90.f, 0.f}, {0.f, -90.f}, {-70.f, 70.f}, {20.f, 200.f}}) {
		c.cornerX = x;
		c.cornerY = y;
		clampCorner(c);
		CHECK(std::hypot(c.cornerX, c.cornerY) <= c.w + 0.001f);
		CHECK(std::hypot(c.cornerX, c.cornerY - c.h) <= std::hypot(c.w, c.h) + 0.001f);
	}
}
