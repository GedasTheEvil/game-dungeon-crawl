#ifndef INVENTORY_H
#define INVENTORY_H
#include <string>
#include "../graphics/font.h"
#include "../entities/item.h"
#include "../world/item_bag.h"
#include "ui_draw.h"
#include <iosfwd>
#include <optional>

// The inventory screen: the weapons and potions in the ItemBag, with their models, details and buttons.
class Inventory {
  private:
	// Clickable things on the screen.
	enum class Target : unsigned char { None, Slot, UseButton, UpgradeButton };

	static constexpr int NO_SLOT = -1;

	ItemBag bag;
	int selectedSlot = 0; // slots are the ItemKind order
	int hoveredSlot = NO_SLOT;
	Target hoveredButton = Target::None;
	Target pressed = Target::None;
	int pressedSlot = NO_SLOT;
	int pressedMouseButton = 0;

	float slotAngle[ITEM_KIND_COUNT] = {}; // model turntable angle per slot
	int lastFrameMs = 0;

	ui::Toast toast; // feedback line after an action

	// Last quick drink (H / 0 in game), for the cooldown and the HUD slot flash.
	std::optional<int> quickDrinkMs;
	QuickKind quickDrinkKind = QuickKind::Health;

	Font title, heading, body, small;

	[[nodiscard]] static Item* Model(ItemKind kind);
	[[nodiscard]] bool CanUse(int slot, const char** reason) const;
	[[nodiscard]] bool CanUpgrade(int slot) const;
	// Mouse button down on the slot and still over it: the slot tile is pushed in.
	[[nodiscard]] bool SlotHeld(int slot) const;
	void Use(int slot);
	void Upgrade(int slot);
	std::string DrinkPotion(ItemKind potion); // applies it to the player; returns the feedback line
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
	Inventory();
	~Inventory();
	void Reset();				 // a new game: only the club
	void AddItem(ItemKind kind); // found: a weapon writes the journal's weapons note
	void Draw();
	void MouseFunction(int button, int state, int x, int y);
	void MouseMotion(int x, int y);
	void KeyPressed(unsigned char key);
	void SpecialKeyPressed(int key);
	// In game, the weapon keys (1-4): equips it if it was found, with the toast. A potion does nothing.
	void Equip(ItemKind weapon);
	// In game, H drinks a healing and 0 a stamina potion, the best fit (quickPotion). Full health / stamina or none
	// left: nothing is drunk, the status box says why.
	void QuickDrink(QuickKind kind);
	// The potion QuickDrink would drink now, ignoring whether anything is missing (the HUD slot); nullopt if none
	// left.
	[[nodiscard]] std::optional<ItemKind> QuickChoice(QuickKind kind) const;
	// Game clock time of the last quick drink of that kind, for the HUD flash.
	[[nodiscard]] std::optional<int> QuickDrinkMs(QuickKind kind) const;
	Item* Equipped() { return Model(bag.Equipped()); }
	[[nodiscard]] ItemKind EquippedKind() const { return bag.Equipped(); }
	[[nodiscard]] static ui::Color PotionColor(ItemKind potion); // tint of the shared potion model
	[[nodiscard]] int Count(ItemKind kind) const { return bag.Count(kind); }
	[[nodiscard]] int Level(ItemKind kind) const { return bag.Level(kind); }
	// Weapon damage with its level bonus.
	[[nodiscard]] int EquippedDamage() const;
	void Dump(std::ostream& f) const { bag.Save(f); }
	void LoadDump(std::istream& f);
};

#endif
