#ifndef HUD_ICON_H
#define HUD_ICON_H

#include "curses.h"
#include "items.h"
#include <cstdint>

namespace PlayerHud {
// Cells of the icon atlas media/textures/ui/hud_icons.png (tools/textures/hud_icons.py), in order. Amulet and Ring: the
// inventory's tabs. Daze: the stars circling a dazed head (plan screech-bite). The weapons follow from FirstWeapon on,
// in ItemKind order (weaponIcon); the curses from FirstCurse on, in Curse order.
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
	None = 0xff
};
constexpr Icon weaponIcon(ItemKind weapon) {
	return static_cast<Icon>(static_cast<int>(Icon::FirstWeapon) + itemIndex(weapon));
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
