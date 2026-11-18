#include "inventory.h"
#include "../level/player_stats.h"
#include "../level/player_actions.h"
#include "../sim/player_control.h"
#include "../graphics/item.h"
#include "../state/game_state.h"
#include "../core/logger.h"
#include "../core/timer.h"
#include "../level/input.h"
#include "../render/gl_includes.h"
#include "../render/lighting.h"
#include "../level/inventory_layout.h"
#include "../render/ui_draw.h"
#include "screen_tabs.h"
#include "player_hud.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <utility>
#include <vector>

// Layout works on a 160 x 100 canvas (y up) that keeps its aspect ratio and is centred on the window;
// the backdrop fills whatever the window adds around it.
namespace {

using namespace ui;
using namespace InventoryLayout;

constexpr Color HEALTH = {0.72f, 0.14f, 0.09f};
constexpr Color STAMINA = {0.78f, 0.68f, 0.16f};

constexpr float NAME_BAND_H = 4.4f;
constexpr float SLOT_SCALE = 14.5f;

constexpr const char* GROUP_NAMES[ITEM_GROUP_COUNT] = {"Weapons", "Potions", "Amulets", "Rings"};
constexpr const char* GUARD_LABELS[GUARD_COUNT] = {"Stab", "Cut", "Smash", "Fire"}; // GUARD_TYPES (plan armor-split)
constexpr PlayerHud::Icon GROUP_ICONS[ITEM_GROUP_COUNT] = {
	PlayerHud::weaponIcon(ItemKind::ShortSword),
	PlayerHud::Icon::Potion,
	PlayerHud::Icon::Amulet,
	PlayerHud::Icon::Ring
};

constexpr float WEAPON_DETAIL_SCALE = 22.f;
constexpr float AMULET_DETAIL_SCALE = 16.f;
constexpr float RING_DETAIL_SCALE = 15.f;
constexpr const char* HAND_MARKS[HAND_COUNT] = {"L", "R"}; // a worn ring's slot
constexpr float PLINTH_Y = 49.f; // detail model base
// The models' light (plan metallic-shading): a bright ambient, so matte items look as flat as on the page, and one
// far light above, left and in front, so metal catches a highlight as it turns. Far, so every slot gets the same.
constexpr float MODEL_AMBIENT = 0.78f;
constexpr Lighting::LightDef MODEL_LIGHT = {0.68f, 0.64f, 0.58f, 3000.f, 0.f};
constexpr float MODEL_LIGHT_POS[3] = {-300.f, 400.f, 600.f}; // from the area's centre
constexpr float REST_ANGLE = 25.f; // degrees, idle slots show the model a little turned
constexpr float SPIN_DEG_PER_MS = 0.09f; // hovered / selected models

constexpr float SCROLL_BAR_W = 1.f;

// The sort buttons' faces (their places: inventory_layout.h).
constexpr const char* SORT_LABELS[SORT_ORDER_COUNT] = {"", "a-z", "*", "new"};
constexpr const char* SORT_HINTS[SORT_ORDER_COUNT] = {"", "Sort by name", "Strongest first", "Last found first"};

ItemKind kindOf(int slot) { return itemAt(slot); }
bool isPotion(int slot) { return ::isPotion(kindOf(slot)); }
bool isWeapon(int slot) { return ::isWeapon(kindOf(slot)); }
bool isAmulet(int slot) { return ::isAmulet(kindOf(slot)); }
bool isRing(int slot) { return ::isRing(kindOf(slot)); }
ItemGroup groupOf(int slot) { return itemGroup(kindOf(slot)); }
ItemGroup groupAt(int index) { return static_cast<ItemGroup>(index); }

Color potionColor(ItemKind potion) {
	const Rgb c = potionDef(potion).colour;
	return {c.r, c.g, c.b};
}

const char* blockReason(UseBlock block) {
	switch (block) {
	case UseBlock::Dead:
		return "You are dead";
	case UseBlock::NotFound:
		return "Not found yet";
	case UseBlock::NoneLeft:
		return "None left";
	case UseBlock::Equipped:
		return "Equipped";
	case UseBlock::HealthFull:
		return "Health is full";
	case UseBlock::StaminaFull:
		return "Stamina is full";
	case UseBlock::NotPoisoned:
		return "You are not poisoned";
	case UseBlock::StrongerResistance:
		return "A stronger resistance runs";
	case UseBlock::None:
		break;
	}
	return "";
}

// A worn ring's hand badge in its slot `r`, over the name band: the left hand's in the left corner, the right's in the
// right one, clear of the ring.
Rect handBadge(const Rect& r, int hand) {
	constexpr float W = 4.1f;
	return {hand == 0 ? r.x + 0.5f : r.x + r.w - W - 0.5f, r.y + NAME_BAND_H + 0.5f, W, 3.5f};
}

// The question mark of an item not found yet, centred in `r`.
void unknownMark(Font& font, const Rect& r, float alpha) { textCenteredIn(font, r, "?", {0.45f, 0.42f, 0.38f}, alpha); }

Rect visibleArea(const RenderSettings& render) { return ui::visibleArea(CANVAS_W, CANVAS_H, render.resX, render.resY); }

void toCanvas(const RenderSettings& render, int mouseX, int mouseY, float& x, float& y) {
	ui::toCanvas(visibleArea(render), render.resX, render.resY, mouseX, mouseY, x, y);
}

} // namespace

Inventory::Inventory(const ScreenWorld& world, UIContext& ui)
	: world(world), ui(ui), bag(world.bag), commands(world.items) {
	loadScreenFonts(title, heading, body, small, 7.f);

	Reset();
	for (float& angle : slotAngle)
		angle = REST_ANGLE;
}

void Inventory::Reset() {
	for (int group = 0; group < ITEM_GROUP_COUNT; group++) {
		tabSlot[group] = groupItems(groupAt(group)).first;
		sortOrder[group] = SortOrder::Found;
	}
	Select(itemIndex(ItemKind::Club));
	toast = ui::Toast{};
}

void Inventory::ViewTo(UiViewState& view) const {
	view.tab = tab;
	for (int group = 0; group < ITEM_GROUP_COUNT; group++)
		view.sort[static_cast<size_t>(group)] = sortOrder[group];
}

void Inventory::ViewFrom(const UiViewState& view) {
	for (int group = 0; group < ITEM_GROUP_COUNT; group++) {
		sortOrder[group] = view.sort[static_cast<size_t>(group)];
		tabSlot[group] = groupItems(groupAt(group)).first; // the bag can differ: each tab starts at the top
		scrollRow[group] = 0;
	}
	if (TabEnabled(view.tab))
		SwitchTab(view.tab);
	else
		Select(selectedSlot);
}

Inventory::~Inventory() {}

Item* Inventory::Model(ItemKind kind) const { return world.assets.items.Of(kind); }

int Inventory::EquippedDamage() const {
	return weaponDamage(bag.Equipped(), weaponDef(bag.Equipped()).damage, bag.Level(bag.Equipped()));
}

// ---- actions ---------------------------------------------------------------

bool Inventory::CanUse(int slot, const char** reason) const {
	UseBlock block = bag.Block(kindOf(slot), bagUser(world.player));
	if (reason != nullptr)
		*reason = block != UseBlock::None ? blockReason(block)
				  : isPotion(slot)		  ? "Drink"
				  : isAmulet(slot)		  ? (bag.Worn() == kindOf(slot) ? "Take off" : "Wear")
				  : isRing(slot)		  ? (bag.RingOn(Hand::Left) == kindOf(slot) ? "Take off" : "Left hand")
										  : "Equip";
	return block == UseBlock::None;
}

bool Inventory::CanUpgrade(int slot) const { return bag.CanUpgrade(kindOf(slot), world.player.Alive(), AtOne()); }

std::optional<God> Inventory::Shrine() const { return world.dungeon.ShrineHere(); }

bool Inventory::ReadyAtShrine(int slot) const {
	const bool alive = world.player.Alive();
	return isRing(slot) ? bag.CanMerge(kindOf(slot), alive, AtShrine::Yes)
						: bag.CanUpgrade(kindOf(slot), alive, AtShrine::Yes);
}

ShrineRow Inventory::ShrineButtons() const {
	if (!Shrine() || !bag.Found(kindOf(selectedSlot)))
		return ShrineRow::None;
	return isRing(selectedSlot) ? ShrineRow::OfferMerge : ShrineRow::Offer;
}

// "a spare Club", "the Lesser Amulet of Armor", "the Copper Garnet Ring"
void Inventory::AskOffer(int slot) {
	const ItemKind kind = kindOf(slot);
	const std::optional<God> god = Shrine();
	if (!god || !bag.CanOffer(kind, world.player.Alive(), AtShrine::Yes))
		return;
	const std::string what = (isWeapon(slot)   ? "a spare "
							  : isPotion(slot) ? "a "
											   : "the ") +
							 std::string(itemText(kind).name);
	const int xp = offerXP(kind, world.player.stats.CurrentXP());
	confirm.Ask(
		"Offer " + what + " to " + godDef(*god).name + " for " + std::to_string(xp) + " XP? It is gone for good.",
		[this, kind] { Offer(kind); }
	);
}

void Inventory::AskMerge(int slot) {
	const ItemKind ring = kindOf(slot);
	const std::optional<ItemKind> next = nextMetal(ring);
	if (!next || !bag.CanMerge(ring, world.player.Alive(), AtOne()))
		return;
	std::string two = itemText(ring).name; // "Copper Ring of Reach" -> "Copper Rings of Reach"
	if (const size_t at = two.find("Ring"); at != std::string::npos)
		two.insert(at + 4, "s");
	confirm.Ask("Merge two " + two + " into a " + itemText(*next).name + "?", [this, ring] { Merge(ring); });
}

void Inventory::Offer(ItemKind kind) {
	const std::optional<God> god = Shrine();
	if (!god)
		return;
	const HeldItems before = Held();
	PlayerActor actor = commands.Actor();
	const ActionResult offered = offerItem(actor, kind, *god, world.dungeon.LevelNumber(), world.alwaysGift);
	if (!offered.done)
		return;
	KeepPlaces(before);
	ShowToast(offered.text);
}

void Inventory::Merge(ItemKind ring) {
	const std::optional<God> god = Shrine();
	if (!god)
		return;
	const HeldItems before = Held();
	PlayerActor actor = commands.Actor();
	const ActionResult merged = mergeRings(actor, ring, *god);
	if (!merged.done)
		return;
	KeepPlaces(before);
	if (merged.select)
		Select(itemIndex(*merged.select));
	ShowToast(merged.text);
}

void Inventory::WearOnHand(int slot, Hand hand) {
	PlayerActor actor = commands.Actor();
	if (const ActionResult worn = wearRingOn(actor, kindOf(slot), hand); worn.done)
		ShowToast(worn.text);
}

void Inventory::Upgrade(int slot) {
	const ItemKind kind = kindOf(slot);
	if (::isAmulet(kind) && !Shrine() && ReadyAtShrine(slot)) {
		ShowToast("Only a god can work this magic: upgrade it at a shrine");
		return;
	}
	const HeldItems before = Held();
	PlayerActor actor = commands.Actor();
	const ActionResult upgraded = upgradeItem(actor, kind, AtOne());
	if (!upgraded.done)
		return;
	KeepPlaces(before);
	if (upgraded.select)
		Select(itemIndex(*upgraded.select));
	ShowToast(upgraded.text);
}

void Inventory::Use(int slot) {
	const HeldItems before = Held();
	const ActionResult used = commands.Use(kindOf(slot));
	if (!used.done)
		return;
	if (::isPotion(kindOf(slot)))
		KeepPlaces(before);
	if (!used.text.empty())
		ShowToast(used.text);
}

Color Inventory::PotionColor(ItemKind potion) { return potionColor(potion); }

void Inventory::Select(int slot) {
	if (slot < 0 || slot >= ITEM_KIND_COUNT)
		return;
	selectedSlot = slot;
	tab = groupOf(slot);
	tabSlot[static_cast<int>(tab)] = slot;
	int& scroll = scrollRow[static_cast<int>(tab)]; // the selected slot in view
	const int row = rowOf(Order(), slot);
	scroll = std::clamp(scroll, row - VISIBLE_ROWS + 1, row);
}

void Inventory::SetSort(SortOrder order) {
	sortOrder[static_cast<int>(tab)] = order;
	Select(selectedSlot);
}

void Inventory::ToggleSort(SortOrder order) { SetSort(Sort() == order ? SortOrder::Found : order); }

int Inventory::SelectedPosition() const { return positionOf(Order(), selectedSlot); }

HeldItems Inventory::Held() const {
	HeldItems held{};
	for (int i = 0; i < ITEM_KIND_COUNT; i++)
		held[static_cast<size_t>(i)] = bag.Count(itemAt(i)) > 0;
	return held;
}

void Inventory::KeepPlaces(const HeldItems& before) {
	const HeldItems now = Held();
	for (size_t i = 0; i < now.size(); i++)
		usedUpHere[i] = usedUpHere[i] || (before[i] && !now[i]);
}

GridOrder Inventory::Order(ItemGroup group) const {
	HeldItems held = Held();
	for (size_t i = 0; i < held.size(); i++)
		held[i] = held[i] || usedUpHere[i];
	return tabOrder(bag, group, sortOrder[static_cast<int>(group)], held);
}

bool Inventory::TwoButtons() const {
	return isWeapon(selectedSlot) || isRing(selectedSlot) || (isAmulet(selectedSlot) && ReadyAtShrine(selectedSlot));
}

void Inventory::ScrollBy(int rows) {
	int& scroll = scrollRow[static_cast<int>(tab)];
	scroll = std::clamp(scroll + rows, 0, std::max(0, rowsOf(tab) - VISIBLE_ROWS));
}

void Inventory::SwitchTab(ItemGroup group) {
	if (!TabEnabled(group))
		return;
	const int last = tabSlot[static_cast<int>(group)];
	// A new game: the first found.
	Select(bag.Found(kindOf(last)) ? last : itemIndex(Order(group).front()));
}

void Inventory::NextTab() {
	for (int step = 1; step < ITEM_GROUP_COUNT; step++) {
		const ItemGroup next = groupAt((static_cast<int>(tab) + step) % ITEM_GROUP_COUNT);
		if (TabEnabled(next)) {
			SwitchTab(next);
			return;
		}
	}
}

// Arrow keys stay in the open tab: left / right walk its slots in the grid's order, up / down move a row (down onto a
// shorter last row: its last slot).
void Inventory::MoveSelection(int dx, int dy) {
	const GridOrder order = Order();
	const int count = static_cast<int>(order.size());
	const int position = positionOf(order, selectedSlot);
	int target = position + dx;
	if (dy > 0)
		target = position - COLUMNS;
	else if (dy < 0 && position / COLUMNS < (count - 1) / COLUMNS)
		target = std::min(position + COLUMNS, count - 1);
	if (target >= 0 && target < count)
		Select(itemIndex(order[static_cast<size_t>(target)]));
}

void Inventory::ShowToast(const std::string& text) { toast.Show(text, GameClock::now()); }

// ---- input -----------------------------------------------------------------

void Inventory::UpdateHover(float x, float y) {
	const Hit hit = hitAt(Order(), Scroll(), TwoButtons(), ShrineButtons(), x, y);
	pointer.Move(
		hit.slot != NO_HIT				? Pick{Target::Slot, hit.slot}
		: hit.tab != NO_HIT				? Pick{Target::Tab, hit.tab}
		: hit.sort != NO_HIT			? Pick{Target::SortButton, hit.sort}
		: hit.guard != NO_HIT			? Pick{Target::Guard, hit.guard}
		: hit.button == Button::Use		? Pick{Target::UseButton}
		: hit.button == Button::Upgrade ? Pick{Target::UpgradeButton}
		: hit.button == Button::Offer	? Pick{Target::OfferButton}
		: hit.button == Button::Merge	? Pick{Target::MergeButton}
										: Pick{}
	);
}

void Inventory::MouseMotion(int x, int y) {
	float cx = 0.f;
	float cy = 0.f;
	toCanvas(world.render, x, y, cx, cy);
	if (confirm.Open()) {
		confirm.Move(cx, cy);
		return;
	}
	UpdateHover(cx, cy);
}

// Left click selects a slot (on press) or presses a button (fires on release over it).
// Right click on a slot selects and uses it straight away.
void Inventory::MouseFunction(int button, int state, int x, int y) {
	float cx = 0.f;
	float cy = 0.f;
	toCanvas(world.render, x, y, cx, cy);
	if (confirm.Open()) {
		confirm.Button(button, state == MOUSE_DOWN, cx, cy);
		if (!confirm.Open())
			pointer.Clear();
		return;
	}
	UpdateHover(cx, cy);

	if (button == MOUSE_WHEEL_UP || button == MOUSE_WHEEL_DOWN) {
		if (state == MOUSE_DOWN)
			ScrollBy(button == MOUSE_WHEEL_UP ? -1 : 1);
		UpdateHover(cx, cy);
		return;
	}

	const Pick at = pointer.hovered;
	if (state == MOUSE_DOWN) {
		// Any button on a slot; only the left one on the rest, a dimmed tab takes none.
		const bool takes = at.target == Target::Slot || (button == MOUSE_LEFT_BUTTON && at.target != Target::Guard &&
														 (at.target != Target::Tab || TabEnabled(groupAt(at.index))));
		pointer.Down(takes ? at : Pick{}, button);
		if (at.target == Target::Slot)
			Select(at.index);
		return;
	}
	if (state != MOUSE_UP)
		return;

	const Pick fired = pointer.Up(at, button);
	switch (fired.target) {
	case Target::UseButton:
		if (isRing(selectedSlot))
			WearOnHand(selectedSlot, Hand::Left);
		else
			Use(selectedSlot);
		return;
	case Target::UpgradeButton:
		if (isRing(selectedSlot))
			WearOnHand(selectedSlot, Hand::Right);
		else
			Upgrade(selectedSlot);
		return;
	case Target::OfferButton:
		AskOffer(selectedSlot);
		return;
	case Target::MergeButton:
		AskMerge(selectedSlot);
		return;
	case Target::Tab:
		SwitchTab(groupAt(fired.index));
		return;
	case Target::SortButton:
		ToggleSort(static_cast<SortOrder>(fired.index));
		return;
	case Target::Slot:
		if (button == MOUSE_RIGHT_BUTTON)
			Use(fired.index);
		return;
	case Target::Guard:
	case Target::None:
		return;
	}
}

void Inventory::KeyPressed(unsigned char key) {
	if (confirm.Open()) {
		if (key == KEY_ESCAPE)
			confirm.Cancel();
		else
			confirm.Navigate(navKeyOf(InputKey::Char(key), world.settings.controls));
		if (!confirm.Open())
			pointer.Clear();
		return;
	}
	if (const NavKey nav = navKeyOf(InputKey::Char(key), world.settings.controls); nav != NavKey::None) {
		Navigate(nav);
		return;
	}
	if ((key == 'u' || key == 'U') && isRing(selectedSlot))
		AskMerge(selectedSlot); // a ring's upgrade: the merge (plan shrine-offerings)
	else if (key == 'u' || key == 'U')
		Upgrade(selectedSlot);
	else if (key == 'o' || key == 'O')
		AskOffer(selectedSlot);
	else if (key == KEY_TAB)
		NextTab();
	// the number keys do nothing here: picking is by mouse and the move keys
}

void Inventory::Navigate(NavKey key) {
	switch (key) {
	case NavKey::Activate:
		Use(selectedSlot);
		return;
	case NavKey::Left:
		MoveSelection(-1, 0);
		return;
	case NavKey::Right:
		MoveSelection(1, 0);
		return;
	case NavKey::Up:
		MoveSelection(0, 1);
		return;
	case NavKey::Down:
		MoveSelection(0, -1);
		return;
	case NavKey::None:
		return;
	}
}

void Inventory::SpecialKeyPressed(int key) {
	const NavKey nav = navKeyOf(InputKey::Special(key), world.settings.controls);
	if (!confirm.Open()) {
		Navigate(nav);
		return;
	}
	confirm.Navigate(nav);
	if (!confirm.Open())
		pointer.Clear();
}

// ---- drawing ---------------------------------------------------------------

void Inventory::Draw() {
	if (std::exchange(reopened, false))
		Select(selectedSlot); // a used-up item may have moved (plan empty-slots-last)
	int now = GameClock::now();
	int elapsed = now - lastFrameMs;
	lastFrameMs = now;
	if (elapsed < 0 || elapsed > 100)
		elapsed = 0;
	for (int slot = 0; slot < ITEM_KIND_COUNT; slot++)
		if (slot == selectedSlot || SlotHovered(slot))
			slotAngle[slot] = std::fmod(slotAngle[slot] + SPIN_DEG_PER_MS * static_cast<float>(elapsed), 360.f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	Rect area = visibleArea(world.render);
	glOrtho(area.x, area.x + area.w, area.y, area.y + area.h, -200, 200);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);

	// The slots of the rows in view, in the grid's order.
	const GridOrder order = Order();
	constexpr size_t SHOWN = static_cast<size_t>(COLUMNS) * VISIBLE_ROWS;
	std::vector<int> shown;
	for (size_t p = static_cast<size_t>(Scroll()) * COLUMNS; p < order.size() && shown.size() < SHOWN; p++)
		shown.push_back(itemIndex(order[p]));

	DrawBackground();
	DrawTabs();
	DrawSortButtons();
	for (int slot : shown)
		DrawSlot(slot);
	DrawDetails();
	DrawButtons();
	DrawStatus();
	DrawScrollBar();

	// Models get their own depth buffer so they never cut into the flat UI drawn before them.
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	Lighting::setAtmosphere({{MODEL_AMBIENT, MODEL_AMBIENT, MODEL_AMBIENT}});
	Lighting::begin();
	Lighting::add(area.cx() + MODEL_LIGHT_POS[0], area.cy() + MODEL_LIGHT_POS[1], MODEL_LIGHT_POS[2], MODEL_LIGHT, 0);
	Lighting::commit();
	for (int slot : shown)
		DrawSlotModel(slot);
	DrawDetailModel();
	Lighting::end();
	glDisable(GL_DEPTH_TEST);

	beginText();
	for (int slot : shown)
		DrawSlotLabels(slot);
	DrawHints();
	DrawFooter();
	ScreenTabs::Draw(ui);
	if (confirm.Open())
		confirm.Draw(heading, body, small, area);

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glColor3f(1, 1, 1);
}

// Right of the grid, only for a tab with more rows than fit: the track over the rows shown, the thumb where they are.
void Inventory::DrawScrollBar() {
	const int rows = rowsOf(tab);
	if (rows <= VISIBLE_ROWS)
		return;
	const float top = TOP_ROW_Y + SLOT_H;
	const float bottom = TOP_ROW_Y - (VISIBLE_ROWS - 1) * (SLOT_H + SLOT_GAP);
	const float x = ITEMS_PANEL.cx() + GRID_W / 2 +
					(ITEMS_PANEL.x + ITEMS_PANEL.w - ITEMS_PANEL.cx() - GRID_W / 2) / 2 - SCROLL_BAR_W / 2;
	fillRect({x, bottom, SCROLL_BAR_W, top - bottom}, {0.09f, 0.07f, 0.05f}, {0.06f, 0.045f, 0.03f}, 1.f);
	const float rowH = (top - bottom) / static_cast<float>(rows);
	const float thumbTop = top - rowH * static_cast<float>(Scroll());
	fillRect({x, thumbTop - rowH * VISIBLE_ROWS, SCROLL_BAR_W, rowH * VISIBLE_ROWS}, GOLD, GOLD_DIM, 1.f);
}

void Inventory::DrawBackground() {
	backdrop(visibleArea(world.render), world.assets.textures.loadingBackground.ID());
	titleBar(title, CANVAS_W / 2, "Inventory", SCREEN_TABS_TITLE_REACH);

	panel(ITEMS_PANEL, 0.9f);

	// Papyrus scroll for the details, in a frame matching the items panel.
	glEnable(GL_TEXTURE_2D);
	world.assets.textures.papyrus.Bind();
	glColor4f(1, 1, 1, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0.04f, 0.07f);
	glVertex2f(DETAIL_PANEL.x, DETAIL_PANEL.y);
	glTexCoord2f(0.96f, 0.07f);
	glVertex2f(DETAIL_PANEL.x + DETAIL_PANEL.w, DETAIL_PANEL.y);
	glTexCoord2f(0.96f, 0.93f);
	glVertex2f(DETAIL_PANEL.x + DETAIL_PANEL.w, DETAIL_PANEL.y + DETAIL_PANEL.h);
	glTexCoord2f(0.04f, 0.93f);
	glVertex2f(DETAIL_PANEL.x, DETAIL_PANEL.y + DETAIL_PANEL.h);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	strokeRect(DETAIL_PANEL, BRONZE, 1.f, 3.f);
	diamond(DETAIL_PANEL.x, DETAIL_PANEL.y, 1.2f, GOLD, 1.f);
	diamond(DETAIL_PANEL.x + DETAIL_PANEL.w, DETAIL_PANEL.y, 1.2f, GOLD, 1.f);
	diamond(DETAIL_PANEL.x + DETAIL_PANEL.w, DETAIL_PANEL.y + DETAIL_PANEL.h, 1.2f, GOLD, 1.f);
	diamond(DETAIL_PANEL.x, DETAIL_PANEL.y + DETAIL_PANEL.h, 1.2f, GOLD, 1.f);
}

// One tile per group: its icon and name. The open one lapis, the others stone; a group with nothing found yet is
// dimmed and does not take clicks.
void Inventory::DrawTabs() {
	const int icons = world.assets.textures.hudIcons.ID();
	for (int group = 0; group < ITEM_GROUP_COUNT; group++) {
		const bool enabled = TabEnabled(groupAt(group));
		const bool active = groupAt(group) == tab;
		const bool hovered = enabled && !active && HoveredAt(Target::Tab) == group;
		const bool held = hovered && pointer.Held({Target::Tab, group});
		TileStyle style = !enabled ? TileStyle::Disabled : (active ? TileStyle::Lapis : TileStyle::Stone);
		Rect r = tile(tabRect(group), style, hovered, held);

		Color tint = groupAt(group) == ItemGroup::Potions ? potionColor(ItemKind::SmallHealth) : Color{1, 1, 1};
		if (!enabled)
			tint = {0.3f, 0.26f, 0.22f};
		PlayerHud::drawIcon(GROUP_ICONS[group], {r.x + 0.6f, r.y + 0.3f, r.h - 0.6f, r.h - 0.6f}, icons, tint);
		beginText();
		Color c = !enabled ? Color{0.36f, 0.29f, 0.20f} : (hovered ? TEXT_HOVER : GOLD);
		textMiddle(small, r.x + r.h + 0.6f, r.cy(), GROUP_NAMES[group], c);
		beginShapes();
	}
}

// Small tiles like the tabs, the open tab's order lapis; a click on the lit one goes back to found first.
void Inventory::DrawSortButtons() {
	for (int order = 1; order < SORT_ORDER_COUNT; order++) {
		const bool active = static_cast<int>(Sort()) == order;
		const bool hovered = HoveredAt(Target::SortButton) == order;
		const bool held = hovered && pointer.Held({Target::SortButton, order});
		const Rect r = tile(sortRect(order), active ? TileStyle::Lapis : TileStyle::Stone, hovered, held);
		if (static_cast<SortOrder>(order) == SortOrder::Strength) { // the fonts' '*' is a mere dot
			diamond(r.cx(), r.cy(), 0.9f, hovered ? TEXT_HOVER : GOLD, 1.f);
			continue;
		}
		beginText();
		textCenteredIn(small, r, SORT_LABELS[order], hovered ? TEXT_HOVER : GOLD);
		beginShapes();
	}
}

// Under a hovered dimmed tab: why it does not open; under a hovered sort button: its order.
void Inventory::DrawHints() {
	char hint[40] = "";
	Rect anchor{};
	const int hoveredTab = HoveredAt(Target::Tab);
	const int hoveredSort = HoveredAt(Target::SortButton);
	if (hoveredTab != NO_TAB && !TabEnabled(groupAt(hoveredTab))) {
		snprintf(hint, sizeof(hint), "%s: none yet", GROUP_NAMES[hoveredTab]);
		anchor = tabRect(hoveredTab);
	} else if (hoveredSort != NO_SORT) {
		const bool active = static_cast<int>(Sort()) == hoveredSort;
		snprintf(hint, sizeof(hint), "%s", active ? "Back to found first" : SORT_HINTS[hoveredSort]);
		anchor = sortRect(hoveredSort);
	} else {
		return;
	}
	const float w = small.TextWidth(hint) + 3.f;
	const Rect label = {std::min(anchor.cx() - w / 2, ITEMS_PANEL.x + ITEMS_PANEL.w - 2 - w), anchor.y - 5.4f, w, 4.4f};
	beginShapes();
	fillRect(label, PANEL_TOP, PANEL_BOTTOM, 0.95f);
	strokeRect(label, GOLD_DIM, 1.f, 1.f);
	beginText();
	textMiddle(small, label.x + 1.5f, label.cy(), hint, GOLD);
}

void Inventory::DrawSlot(int slot) {
	Rect r = slotRect(Order(), slot, Scroll());
	bool hovered = SlotHovered(slot);
	bool selected = slot == selectedSlot;
	bool held = SlotHeld(slot);
	bool owned = bag.Count(kindOf(slot)) > 0;

	if (selected) {
		// Soft gold halo that breathes.
		float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GameClock::now()) * 0.004f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		ring(r, 2.6f, GOLD, 0.35f + 0.25f * pulse, 0.f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	r = tile(r, TileStyle::Stone, hovered || selected, held);

	// Name band: lapis for the weapon in hand, dark stone otherwise.
	Rect band = {r.x, r.y, r.w, NAME_BAND_H};
	// the weapon in hand, the amulet on, a ring on
	if (kindOf(slot) == bag.Equipped() || kindOf(slot) == bag.Worn() || bag.RingWorn(kindOf(slot)))
		fillRect(band, LAPIS, LAPIS_DARK, 1.f);
	else
		fillRect(band, {0.09f, 0.07f, 0.05f}, {0.06f, 0.045f, 0.03f}, 1.f);
	line(r.x, r.y + NAME_BAND_H, r.x + r.w, r.y + NAME_BAND_H, selected ? GOLD : BRONZE, 1.f, 1.f);

	if (!owned) // unfound items stay in shadow
		fillRect(r, BLACK, BLACK, 0.35f);

	if (selected) {
		strokeRect(r, GOLD, 1.f, 3.f);
		strokeRect(r.inset(0.9f), GOLD, 0.45f, 1.f);
	} else {
		tileFrame(r, TileStyle::Stone, hovered);
	}

	// Count badge for stacks.
	if (!isWeapon(slot) && owned) {
		Rect badge = {r.x + r.w - 4.6f, r.y + r.h - 4.f, 4.1f, 3.5f};
		fillRect(badge, {0.05f, 0.04f, 0.03f}, {0.05f, 0.04f, 0.03f}, 0.85f);
		strokeRect(badge, GOLD_DIM, 1.f, 1.f);
	}

	// A worn ring: a badge per hand it is on (its letter: DrawSlotLabels).
	for (int hand = 0; hand < HAND_COUNT; hand++)
		if (bag.RingOn(static_cast<Hand>(hand)) == kindOf(slot)) {
			const Rect badge = handBadge(r, hand);
			fillRect(badge, LAPIS, LAPIS_DARK, 0.95f);
			strokeRect(badge, GOLD_DIM, 1.f, 1.f);
		}

	// Upgrade ready (an amulet's or a ring's at a shrine): a pulsing gold arrow under the level.
	if (ReadyAtShrine(slot)) {
		float pulse = 0.6f + 0.4f * std::sin(static_cast<float>(GameClock::now()) * 0.006f);
		float ax = r.x + r.w - 2.8f;
		float ay = r.y + r.h - 7.8f;
		glColor4f(GOLD.r, GOLD.g, GOLD.b, pulse);
		glBegin(GL_TRIANGLES);
		glVertex2f(ax - 1.5f, ay);
		glVertex2f(ax + 1.5f, ay);
		glVertex2f(ax, ay + 2.f);
		glEnd();
		glBegin(GL_QUADS);
		glVertex2f(ax - 0.6f, ay - 1.4f);
		glVertex2f(ax + 0.6f, ay - 1.4f);
		glVertex2f(ax + 0.6f, ay);
		glVertex2f(ax - 0.6f, ay);
		glEnd();
	}
}

void Inventory::DrawSlotModel(int slot) {
	if (!bag.Found(kindOf(slot)))
		return; // a question mark instead (DrawSlotLabels)
	Rect r = slotRect(Order(), slot, Scroll());
	bool held = SlotHeld(slot);
	Item* model = Model(kindOf(slot));

	Color tint = {1, 1, 1};
	if (bag.Count(kindOf(slot)) <= 0)
		tint = {0.07f, 0.055f, 0.04f};
	else if (slot != selectedSlot && !SlotHovered(slot))
		tint = {tint.r * 0.8f, tint.g * 0.8f, tint.b * 0.8f};

	float savedScale = model->scale;
	float savedAngle = model->rotA;
	model->scale = SLOT_SCALE;
	model->rotA = slotAngle[slot];
	glColor3f(tint.r, tint.g, tint.b);
	glPushMatrix();
	glTranslatef(r.cx(), r.y + NAME_BAND_H + 1.2f - (held ? TILE_SINK : 0.f), 0);
	model->Draw();
	model->DrawPart(); // a ring's gem, right after its band
	glPopMatrix();
	model->scale = savedScale;
	model->rotA = savedAngle;
}

void Inventory::DrawSlotLabels(int slot) {
	Rect r = slotRect(Order(), slot, Scroll());
	if (SlotHeld(slot))
		r.y -= TILE_SINK;
	bool owned = bag.Count(kindOf(slot)) > 0;
	bool lit = slot == selectedSlot || SlotHovered(slot);

	Color nameColor = lit ? GOLD : Color{0.78f, 0.64f, 0.40f};
	if (!owned)
		nameColor = {0.36f, 0.29f, 0.20f};
	if (bag.Found(kindOf(slot)))
		textCenteredIn(small, {r.x, r.y, r.w, NAME_BAND_H}, itemText(kindOf(slot)).shortName, nameColor);
	else // not even its name: the player does not know it exists
		unknownMark(title, {r.x, r.y + NAME_BAND_H, r.w, r.h - NAME_BAND_H}, lit ? 0.9f : 0.6f);

	if (!isWeapon(slot) && owned) {
		char count[8];
		snprintf(count, sizeof(count), "%d", bag.Count(kindOf(slot)));
		textCentered(small, r.x + r.w - 2.55f, r.y + r.h - 3.7f, count, GOLD);
	}

	for (int hand = 0; hand < HAND_COUNT; hand++) // a worn ring: the hand it is on
		if (bag.RingOn(static_cast<Hand>(hand)) == kindOf(slot))
			textCentered(small, handBadge(r, hand).cx(), handBadge(r, hand).y + 0.3f, HAND_MARKS[hand], GOLD);

	if (isWeapon(slot) && owned) {
		char level[8];
		snprintf(level, sizeof(level), "Lv %d", bag.Level(kindOf(slot)));
		text(small, r.x + r.w - small.TextWidth(level) - 1.f, r.y + r.h - 3.8f, level, lit ? GOLD : GOLD_DIM);
	}
}

constexpr float STAT_Y = 37.4f; // the baseline of the details' stat block
// One function per item class draws its stats (plan refactor-inventory-details).

// The red line under the name: what class the item is, and what it does now.
std::string Inventory::DetailKind() const {
	const ItemKind kind = kindOf(selectedSlot);
	if (isAmulet(selectedSlot))
		return bag.Worn() == kind ? "Amulet, worn" : "Amulet";
	if (isRing(selectedSlot)) {
		const bool left = bag.RingOn(Hand::Left) == kind;
		const bool right = bag.RingOn(Hand::Right) == kind;
		if (left && right)
			return "Ring, on both hands";
		return left ? "Ring, left hand" : right ? "Ring, right hand" : "Ring";
	}
	if (isWeapon(selectedSlot))
		return std::string(!isRanged(kind) ? "Close combat" : "Ranged") + ", level " + std::to_string(bag.Level(kind));
	return "Potion";
}

void Inventory::DrawPotionDetails(const ItemText& info) {
	textCentered(body, DETAIL_PANEL.cx(), STAT_Y, info.effect, INK);
}

void Inventory::DrawRingDetails(const ItemText& info) {
	const float cx = DETAIL_PANEL.cx();
	textCentered(body, cx, STAT_Y + 5.f, info.effect, INK);
	// Two at a time, one per hand: what each hand wears, and what the selected ring's gem gives in all.
	char hands[64];
	const std::optional<ItemKind> left = bag.RingOn(Hand::Left);
	const std::optional<ItemKind> right = bag.RingOn(Hand::Right);
	if (left || right)
		snprintf(
			hands,
			sizeof(hands),
			"Left: %s   Right: %s",
			left ? itemText(*left).shortName : "-",
			right ? itemText(*right).shortName : "-"
		);
	else
		snprintf(hands, sizeof(hands), "Two rings, one on each hand");
	textCentered(small, cx, STAT_Y + 0.5f, hands, INK_FADED);
	const std::string sum = ringSumText(world.player.stats.Rings(), ringOf(kindOf(selectedSlot)).gem);
	if (!sum.empty())
		textCentered(small, cx, STAT_Y - 3.2f, ("Worn: " + sum).c_str(), INK_GREEN);
}

void Inventory::DrawAmuletDetails(const ItemText& info) {
	const float cx = DETAIL_PANEL.cx();
	textCentered(body, cx, STAT_Y + 5.f, info.effect, INK);
	// One amulet at a time: what putting this one on would take off.
	const std::optional<ItemKind> worn = bag.Worn();
	if (worn && *worn != kindOf(selectedSlot))
		textCentered(small, cx, STAT_Y + 0.5f, (std::string("Worn now: ") + itemText(*worn).name).c_str(), INK_FADED);
	else if (!worn)
		textCentered(small, cx, STAT_Y + 0.5f, "One amulet at a time", INK_FADED);
	// Upgrade progress: the spares' points of the cost (plan amulet-upgrades).
	char points[40] = "Top tier";
	const bool top = !nextTier(kindOf(selectedSlot));
	const int cost = amuletWorth(amuletOf(kindOf(selectedSlot)).tier);
	const int have = bag.UpgradePoints(kindOf(selectedSlot));
	if (!top)
		snprintf(
			points,
			sizeof(points),
			"Upgrade: %d / %d %s",
			std::min(have, cost),
			cost,
			cost == 1 ? "point" : "points"
		);
	if (bag.Count(kindOf(selectedSlot)) > 0)
		textCentered(small, cx, STAT_Y - 3.2f, points, !top && have >= cost ? INK_GREEN : INK_FADED);
}

void Inventory::DrawWeaponDetails() {
	const WeaponDef& shown = weaponDef(kindOf(selectedSlot));
	const WeaponDef& current = weaponDef(bag.Equipped());
	int shownDamage = weaponDamage(kindOf(selectedSlot), shown.damage, bag.Level(kindOf(selectedSlot)));
	char buf[48];
	float labelX = DETAIL_PANEL.x + 9;
	float valueX = DETAIL_PANEL.x + 30;
	// unit: the values are tenths, shown with one decimal and the unit after them
	auto statRow = [&](float y, const char* label, int value, int currentValue, const char* unit = nullptr) {
		text(body, labelX, y, label, INK_FADED);
		if (unit)
			snprintf(buf, sizeof(buf), "%.1f%s", static_cast<double>(value) / 10, unit);
		else
			snprintf(buf, sizeof(buf), "%d", value);
		text(body, valueX, y, buf, INK);
		if (kindOf(selectedSlot) == bag.Equipped() || value == currentValue)
			return;
		const float diffX = valueX + body.TextWidth(unit ? "0.0/s" : "000") + 1;
		if (unit)
			snprintf(buf, sizeof(buf), "%+.1f", static_cast<double>(value - currentValue) / 10);
		else
			snprintf(buf, sizeof(buf), "%+d", value - currentValue);
		text(body, diffX, y, buf, value > currentValue ? INK_GREEN : INK_RED);
	};
	// Attacks per second in tenths (plan weapon-compare-speed); a bow's AttackMs holds its draw.
	auto attacksPerSecond = [](const WeaponDef& w) {
		return static_cast<int>(std::lround(10000.0 / w.motion.AttackMs()));
	};
	constexpr float ROW = 2.8f;
	const float typeY = STAT_Y + 2.5f * ROW;
	// The damage mix, the main type first: what the monsters' resistances work on.
	const DamageMix& mix = shown.mix;
	const auto main = std::to_underlying(mainType(mix));
	text(body, labelX, typeY, "Type", INK_FADED);
	snprintf(buf, sizeof(buf), "%s %d%%", DAMAGE_TYPE_NAMES[main], mix[main]);
	text(body, valueX, typeY, buf, INK);
	float minorX = valueX + body.TextWidth(buf) + 1.5f;
	for (size_t t = 0; t < DAMAGE_TYPE_COUNT; t++) {
		if (t == main || mix[t] <= 0)
			continue;
		snprintf(buf, sizeof(buf), "%s %d%%", DAMAGE_TYPE_NAMES[t], mix[t]);
		text(small, minorX, typeY + 0.2f, buf, INK_FADED);
		minorX += small.TextWidth(buf) + 1.5f;
	}
	statRow(typeY - ROW, "Damage", shownDamage, EquippedDamage());
	statRow(typeY - 2 * ROW, "Speed", attacksPerSecond(shown), attacksPerSecond(current), "/s");
	statRow(typeY - 3 * ROW, "Range", shown.range, current.range);

	// Progress towards the next level: copies collected / copies needed.
	int level = bag.Level(kindOf(selectedSlot));
	const float copiesY = typeY - 4 * ROW;
	text(body, labelX, copiesY, "Copies", INK_FADED);
	if (level >= MAX_WEAPON_LEVEL) {
		text(body, valueX, copiesY, "Max level", INK);
	} else {
		int cost = upgradeCost(level);
		snprintf(buf, sizeof(buf), "%d / %d", bag.Count(kindOf(selectedSlot)), cost);
		text(body, valueX, copiesY, buf, bag.Count(kindOf(selectedSlot)) >= cost ? INK_GREEN : INK);
		snprintf(buf, sizeof(buf), "next %d dmg", weaponDamage(kindOf(selectedSlot), shown.damage, level + 1));
		text(small, valueX + body.TextWidth("00 / 00") + 1.5f, copiesY + 0.2f, buf, INK_FADED);
	}
}

void Inventory::DrawDetails() {
	const ItemText& info = itemText(kindOf(selectedSlot));
	const bool found = bag.Found(kindOf(selectedSlot)); // a potion used up still shows what it does
	float cx = DETAIL_PANEL.cx();

	// Shadow under the model on its plinth line.
	ellipse(cx, PLINTH_Y, 12.f, 1.8f, INK, 0.45f);

	beginText();
	constexpr float LORE_Y = 28.2f;
	if (!found) { // nothing that tells what it is
		textCentered(title, cx, PLINTH_Y + 3.f, "?", INK_FADED);
		textCentered(heading, cx, 77.5f, "Unknown", INK_FADED);
		textCentered(small, cx, LORE_Y, "Still hidden somewhere", INK_FADED);
		textCentered(small, cx, LORE_Y - 3.4f, "in the tomb...", INK_FADED);
		DrawDetailRules();
		return;
	}
	textCentered(heading, cx, 77.5f, info.name, INK);
	textCentered(small, cx, 73.8f, DetailKind().c_str(), INK_RED);

	if (isPotion(selectedSlot))
		DrawPotionDetails(info);
	else if (isRing(selectedSlot))
		DrawRingDetails(info);
	else if (isAmulet(selectedSlot))
		DrawAmuletDetails(info);
	else
		DrawWeaponDetails();
	if (ShrineButtons() == ShrineRow::None) { // at a shrine its buttons take the lore's place
		textCentered(small, cx, LORE_Y, info.lore1, INK_FADED);
		textCentered(small, cx, LORE_Y - 3.4f, info.lore2, INK_FADED);
	}
	DrawDetailRules();
}

// The ruled lines of the details panel: under the name, above the lore.
void Inventory::DrawDetailRules() {
	const float cx = DETAIL_PANEL.cx();
	beginShapes();
	line(DETAIL_PANEL.x + 8, 72.5f, DETAIL_PANEL.x + DETAIL_PANEL.w - 8, 72.5f, INK_FADED, 0.8f, 1.f);
	diamond(cx, 72.5f, 0.6f, INK_RED, 1.f);
	line(DETAIL_PANEL.x + 8, 31.6f, DETAIL_PANEL.x + DETAIL_PANEL.w - 8, 31.6f, INK_FADED, 0.5f, 1.f);
}

void Inventory::DrawDetailModel() {
	if (!bag.Found(kindOf(selectedSlot)))
		return; // a question mark instead (DrawDetails)
	Item* model = Model(kindOf(selectedSlot));
	Color tint = {1, 1, 1};
	if (bag.Count(kindOf(selectedSlot)) <= 0)
		tint = {0.12f, 0.08f, 0.05f};

	float savedScale = model->scale;
	float savedAngle = model->rotA;
	model->scale = isPotion(selectedSlot)	? potionModelDef(potionDef(kindOf(selectedSlot)).model).detailScale
				   : isAmulet(selectedSlot) ? AMULET_DETAIL_SCALE
				   : isRing(selectedSlot)	? RING_DETAIL_SCALE
											: WEAPON_DETAIL_SCALE;
	model->rotA = slotAngle[selectedSlot];
	glColor3f(tint.r, tint.g, tint.b);
	glPushMatrix();
	glTranslatef(DETAIL_PANEL.cx(), PLINTH_Y, 0);
	model->Draw();
	model->DrawPart();
	glPopMatrix();
	model->scale = savedScale;
	model->rotA = savedAngle;
}

void Inventory::DrawButtons() {
	const char* label = nullptr;
	bool canUse = CanUse(selectedSlot, &label);
	DrawButton(Target::UseButton, label, canUse);
	if (isRing(selectedSlot))
		DrawButton(
			Target::UpgradeButton,
			bag.RingOn(Hand::Right) == kindOf(selectedSlot) ? "Take off" : "Right hand",
			canUse
		);
	else if (TwoButtons()) // an amulet ready away from a shrine: where to go
		DrawButton(
			Target::UpgradeButton,
			isAmulet(selectedSlot) && !Shrine() ? "At a shrine" : "Upgrade",
			CanUpgrade(selectedSlot)
		);
	DrawShrineButtons();
}

// At a shrine (plan shrine-offerings): Offer a spare, and Merge two rings.
void Inventory::DrawShrineButtons() {
	const ShrineRow row = ShrineButtons();
	if (row == ShrineRow::None)
		return;
	const bool alive = world.player.Alive();
	DrawButton(Target::OfferButton, "Offer", bag.CanOffer(kindOf(selectedSlot), alive, AtShrine::Yes));
	if (row == ShrineRow::OfferMerge)
		DrawButton(Target::MergeButton, "Merge", bag.CanMerge(kindOf(selectedSlot), alive, AtShrine::Yes));
}

void Inventory::DrawButton(Target which, const char* label, bool enabled) {
	bool hovered = enabled && pointer.hovered.target == which;
	bool held = hovered && pointer.Held({which});
	Rect r = which == Target::UpgradeButton ? UPGRADE_BUTTON
			 : which == Target::MergeButton ? MERGE_BUTTON
			 : which == Target::OfferButton
				 ? (ShrineButtons() == ShrineRow::OfferMerge ? OFFER_BUTTON : SHRINE_WIDE_BUTTON)
			 : TwoButtons() ? EQUIP_BUTTON
							: WIDE_BUTTON;
	r = tile(r, enabled ? TileStyle::Lapis : TileStyle::PapyrusDisabled, hovered, held);

	beginText();
	if (enabled)
		textCenteredIn(heading, r, label, hovered ? TEXT_HOVER : GOLD);
	else
		textCenteredIn(body, r, label, INK_FADED);
	beginShapes();
}

// Three rows under the grid: level and XP with the attributes, the health and stamina bars, then the guard per damage
// type (plan armor-split) with its make-up in a tooltip.
void Inventory::DrawStatus() {
	const PlayerStats* s = &world.player.stats;
	constexpr float ROW_A = STATS_ROW_Y[0];
	constexpr float ROW_B = STATS_ROW_Y[1];
	constexpr float ROW_C = STATS_ROW_Y[2];
	constexpr float BAR_H = 2.4f;
	float x0 = ITEMS_PANEL.x + 5;

	line(x0, STATS_RULE_Y, ITEMS_PANEL.x + ITEMS_PANEL.w - 5, STATS_RULE_Y, BRONZE, 1.f, 1.f);

	auto bar = [&](float x, float y, float w, float ratio, Color top, Color bottom) {
		ratio = ratio < 0.f ? 0.f : (ratio > 1.f ? 1.f : ratio);
		Rect r = {x, y + 0.3f, w, BAR_H};
		fillRect(r, {0.05f, 0.03f, 0.02f}, {0.05f, 0.03f, 0.02f}, 1.f);
		fillRect({r.x, r.y, r.w * ratio, r.h}, top, bottom, 1.f);
		strokeRect(r, GOLD_DIM, 1.f, 1.f);
	};
	auto ratioOf = [](double value, double max) { return static_cast<float>(max > 0 ? value / max : 0); };

	double levelEnd = PlayerStats::LevelXP(s->CurrentLevel() + 1);
	bar(22.f, ROW_A, 18.f, s->LevelProgress(), {1.f, 0.85f, 0.45f}, GOLD_DIM);
	bar(22.f, ROW_B, 18.f, ratioOf(s->CurrentHP(), s->CurrentMaxHP()), {0.85f, 0.25f, 0.15f}, HEALTH);
	bar(66.f,
		ROW_B,
		14.f,
		ratioOf(world.player.stats.Stamina(), world.player.stats.MaxStamina()),
		{0.95f, 0.85f, 0.3f},
		STAMINA);

	char buf[32];
	beginText();
	auto labelValue = [&](float x, float y, const char* label, const char* value) {
		text(small, x, y, label, GOLD_DIM);
		text(small, x + small.TextWidth(label) + 1.2f, y, value, GOLD);
	};
	snprintf(buf, sizeof(buf), "%d", s->CurrentLevel());
	labelValue(x0, ROW_A, "Level", buf);
	snprintf(buf, sizeof(buf), "%d/%d", static_cast<int>(s->CurrentXP()), static_cast<int>(levelEnd));
	text(small, 41.5f, ROW_A, buf, GOLD);
	snprintf(buf, sizeof(buf), "%d", s->CurrentMight());
	labelValue(66.f, ROW_A, "Might", buf);
	snprintf(buf, sizeof(buf), "%d", s->Damage(EquippedDamage()));
	labelValue(80.f, ROW_A, "Dmg", buf);

	text(small, x0, ROW_B, "Health", GOLD_DIM);
	snprintf(buf, sizeof(buf), "%d/%d", s->CurrentHP(), s->CurrentMaxHP());
	text(small, 41.5f, ROW_B, buf, GOLD);
	text(small, 54.f, ROW_B, "Stamina", GOLD_DIM);
	snprintf(buf, sizeof(buf), "%d/%d", world.player.stats.Stamina(), world.player.stats.MaxStamina());
	text(small, 81.5f, ROW_B, buf, GOLD);

	const ArmorPoints armor = s->CurrentArmor();
	const int hoveredGuard = HoveredAt(Target::Guard);
	for (int i = 0; i < GUARD_COUNT; i++) {
		const DamageType type = GUARD_TYPES[i];
		const char* label = GUARD_LABELS[i];
		const float x = guardRect(i).x + 0.5f;
		text(small, x, ROW_C, label, hoveredGuard == i ? LABEL : GOLD_DIM);
		text(small, x + small.TextWidth(label) + 1.2f, ROW_C, guardText(type, s->resist, armor).c_str(), GOLD);
	}
	if (hoveredGuard >= 0)
		tooltip(guardRect(hoveredGuard), guardTooltip(GUARD_TYPES[hoveredGuard], s->resist, armor));
	beginShapes();
}

void Inventory::DrawFooter() {
	constexpr float CENTRE = CANVAS_W / 2;
	if (float alpha = toast.Alpha(GameClock::now()); alpha > 0.f)
		textCentered(body, CENTRE, 7.2f, toast.text.c_str(), {1.f, 0.9f, 0.6f}, alpha);
	textCentered(
		small,
		CENTRE,
		2.2f,
		Shrine()
			? "Click: select    Right click / Enter: use    U: upgrade    O: offer    Arrows: browse    Tab: next "
			  "group    I / "
			  "Esc: close"
			: "Click: select    Right click / Enter: use    U: upgrade    Arrows: browse    Tab: next group    I / "
			  "Esc: close",
		{0.55f, 0.45f, 0.30f}
	);
}

// ---- saves -----------------------------------------------------------------

void Inventory::BagLoaded() { selectedSlot = itemIndex(bag.Equipped()); }
