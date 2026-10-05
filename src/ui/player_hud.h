#ifndef PLAYER_HUD_H
#define PLAYER_HUD_H

#include "ui_draw.h"
#include <cstdint>
#include <string>

class Font;

// The player's HUD panel, bottom left: health (with a trailing "lost" part after a hit), stamina, the quick slots
// (weapon in hand, the potions H and 0 would drink) with their key caps, the key sockets and an XP line.
namespace PlayerHud {
// Panel size on screen: the layout is in panel units on a canvas 100 / SCALE high (square units, x from the left).
constexpr float SCALE = 0.85f;
constexpr ui::Rect PANEL = {1.5f, 1.5f, 53.f, 21.5f};

// Cells of the icon atlas textures/ui/hud_icons.png (tools/textures/hud_icons.py), in order.
enum class Icon : std::uint8_t { Club, Sword, Spear, Bow, Potion, None };

struct Slot {
	Icon icon = Icon::None; // None: an empty slot (no potion of that kind left)
	ui::Color tint = {1, 1, 1};
	int count = -1;			  // badge; < 0: none (the weapon)
	std::string key;		  // the key cap under the slot (from the bindings)
	int flashAgeMs = 1000000; // since the last use (a quick drink), for the flash
};

struct View {
	int hp = 0, maxHp = 1;
	int stamina = 0, maxStamina = 1;
	int staminaRefusedAgeMs = 1000000; // since the last jump or sprint refused for lack of stamina
	float xpRatio = 0.f;			   // progress to the next level, 0..1
	int keysHeld = 0;				   // bit (colour - 1) per key held
	int levelKeys = 0;				   // bit (colour - 1) per key on the level: one socket each
	Slot slots[3];					   // weapon, healing potion, stamina potion
};

// Once a game tick: the health bar's lost part follows the player's health (it holds, then drains).
void tick(int hp, int maxHp);
// A new game or a loaded one: no lost part left over.
void reset();

// Sets its own square-pixel ortho projection (100 / SCALE high) for a resX x resY window. numbers: health numbers;
// small: key caps and counts; icons: the icon atlas texture. Leaves texturing on and the HUD blend function
// (GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR) set.
void draw(const View& view, int resX, int resY, Font& numbers, Font& small, int icons);
} // namespace PlayerHud

#endif
