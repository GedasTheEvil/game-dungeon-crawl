// The editor palette's scroll (plan editor-palette-scroll): the decor layer's 34 slots overflowed the 3-row panel.
#include "../../external/doctest/doctest.h"
#include "../../tools/editor/palette_scroll.h"

namespace {
constexpr ui::Rect PANEL = {84.f, 53.f, 70.f, 27.f}; // as TILES_PANEL in the editor
constexpr int DECOR_SLOTS = 34;
} // namespace

TEST_CASE("editor-palette-scroll: the panel holds 3 rows, the decor layer scrolls 1") {
	CHECK(paletteVisibleRows(PANEL) == 3);
	CHECK(paletteMaxScroll(DECOR_SLOTS, PANEL) == 1);
	CHECK(paletteMaxScroll(27, PANEL) == 0);
}

TEST_CASE("editor-palette-scroll: no shown slot leaves the panel, every slot shows at some scroll") {
	for (int scroll = 0; scroll <= paletteMaxScroll(DECOR_SLOTS, PANEL); scroll++)
		for (int slot = 0; slot < DECOR_SLOTS; slot++)
			if (paletteSlotShown(slot, scroll, PANEL))
				CHECK(paletteSlotRect(slot, scroll, PANEL).within(PANEL));
	for (int slot = 0; slot < DECOR_SLOTS; slot++)
		CHECK((paletteSlotShown(slot, 0, PANEL) || paletteSlotShown(slot, 1, PANEL)));
	CHECK_FALSE(paletteSlotShown(30, 0, PANEL)); // the 4th row hides at the top
	CHECK(paletteSlotShown(30, 1, PANEL));
	CHECK_FALSE(paletteSlotShown(0, 1, PANEL));
}

TEST_CASE("editor-palette-scroll: a hidden slot is never hit, a scrolled one is") {
	const ui::Rect fourth = paletteSlotRect(30, 0, PANEL); // below the panel, over the brush panel
	CHECK(paletteSlotAt(fourth.cx(), fourth.cy(), DECOR_SLOTS, 0, PANEL) == -1);
	const ui::Rect shown = paletteSlotRect(30, 1, PANEL);
	CHECK(paletteSlotAt(shown.cx(), shown.cy(), DECOR_SLOTS, 1, PANEL) == 30);
}

TEST_CASE("editor-palette-scroll: the scroll follows a slot") {
	CHECK(paletteScrollTo(33, 0, DECOR_SLOTS, PANEL) == 1);
	CHECK(paletteScrollTo(2, 1, DECOR_SLOTS, PANEL) == 0);
	CHECK(paletteScrollTo(10, 1, DECOR_SLOTS, PANEL) == 1); // shows already
	CHECK(paletteScrollTo(-1, 5, DECOR_SLOTS, PANEL) == 1); // no slot: only clamps
}
