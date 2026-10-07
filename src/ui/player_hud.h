#ifndef PLAYER_HUD_H
#define PLAYER_HUD_H

#include "ui_draw.h"
#include "../world/items.h"
#include "../world/poison.h"
#include <cstdint>
#include <string>

class Font;

// The player's HUD panel, bottom left: health (with a trailing "lost" part after a hit), stamina, the quick slots
// (weapon in hand, the potions H and 0 would drink) with their key caps, the key sockets and an XP line. Above it, one
// poison drop per running tier (one, two or three pips) with its seconds left.
namespace PlayerHud {
// Panel size on screen: the layout is in panel units on a canvas 100 / SCALE high (square units, x from the left).
constexpr float SCALE = 0.85f;
constexpr ui::Rect PANEL = {1.5f, 1.5f, 58.f, 21.5f};

// Cells of the icon atlas textures/ui/hud_icons.png (tools/textures/hud_icons.py), in order. Amulet and Ring: the
// inventory's tabs. The weapons follow from FirstWeapon on, in ItemKind order (weaponIcon).
enum class Icon : std::uint8_t { Potion, Amulet, Ring, FirstWeapon = 8, FirstAmulet = 21, None = 0xff };
constexpr Icon weaponIcon(ItemKind weapon) {
	return static_cast<Icon>(static_cast<int>(Icon::FirstWeapon) + itemIndex(weapon));
}
constexpr Icon amuletIcon(AmuletType type) {
	return static_cast<Icon>(static_cast<int>(Icon::FirstAmulet) + static_cast<int>(type));
}

struct Slot {
	Icon icon = Icon::None; // None: an empty slot (no potion of that kind left)
	ui::Color tint = {1, 1, 1};
	int count = -1;			  // badge; < 0: none (the weapon)
	std::string badge;		  // badge text instead of the count (the amulet's tier, I .. IV)
	std::string key;		  // the key cap under the slot (from the bindings); empty: no cap (the amulet)
	int flashAgeMs = 1000000; // since the last use (a quick drink), for the flash
};

struct View {
	int hp = 0, maxHp = 1;
	int stamina = 0, maxStamina = 1;
	int staminaRefusedAgeMs = 1000000;		  // since the last jump or sprint refused for lack of stamina
	float xpRatio = 0.f;					  // progress to the next level, 0..1
	int keysHeld = 0;						  // bit (colour - 1) per key held
	int levelKeys = 0;						  // bit (colour - 1) per key on the level: one socket each
	Slot slots[4];							  // weapon, healing potion, stamina potion, worn amulet
	int poisonLeftMs[POISON_TIER_COUNT] = {}; // per tier, 0: not running. Any: the health bar is green.
};

// Once a game tick: the health bar's lost part follows the player's health (it holds, then drains).
void tick(int hp, int maxHp);
// A new game or a loaded one: no lost part left over.
void reset();

// Sets its own square-pixel ortho projection (100 / SCALE high) for a resX x resY window. numbers: health numbers;
// small: key caps and counts; icons: the icon atlas texture. Leaves texturing on and the HUD blend function
// (GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR) set.
void draw(const View& view, int resX, int resY, Font& numbers, Font& small, int icons);
// One atlas icon (not None) filling `r`, in the canvas set up. Leaves texturing off.
void drawIcon(Icon icon, const ui::Rect& r, int icons, ui::Color tint);
} // namespace PlayerHud

#endif
