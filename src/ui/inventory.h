#ifndef INVENTORY_H
#define INVENTORY_H
#include <string>
#include "../graphics/font.h"
#include "../entities/item.h"
#include "quick_potion.h"
#include "ui_draw.h"
#include "fstream"
#include <optional>

namespace ItemType {
constexpr int MELEE_WEAPON = 1;
constexpr int RANGED_WEAPON = 2;
constexpr int POTION = 3;
} // namespace ItemType

namespace WeaponId {
constexpr int CLUB = 0;
constexpr int SWORD = 1;
constexpr int SPEAR = 2;
constexpr int BOW = 0;
} // namespace WeaponId

namespace PotionId {
constexpr int SMALL_HEALTH = 0;
constexpr int LARGE_HEALTH = 1;
constexpr int STRENGTH = 2;
constexpr int ARMOR = 3;
constexpr int LIFE = 4;
constexpr int SMALL_STAMINA = 5;
constexpr int LARGE_STAMINA = 6;
constexpr int COUNT = 7;
} // namespace PotionId

// Inventory slots, in screen and save order: club, sword, spear, bow, then the potions.
namespace InvSlot {
constexpr int FIRST_POTION = 4;
constexpr int COUNT = FIRST_POTION + PotionId::COUNT;
constexpr int LEGACY_COUNT = 9; // saves from before the stamina potions and item levels
constexpr int NONE = -1;
} // namespace InvSlot

class Inventory {
  private:
	// Clickable things on the screen.
	enum class Target : unsigned char { None, Slot, UseButton, UpgradeButton };

	int counts[InvSlot::COUNT] = {};
	int levels[InvSlot::COUNT] = {}; // weapons only, from 1
	int equippedSlot = 0;
	int selectedSlot = 0;
	int hoveredSlot = InvSlot::NONE;
	Target hoveredButton = Target::None;
	Target pressed = Target::None;
	int pressedSlot = InvSlot::NONE;
	int pressedMouseButton = 0;

	float slotAngle[InvSlot::COUNT] = {}; // model turntable angle per slot
	int lastFrameMs = 0;

	std::string toast; // feedback line after an action
	int toastStartMs = 0;

	// Last quick drink (H / 0 in game), for the cooldown and the HUD slot flash.
	std::optional<int> quickDrinkMs;
	QuickKind quickDrinkKind = QuickKind::Health;

	Font title, heading, body, small;

	[[nodiscard]] static Item* SlotItem(int slot);
	[[nodiscard]] static int SlotFromItem(int type, int id);
	[[nodiscard]] bool CanUse(int slot, const char** reason) const;
	[[nodiscard]] bool CanUpgrade(int slot) const;
	// Mouse button down on the slot and still over it: the slot tile is pushed in.
	[[nodiscard]] bool SlotHeld(int slot) const;
	void Use(int slot);
	void Upgrade(int slot);
	std::string DrinkPotion(int potionId); // returns the feedback line
	void Select(int slot);
	void MoveSelection(int dx, int dy);
	void ShowToast(const std::string& text);
	void UpdateHover(float x, float y);

	void DrawBackground();
	void DrawSlot(int slot);
	void DrawSlotModel(int slot);
	void DrawSlotLabels(int slot);
	void DrawDetails();
	void DrawDetailModel();
	void DrawButtons();
	void DrawButton(Target which, const char* label, bool enabled);
	void DrawStatus();
	void DrawFooter();

  public:
	bool show; // if true, show inventory
	Inventory();
	~Inventory();
	void Reset(); // a new game: only the club
	void AddItem(int type, int id);
	void Draw();
	void MouseFunction(int button, int state, int x, int y);
	void MouseMotion(int x, int y);
	void KeyPressed(unsigned char key);
	void SpecialKeyPressed(int key);
	// In game, the number row equips a weapon (its slot hotkey); a potion hotkey or a missing weapon does nothing.
	void EquipHotkey(unsigned char key);
	// In game, H drinks a healing and 0 a stamina potion, the best fit (quickPotion). Full health / stamina or none
	// left: nothing is drunk, the status box says why.
	void QuickDrink(QuickKind kind);
	// The potion QuickDrink would drink now, ignoring whether anything is missing (the HUD slot); NO_POTION if none
	// left.
	[[nodiscard]] int QuickChoice(QuickKind kind) const;
	// Game clock time of the last quick drink of that kind, for the HUD flash.
	[[nodiscard]] std::optional<int> QuickDrinkMs(QuickKind kind) const;
	[[nodiscard]] static bool IsQuickHealKey(unsigned char key) { return key == 'h' || key == 'H'; }
	[[nodiscard]] static bool IsQuickStaminaKey(unsigned char key) { return key == '0'; }
	Item* Equipped();
	[[nodiscard]] static ui::Color PotionColor(int potionId); // tint of the shared potion model
	[[nodiscard]] int EquippedType() const;
	[[nodiscard]] int EquippedId() const;
	[[nodiscard]] int Count(int type, int id) const;
	[[nodiscard]] int Level(int type, int id) const;
	// Weapon damage with its level bonus.
	[[nodiscard]] int EquippedDamage() const;
	[[nodiscard]] static const char* ItemName(int type, int id);
	void Dump(std::ofstream& f);
	void LoadDump(std::ifstream& f);
};

#endif
