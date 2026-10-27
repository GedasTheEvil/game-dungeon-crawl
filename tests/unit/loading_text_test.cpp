#include "../../external/doctest/doctest.h"
#include "../../src/state/loading_text.h"
#include "../../src/world/items.h"
#include "../../src/world/monster_kinds.h"

// Plan loading-two-lines: no detail line runs off the right edge of the loading screen.
TEST_CASE("every monster name fits the loading screen's detail line") {
	for (int id = 1; id <= MONSTER_TYPE_MAX; id++)
		CHECK_MESSAGE(
			loading_text::detailWidth(monsterKind(id)->name) <= loading_text::MAX_WIDTH,
			monsterKind(id)->name
		);
}

TEST_CASE("every weapon name fits the loading screen's detail line") {
	for (int i = 0; i < WEAPON_KIND_COUNT; i++)
		CHECK_MESSAGE(
			loading_text::detailWidth(itemText(itemAt(i)).name) <= loading_text::MAX_WIDTH,
			itemText(itemAt(i)).name
		);
	CHECK(loading_text::detailWidth("Containers, potions and amulets") <= loading_text::MAX_WIDTH);
}

TEST_CASE("the step lines fit") {
	CHECK(loading_text::stepWidth("Loading game soundtrack") <= loading_text::MAX_WIDTH);
}
