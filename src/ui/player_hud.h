#ifndef PLAYER_HUD_H
#define PLAYER_HUD_H

#include "ui_draw.h"
#include "../world/items.h"
#include "../world/curses.h"
#include "../world/poison.h"
#include <cstdint>
#include <string>

class Font;

// The player's HUD panel, bottom left: health (with a trailing "lost" part after a hit), stamina, the quick slots
// (weapon in hand, the potions H and 0 would drink) with their key caps, the key sockets, the worn rings' gems and an
// XP line. Above it, one poison drop per running tier (one, two or three pips) with its seconds left, a running
// resistance potion's (one pip lesser, two greater), a running Blessing of Hapi (a blue-green drop), then each running
// curse in a purple frame (one pip lesser, two greater), then the daze (plan screech-bite) in a sand-gold frame.
namespace PlayerHud {
// Panel size on screen: the layout is in panel units on a canvas 100 / SCALE high (square units, x from the left).
constexpr float SCALE = 0.85f;
constexpr ui::Rect PANEL = {1.5f, 1.5f, 58.f, 21.5f};
// The health bar's fill, the boss bar's too: blood, green while poisoned.
constexpr ui::Color BLOOD_TOP = {0.85f, 0.14f, 0.08f};
constexpr ui::Color BLOOD_BOTTOM = {0.45f, 0.05f, 0.03f};
constexpr ui::Color POISON_TOP = {0.45f, 0.82f, 0.2f};
constexpr ui::Color POISON_BOTTOM = {0.1f, 0.36f, 0.05f};
// The key caps under the slots, the interact prompt's too: the options screen's.
constexpr ui::Color CAP_TOP = {0.19f, 0.15f, 0.10f};
constexpr ui::Color CAP_BOTTOM = {0.10f, 0.08f, 0.055f};
constexpr float CAP_H = 2.6f;
constexpr float CAP_PAD = 2.2f; // added to the key's width
constexpr float CAP_PEN_DROP = 1.8f; // cap centre to the small font's pen y

// Cells of the icon atlas media/textures/ui/hud_icons.png (tools/textures/hud_icons.py), in order. Amulet and Ring: the
// inventory's tabs. Daze: the stars circling a dazed head (plan screech-bite). The weapons follow from FirstWeapon on,
// in ItemKind order (weaponIcon); the curses from FirstCurse on, in Curse order.
enum class Icon : std::uint8_t {
	Potion,
	Amulet,
	Ring,
	Daze,
	FirstWeapon = 8,
	FirstAmulet = 21,
	FirstCurse = 37,
	None = 0xff
};
constexpr Icon weaponIcon(ItemKind weapon) {
	return static_cast<Icon>(static_cast<int>(Icon::FirstWeapon) + itemIndex(weapon));
}
constexpr Icon amuletIcon(AmuletType type) {
	return static_cast<Icon>(static_cast<int>(Icon::FirstAmulet) + static_cast<int>(type));
}

struct Slot {
	Icon icon = Icon::None; // None: an empty slot (no potion of that kind left)
	ui::Color tint = {1, 1, 1};
	int count = -1; // badge; < 0: none (the weapon)
	std::string badge; // badge text instead of the count (the amulet's tier, I .. IV)
	std::string key; // the key cap under the slot (from the bindings); empty: no cap (the amulet)
	int flashAgeMs = 1000000; // since the last use (a quick drink), for the flash
	// The weapon slot, a ranged weapon (plan ranged-weaker-than-melee): its shots, as pips once one is spent (full:
	// none), the next one filling up as it comes back (shotRecovery 0..1). A refused shot blinks the slot red (too
	// close) or the pips (none left).
	int shots = 0;
	int shotsLeft = 0;
	float shotRecovery = 0.f;
	int shotRefusedAgeMs = 1000000;
	bool shotTooClose = false;
};

struct View {
	int hp = 0, maxHp = 1;
	int stamina = 0, maxStamina = 1;
	int staminaRefusedAgeMs = 1000000; // since the last jump or sprint refused for lack of stamina
	float xpRatio = 0.f; // progress to the next level, 0..1
	int keysHeld = 0; // bit (colour - 1) per key held
	int levelKeys = 0; // bit (colour - 1) per key on the level: one socket each
	Slot slots[4]; // weapon, healing potion, stamina potion, worn amulet
	// The worn rings (plan ring-accessories), left and right hand: a gem each in its colour, under the key sockets.
	bool ringWorn[HAND_COUNT] = {};
	ui::Color ringGem[HAND_COUNT] = {};
	int poisonLeftMs[POISON_TIER_COUNT] = {}; // per tier, 0: not running. Any: the health bar is green.
	int resistLeftMs = 0; // a resistance potion's time left, 0: none
	bool resistGreater = false; // the greater one
	int waterLeftMs = 0; // the Blessing of Hapi's time left, 0: none (plan water-blessing-potion)
	int curseLeftMs[CURSE_COUNT] = {}; // per curse, 0: not running
	bool curseGreater[CURSE_COUNT] = {}; // at the greater grade
	int dazeLeftMs = 0; // a screech's daze, 0: none (plan screech-bite)
	// The stamina-skill sign (plan skill-flaming-arrows): shown once a stamina skill is learnt, lit while the toggle is
	// on, it blinks when a skill was refused for lack of stamina.
	bool staminaSkillShown = false;
	bool staminaSkillOn = false;
	int staminaSkillRefusedAgeMs = 1000000;
	std::string staminaSkillKey;
	bool hekaWaiting = false; // a heka to spend on a skill: a pulsing tile with the Heka key
	std::string hekaKey;
};

// Once a game tick: the health bar's lost part follows the player's health (it holds, then drains).
void tick(int hp, int maxHp);
// A new game or a loaded one: no lost part left over.
void reset();

// Sets its own square-pixel ortho projection (100 / SCALE high) for a resX x resY window. numbers: health numbers;
// small: key caps and counts; icons: the icon atlas texture; skillIcons: the skill atlas (the heka mark). Leaves
// texturing on and the HUD blend function (GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR) set.
void draw(const View& view, int resX, int resY, Font& numbers, Font& small, int icons, int skillIcons);
// One atlas icon (not None) filling `r`, in the canvas set up. Leaves texturing off.
void drawIcon(Icon icon, const ui::Rect& r, int icons, ui::Color tint);
} // namespace PlayerHud

#endif
