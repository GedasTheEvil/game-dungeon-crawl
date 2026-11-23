#ifndef INVENTORY_H
#define INVENTORY_H
#include "ui_screen.h"
#include <string>
#include "../render/font.h"
#include "../graphics/item.h"
#include "../level/item_bag.h"
#include "../level/player_actions.h"
#include "../state/item_commands.h"
#include "screen_world.h"
#include "../sim/dungeon_save.h"
#include "../level/bindings.h"
#include "../level/pointer.h"
#include "../render/ui_draw.h"
#include "confirm_box.h"
#include "../level/inventory_layout.h"
#include "../level/shrines.h"
#include <iosfwd>
#include <optional>

// The inventory screen: the items in the ItemBag, one tab per ItemGroup, with their models, details and buttons.
struct UIContext;

class Inventory final : public UiScreen {
  private:
	// Clickable things on the screen.
	enum class Target : unsigned char {
		None,
		Slot,
		Tab,
		SortButton,
		UseButton,
		UpgradeButton,
		OfferButton,
		MergeButton,
		Guard // a stats card guard cell (plan armor-split): only its tooltip
	};

	static constexpr int NO_SLOT = -1;
	static constexpr int NO_TAB = -1;
	static constexpr int NO_SORT = -1;

	const ScreenWorld& world; // the game it shows and changes (plan game-state-split)
	UIContext& ui; // the tab strip
	ItemBag& bag; // the player's items, GameState::bag (plan bag-out-of-inventory)
	ItemCommands& commands; // the actions on them (plan game-state-split)
	int selectedSlot = 0; // a slot is an ItemKind index; the grid shows them in tabOrder
	ItemGroup tab = ItemGroup::Weapons; // the selected slot's group
	int tabSlot[ITEM_GROUP_COUNT] = {}; // the slot each tab selected last, for switching back to it
	int scrollRow[ITEM_GROUP_COUNT] = {}; // rows each tab is scrolled down
	SortOrder sortOrder[ITEM_GROUP_COUNT] = {}; // per tab: found first at the start; saved with the tab (UiViewState)
	// The items the open screen used up (plan empty-slots-last: drunk, upgraded, offered, merged away): they keep
	// their place among the held ones until it closes.
	HeldItems usedUpHere{};
	bool reopened = true; // closed since the last Draw: the selection is brought back into view
	// A target and which one: the slot, the tab, or the SortOrder past Found of a sort button.
	struct Pick {
		Target target = Target::None;
		int index = -1;
		bool operator==(const Pick&) const = default;
	};
	ui::Pointer<Pick> pointer;
	ui::ScrollDrag scrollDrag; // the bar's thumb held (plan scrollbar-mouse-drag)

	float slotAngle[ITEM_KIND_COUNT] = {}; // model turntable angle per slot
	int lastFrameMs = 0;

	ui::Toast toast; // feedback line after an action
	ConfirmBox confirm; // "Offer the ... to Ptah?" over the screen (plan shrine-offerings)

	Font title, heading, body, small;
	int fontGeneration = -1; // ensureScreenFonts

	[[nodiscard]] Item* Model(ItemKind kind) const;
	[[nodiscard]] bool CanUse(int slot, const char** reason) const;
	[[nodiscard]] bool CanUpgrade(int slot) const;
	// The god whose shrine the player stands at (plan shrine-offerings): amulets upgrade, rings merge and spares are
	// offered only there.
	[[nodiscard]] std::optional<God> Shrine() const;
	[[nodiscard]] AtShrine AtOne() const { return Shrine() ? AtShrine::Yes : AtShrine::No; }
	// What a shrine would allow: an amulet's upgrade or a ring's merge (the slot's pulsing arrow, the Upgrade button
	// that says where to go).
	[[nodiscard]] bool ReadyAtShrine(int slot) const;
	[[nodiscard]] InventoryLayout::ShrineRow ShrineButtons() const; // the selected item's shrine row
	void AskOffer(int slot);
	void AskMerge(int slot);
	void Offer(ItemKind kind);
	void Merge(ItemKind ring);
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
	void Select(int slot);
	[[nodiscard]] bool TabEnabled(ItemGroup group) const { return bag.AnyFound(group); }
	void SwitchTab(ItemGroup group); // to the slot it selected last
	void NextTab(); // Tab: the next enabled tab, wrapping round
	[[nodiscard]] SortOrder Sort() const { return sortOrder[static_cast<int>(tab)]; }
	[[nodiscard]] HeldItems Held() const; // count > 0
	void KeepPlaces(const HeldItems& before); // the ones held before and none left now go to usedUpHere
	// The tab's grid order: tabOrder in its sort, the items in usedUpHere still held.
	[[nodiscard]] InventoryLayout::GridOrder Order(ItemGroup group) const;
	[[nodiscard]] InventoryLayout::GridOrder Order() const { return Order(tab); } // the open tab's
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
	[[nodiscard]] std::string DetailKind() const;
	void DrawPotionDetails(const ItemText& info);
	void DrawRingDetails(const ItemText& info);
	void DrawAmuletDetails(const ItemText& info);
	void DrawWeaponDetails();
	void DrawDetailRules();
	void DrawDetailModel();
	void DrawButtons();
	void DrawButton(Target which, const char* label, bool enabled);
	void DrawStatus();
	void DrawFooter();
	void DrawShrineButtons();

  public:
	Inventory(const ScreenWorld& world, UIContext& ui);
	~Inventory();
	void Reset(); // a new game: the first tabs, found first (ItemCommands::Reset empties the bag)
	// The open tab and the sort orders into / out of a save (plan save-remembers-ui-state). Setting opens the tab at
	// its first item, each tab sorted as saved; a tab with nothing found stays as it is.
	void ViewTo(UiViewState& view) const;
	void ViewFrom(const UiViewState& view);
	void Draw() override;
	// Once a tick while the screen is not open: the items it used up sort as empty again.
	void ForgetUsedUp() {
		usedUpHere = {};
		reopened = true;
	}
	void MouseFunction(int button, int state, int x, int y) override;
	void MouseMotion(int x, int y) override;
	void KeyPressed(unsigned char key) override;
	// Selects the item and opens its tab, like a click on its slot (scenario `select`).
	void SelectItem(ItemKind kind) { Select(itemIndex(kind)); }
	// The open tab's order (scenario `sort`); the selected item stays selected and in view.
	void SetSort(SortOrder order);
	[[nodiscard]] ItemGroup OpenTab() const { return tab; }
	// The selected item's place in its tab's grid, in the open order (scenario `expect selected`).
	[[nodiscard]] int SelectedPosition() const;
	void SpecialKeyPressed(int key) override;
	[[nodiscard]] static ui::Color PotionColor(ItemKind potion); // the potion icons' tint (HUD quick slot)
	[[nodiscard]] int EquippedDamage() const; // weapon damage with its level bonus
	void BagLoaded(); // a save game was read into the bag (dungeon_save.h): the selection follows it
	// The confirm box is open: it takes every key, Esc too (input.cpp).
	[[nodiscard]] bool Asking() const { return confirm.Open(); }
};

#endif
