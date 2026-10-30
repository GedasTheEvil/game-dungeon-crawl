#ifndef INVENTORY_H
#define INVENTORY_H
#include <string>
#include "../graphics/font.h"
#include "../entities/item.h"
#include "../world/item_bag.h"
#include "../input/bindings.h"
#include "pointer.h"
#include "ui_draw.h"
#include <iosfwd>
#include <optional>

// The inventory screen: the items in the ItemBag, one tab per ItemGroup, with their models, details and buttons.
class Inventory {
  private:
	// Clickable things on the screen.
	enum class Target : unsigned char { None, Slot, Tab, SortButton, UseButton, UpgradeButton };

	static constexpr int NO_SLOT = -1;
	static constexpr int NO_TAB = -1;
	static constexpr int NO_SORT = -1;

	ItemBag bag;
	int selectedSlot = 0; // a slot is an ItemKind index; the grid shows them in tabOrder
	ItemGroup tab = ItemGroup::Weapons; // the selected slot's group
	int tabSlot[ITEM_GROUP_COUNT] = {}; // the slot each tab selected last, for switching back to it
	int scrollRow[ITEM_GROUP_COUNT] = {}; // rows each tab is scrolled down
	SortOrder sortOrder[ITEM_GROUP_COUNT] = {}; // per tab, not saved: found first at the start
	// A target and which one: the slot, the tab, or the SortOrder past Found of a sort button.
	struct Pick {
		Target target = Target::None;
		int index = -1;
		bool operator==(const Pick&) const = default;
	};
	ui::Pointer<Pick> pointer;

	float slotAngle[ITEM_KIND_COUNT] = {}; // model turntable angle per slot
	int lastFrameMs = 0;

	ui::Toast toast; // feedback line after an action

	// Last quick drink (H / 0 in game), for the cooldown and the HUD slot flash.
	std::optional<int> quickDrinkMs;
	QuickKind quickDrinkKind = QuickKind::Health;
	// The weapon in hand when insanity put the club there (FollowInsanity); a load forgets it.
	std::optional<ItemKind> saneWeapon;

	Font title, heading, body, small;

	[[nodiscard]] static Item* Model(ItemKind kind);
	[[nodiscard]] bool CanUse(int slot, const char** reason) const;
	[[nodiscard]] bool CanUpgrade(int slot) const;
	// Mouse button down on the slot and still over it: the slot tile is pushed in.
	[[nodiscard]] bool SlotHeld(int slot) const { return pointer.Held({Target::Slot, slot}); }
	[[nodiscard]] bool SlotHovered(int slot) const { return pointer.hovered == Pick{Target::Slot, slot}; }
	// The slot, tab or sort order under the mouse, if it is over that kind of target; else -1.
	[[nodiscard]] int HoveredAt(Target target) const {
		return pointer.hovered.target == target ? pointer.hovered.index : -1;
	}
	void Use(int slot);
	void Upgrade(int slot);
	// The ring card's hand buttons (the Use and Upgrade buttons' places): on that hand, or off it.
	void WearOnHand(int slot, Hand hand);
	void ApplyRings(); // the worn rings' bonus to the player
	std::string DrinkPotion(ItemKind potion); // applies it to the player; returns the feedback line
	void Select(int slot);
	[[nodiscard]] bool TabEnabled(ItemGroup group) const { return bag.AnyFound(group); }
	void SwitchTab(ItemGroup group); // to the slot it selected last
	void NextTab(); // Tab: the next enabled tab, wrapping round
	[[nodiscard]] SortOrder Sort() const { return sortOrder[static_cast<int>(tab)]; }
	// A click on a sort button: that order for the open tab, or found first again if it was on.
	void ToggleSort(SortOrder order);
	// Two buttons under the details (use and upgrade): a weapon, or an amulet the spares can upgrade; a ring's left and
	// right hand.
	[[nodiscard]] bool TwoButtons() const;
	[[nodiscard]] int Scroll() const { return scrollRow[static_cast<int>(tab)]; }
	void ScrollBy(int rows); // the open tab, within its rows
	void DrawScrollBar();
	void MoveSelection(int dx, int dy);
	void Navigate(NavKey key); // the arrows and the bound move keys walk the grid, Enter / Space / Interact use
	void ShowToast(const std::string& text);
	void UpdateHover(float x, float y);

	void DrawBackground();
	void DrawTabs();
	void DrawSortButtons();
	void DrawHints();
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
	void Reset(); // a new game: only the club
	void AddItem(ItemKind kind); // found: its journal notes (ItemBag::Find)
	void Draw();
	void MouseFunction(int button, int state, int x, int y);
	void MouseMotion(int x, int y);
	void KeyPressed(unsigned char key);
	// Selects the item and opens its tab, like a click on its slot (scenario `select`).
	void SelectItem(ItemKind kind) { Select(itemIndex(kind)); }
	// The open tab's order (scenario `sort`); the selected item stays selected and in view.
	void SetSort(SortOrder order);
	[[nodiscard]] ItemGroup OpenTab() const { return tab; }
	// The selected item's place in its tab's grid, in the open order (scenario `expect selected`).
	[[nodiscard]] int SelectedPosition() const;
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
	// Puts the ring on that hand like its card's hand button (scenario `wear <ring> left|right`). False if it is not
	// held or not on that hand afterwards.
	bool WearRing(ItemKind ring, Hand hand);
	// In game, H drinks a healing and 0 a stamina potion, the best fit (quickPotion). Full health / stamina or none
	// left: nothing is drunk, the status box says why.
	void QuickDrink(QuickKind kind);
	// In game, = drinks an antidote. Not poisoned or none left: nothing is drunk, the status box says why.
	void QuickAntidote();
	// Once a tick: insanity's start puts the club in hand and remembers the weapon, its end gives the weapon back (plan
	// curses).
	void FollowInsanity(bool insane);
	// The potion QuickDrink would drink now, ignoring whether anything is missing (the HUD slot); nullopt if none
	// left.
	[[nodiscard]] std::optional<ItemKind> QuickChoice(QuickKind kind) const;
	// Game clock time of the last quick drink of that kind, for the HUD flash.
	[[nodiscard]] std::optional<int> QuickDrinkMs(QuickKind kind) const;
	Item* Equipped() { return Model(bag.Equipped()); }
	[[nodiscard]] ItemKind EquippedKind() const { return bag.Equipped(); }
	[[nodiscard]] static ui::Color PotionColor(ItemKind potion); // the potion icons' tint (HUD quick slot)
	[[nodiscard]] int Count(ItemKind kind) const { return bag.Count(kind); }
	ItemBag& Bag() { return bag; } // what the world adds to (SimLinks)
	[[nodiscard]] int Level(ItemKind kind) const { return bag.Level(kind); }
	// Weapon damage with its level bonus.
	[[nodiscard]] int EquippedDamage() const;
	void Dump(std::ostream& f) const { bag.Save(f); }
	void LoadDump(std::istream& f);
};

#endif
