#ifndef HUD_ICON_H
#define HUD_ICON_H

#include "curses.h"
#include "items.h"
#include <cstdint>

namespace PlayerHud {
constexpr int ICON_COLUMNS = 8; // the atlas grid
constexpr int ICON_ROWS = 7;
// Cells of the icon atlas media/textures/ui/hud_icons.png (tools/textures/hud_icons.py), in order. Amulet and Ring: the
// inventory's tabs. Daze: the stars circling a dazed head (plan screech-bite). The real weapons follow from FirstWeapon
// on (club .. spear, the ranged ones; weaponIcon maps a kind to its cell, the test weapons to a real weapon's); the
// curses from FirstCurse on, in Curse order.
enum class Icon : std::uint8_t {
	Potion,
	Amulet,
	Ring,
	Daze,
	IsisWard, // the Protection of Isis (plan shrine-of-isis): her throne sign
	Drop, // white, tinted per effect on a status card (plan curse-blessing-cards): poison tiers, resistance, Hapi
	Burning, // a flame
	// The other gods' blessings (plan shrines-of-the-gods): Montu's in the cell left free here, the rest after the
	// curses.
	MontuBlessing, // a khopesh under a falcon's sun disc
	FirstWeapon = 8,
	FirstAmulet = 21,
	FirstCurse = 37,
	PtahBlessing = FirstCurse + CURSE_COUNT, // his djed pillar
	AmunBlessing, // his ram's head with its curled horn
	SerketBlessing, // her scorpion
	// A monster's terrain mood (plans terrain-affinity, monster-status-icons): an arrow up on its home ground, down on
	// the ground it hates.
	TerrainNative,
	TerrainHostile,
	None = 0xff
};
// The cells hold the real weapons only (no cell per test weapon: plan hud-ranged-weapon-icons), so a kind maps by a
// switch, not by its ItemKind offset. A test weapon shows the weapon it stands for.
constexpr Icon weaponIcon(ItemKind weapon) {
	int cell = 0;
	switch (weapon) {
	case ItemKind::Club:
	case ItemKind::SmashTest:
		cell = 0;
		break;
	case ItemKind::Dagger:
		cell = 1;
		break;
	case ItemKind::ShortSword:
	case ItemKind::CutTest:
		cell = 2;
		break;
	case ItemKind::Khopesh:
	case ItemKind::FireTest:
		cell = 3;
		break;
	case ItemKind::EpsilonAxe:
		cell = 4;
		break;
	case ItemKind::DuckbillAxe:
		cell = 5;
		break;
	case ItemKind::Mace:
		cell = 6;
		break;
	case ItemKind::Spear:
	case ItemKind::StabTest:
		cell = 7;
		break;
	case ItemKind::SelfBow:
	case ItemKind::BowTest:
		cell = 8;
		break;
	case ItemKind::CompositeBow:
		cell = 9;
		break;
	case ItemKind::Sling:
		cell = 10;
		break;
	case ItemKind::ThrowingStick:
		cell = 11;
		break;
	case ItemKind::Javelin:
		cell = 12;
		break;
	default:
		break;
	}
	// NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange): the cells from FirstWeapon on are the weapons'
	return static_cast<Icon>(static_cast<int>(Icon::FirstWeapon) + cell);
}
constexpr Icon amuletIcon(AmuletType type) {
	return static_cast<Icon>(static_cast<int>(Icon::FirstAmulet) + static_cast<int>(type));
}
constexpr Icon curseIcon(Curse curse) {
	// NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange): the cells from FirstCurse on are the curses'
	return static_cast<Icon>(static_cast<int>(Icon::FirstCurse) + static_cast<int>(curse));
}
} // namespace PlayerHud

#endif
