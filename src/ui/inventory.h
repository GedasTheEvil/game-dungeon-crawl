#ifndef INVENTORY_H
#define INVENTORY_H
#include <string>
#include "../graphics/font.h"
#include "../entities/item.h"
#include "../world/item_bag.h"
#include "ui_draw.h"
#include <iosfwd>
#include <optional>

// The inventory screen: the items in the ItemBag, one tab per ItemGroup, with their models, details and buttons.
class Inventory {
  private:
	// Clickable things on the screen.
	enum class Target : unsigned char { None, Slot, Tab, UseButton, UpgradeButton };

	static constexpr int NO_SLOT = -1;
	static constexpr int NO_TAB = -1;

	ItemBag bag;
	int selectedSlot = 0;				  // a slot is an ItemKind index; the grid shows them in tabOrder
	ItemGroup tab = ItemGroup::Weapons;	  // the selected slot's group
	int tabSlot[ITEM_GROUP_COUNT] = {};	  // the slot each tab selected last, for switching back to it
	int scrollRow[ITEM_GROUP_COUNT] = {}; // rows each tab is scrolled down
	int hoveredSlot = NO_SLOT;
	int hoveredTab = NO_TAB;
	Target hoveredButton = Target::None;
	Target pressed = Target::None;
	int pressedSlot = NO_SLOT; // or the tab, when pressed is Target::Tab
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
	[[nodiscard]] bool TabEnabled(ItemGroup group) const { return bag.AnyFound(group); }
	void SwitchTab(ItemGroup group); // to the slot it selected last
	[[nodiscard]] int Scroll() const { return scrollRow[static_cast<int>(tab)]; }
	void ScrollBy(int rows); // the open tab, within its rows
	void DrawScrollBar();
	void MoveSelection(int dx, int dy);
	void ShowToast(const std::string& text);
	void UpdateHover(float x, float y);

	void DrawBackground();
	void DrawTabs();
	void DrawTabHint();
	void DrawSlot(int slot);
	void DrawSlotModel(int slot);
	void DrawSlotLabels(int slot);
	void DrawDetails();
	void DrawDetailRules();
	void DrawDetailModel();
	void DrawButtons();
	void DrawButton(Target which, const char* label, bool enabled);
	void DrawStatus();
	void DrawFooter();

  public:
	Inventory();
	~Inventory();
	void Reset();				 // a new game: only the club
	void AddItem(ItemKind kind); // found: its journal notes (ItemBag::Find)
	void Draw();
	void MouseFunction(int button, int state, int x, int y);
	void MouseMotion(int x, int y);
	void KeyPressed(unsigned char key);
	// Selects the item and opens its tab, like a click on its slot (scenario `select`).
	void SelectItem(ItemKind kind) { Select(itemIndex(kind)); }
	void SpecialKeyPressed(int key);
	// In game, the class keys (1 melee, 2 ranged): equips the next weapon of that class held, in ItemKind order and
	// wrapping round, with the toast; the first one if the other class is in hand. None held: nothing.
	void EquipNext(bool ranged);
	// Equips the weapon like a click on its slot (scenario `equip`). False if it is not in hand afterwards (not
	// held, a potion).
	bool Equip(ItemKind weapon);
	// Puts the amulet on like a click on its slot, or takes the worn one off (nullopt) (scenario `wear`). False if
	// it is not held or not worn afterwards.
	bool Wear(std::optional<ItemKind> amulet);
	// In game, H drinks a healing and 0 a stamina potion, the best fit (quickPotion). Full health / stamina or none
	// left: nothing is drunk, the status box says why.
	void QuickDrink(QuickKind kind);
	// In game, = drinks an antidote. Not poisoned or none left: nothing is drunk, the status box says why.
	void QuickAntidote();
	// The potion QuickDrink would drink now, ignoring whether anything is missing (the HUD slot); nullopt if none
	// left.
	[[nodiscard]] std::optional<ItemKind> QuickChoice(QuickKind kind) const;
	// Game clock time of the last quick drink of that kind, for the HUD flash.
	[[nodiscard]] std::optional<int> QuickDrinkMs(QuickKind kind) const;
	Item* Equipped() { return Model(bag.Equipped()); }
	[[nodiscard]] ItemKind EquippedKind() const { return bag.Equipped(); }
	[[nodiscard]] static ui::Color PotionColor(ItemKind potion); // tint of the shared potion model
	[[nodiscard]] int Count(ItemKind kind) const { return bag.Count(kind); }
	ItemBag& Bag() { return bag; } // what the world adds to (SimLinks)
	[[nodiscard]] int Level(ItemKind kind) const { return bag.Level(kind); }
	// Weapon damage with its level bonus.
	[[nodiscard]] int EquippedDamage() const;
	void Dump(std::ostream& f) const { bag.Save(f); }
	void LoadDump(std::istream& f);
};

#endif
