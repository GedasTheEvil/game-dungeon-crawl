// Plan hud-ranged-weapon-icons: the HUD's weapon icon is the weapon's own cell (the atlas holds the real weapons
// only), the test weapons show a real weapon's, and no weapon spills into the amulets' cells.
#include "../../external/doctest/doctest.h"
#include "../../src/level/ui/hud_icon.h"

using PlayerHud::Icon;
using PlayerHud::weaponIcon;

namespace {
int cell(ItemKind kind) { return static_cast<int>(weaponIcon(kind)) - static_cast<int>(Icon::FirstWeapon); }
} // namespace

TEST_CASE("hud-ranged-weapon-icons: each weapon shows its own cell") {
	CHECK(cell(ItemKind::Club) == 0);
	CHECK(cell(ItemKind::Spear) == 7);
	CHECK(cell(ItemKind::SelfBow) == 8);
	CHECK(cell(ItemKind::CompositeBow) == 9);
	CHECK(cell(ItemKind::Sling) == 10);
	CHECK(cell(ItemKind::ThrowingStick) == 11);
	CHECK(cell(ItemKind::Javelin) == 12);
}

TEST_CASE("hud-ranged-weapon-icons: the test weapons borrow a real weapon's cell") {
	CHECK(weaponIcon(ItemKind::SmashTest) == weaponIcon(ItemKind::Club));
	CHECK(weaponIcon(ItemKind::StabTest) == weaponIcon(ItemKind::Spear));
	CHECK(weaponIcon(ItemKind::CutTest) == weaponIcon(ItemKind::ShortSword));
	CHECK(weaponIcon(ItemKind::FireTest) == weaponIcon(ItemKind::Khopesh));
	CHECK(weaponIcon(ItemKind::BowTest) == weaponIcon(ItemKind::SelfBow));
}

TEST_CASE("hud-ranged-weapon-icons: no weapon maps into the amulets' cells") {
	for (int i = 0; i < WEAPON_KIND_COUNT; i++) {
		const int icon = static_cast<int>(weaponIcon(static_cast<ItemKind>(i)));
		CHECK(icon >= static_cast<int>(Icon::FirstWeapon));
		CHECK(icon < static_cast<int>(Icon::FirstAmulet));
	}
}
