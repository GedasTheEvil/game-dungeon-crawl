#ifndef InventoryH
#define InventoryH

#include <string>
#include "../graphics/font.h"
#include "../entities/item.h"
#include "fstream"

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
constexpr int COUNT = 5;
} // namespace PotionId

// Inventory slots, in screen and save order: club, sword, spear, bow, then the five potions.
namespace InvSlot {
constexpr int FIRST_POTION = 4;
constexpr int COUNT = FIRST_POTION + PotionId::COUNT;
constexpr int NONE = -1;
} // namespace InvSlot

class inventory {
  private:
	// Clickable things on the screen.
	enum class Target { None, Slot, Button };

	int counts[InvSlot::COUNT] = {};
	int equippedSlot = 0;
	int selectedSlot = 0;
	int hoveredSlot = InvSlot::NONE;
	bool hoveredButton = false;
	Target pressed = Target::None;
	int pressedSlot = InvSlot::NONE;
	int pressedMouseButton = 0;

	float slotAngle[InvSlot::COUNT] = {}; // model turntable angle per slot
	int lastFrameMs = 0;

	std::string toast; // feedback line after an action
	int toastStartMs = 0;

	Font title, heading, body, small;

	[[nodiscard]] static item* SlotItem(int slot);
	[[nodiscard]] static int SlotFromItem(int type, int id);
	[[nodiscard]] bool CanUse(int slot, const char** reason) const;
	void Use(int slot);
	void DrinkPotion(int potionId);
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
	void DrawButton();
	void DrawStatus();
	void DrawFooter();

  public:
	bool show; // if true, show inventory
	inventory();
	~inventory();
	void GetItem(int type, int ID);
	void Draw();
	void MouseFunction(int button, int state, int x, int y);
	void MouseMotion(int x, int y);
	void KeyPressed(unsigned char key);
	void SpecialKeyPressed(int key);
	item* Equipped();
	[[nodiscard]] int EquippedType() const;
	[[nodiscard]] int EquippedId() const;
	[[nodiscard]] int Count(int type, int id) const;
	void Dump(std::ofstream& f);
	void LoadDump(std::ifstream& f);
};

#endif
