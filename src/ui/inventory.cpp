#include "inventory.h"
#include "../entities/player_stats.h"
#include "../entities/item.h"
#include "../state/game_state.h"
#include "../core/logger.h"
#include "../core/timer.h"
#include "../input/input.h"
#include <GL/gl.h>
#include "../graphics/gl_includes.h"
#include "ui_draw.h"
#include "screen_tabs.h"
#include "player_hud.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

// Layout works on a 160 x 100 canvas (y up) that keeps its aspect ratio and is centred on the window;
// the backdrop fills whatever the window adds around it.
namespace {

using namespace ui;

constexpr Color HEALTH = {0.72f, 0.14f, 0.09f};
constexpr Color STAMINA = {0.78f, 0.68f, 0.16f};

constexpr Rect ITEMS_PANEL = {4, 13, 92, 72};
constexpr Rect DETAIL_PANEL = {100, 13, 56, 72};
constexpr Rect WIDE_BUTTON = {107, 16, 42, 7}; // potions: Drink
constexpr Rect EQUIP_BUTTON = {104, 16, 23, 7};
constexpr Rect UPGRADE_BUTTON = {129, 16, 23, 7};
// One grid for every tab: COLUMNS slots a row, as many rows as the group needs. VISIBLE_ROWS fit the panel; a group
// with more scrolls by whole rows (the wheel, the arrow keys past the last row shown), with a thin bar at the side.
constexpr int COLUMNS = 4;
constexpr int VISIBLE_ROWS = 2;
constexpr float SLOT_W = 19.f;
constexpr float SLOT_H = 22.f;
constexpr float SLOT_GAP = 3.f;
constexpr float TOP_ROW_Y = 52.f;
constexpr float NAME_BAND_H = 4.4f;
constexpr float SLOT_SCALE = 14.5f;

// The group tabs across the top of the items panel, the same width as the grid.
constexpr float TAB_Y = 77.6f;
constexpr float TAB_H = 6.f;
constexpr float TAB_GAP = 2.f;
constexpr const char* GROUP_NAMES[ITEM_GROUP_COUNT] = {"Weapons", "Potions", "Amulets", "Rings"};
constexpr PlayerHud::Icon GROUP_ICONS[ITEM_GROUP_COUNT] = {PlayerHud::weaponIcon(ItemKind::ShortSword),
														   PlayerHud::Icon::Potion, PlayerHud::Icon::Amulet,
														   PlayerHud::Icon::Ring};

constexpr float WEAPON_DETAIL_SCALE = 22.f;
constexpr float POTION_DETAIL_SCALE = 16.f;
constexpr float PLINTH_Y = 49.f;		 // detail model base
constexpr float REST_ANGLE = 25.f;		 // degrees, idle slots show the model a little turned
constexpr float SPIN_DEG_PER_MS = 0.09f; // hovered / selected models

constexpr const char* HOTKEYS = "1234567890-="; // the first slots of the open tab, the keyboard's number row
constexpr int HOTKEY_COUNT = 12;
constexpr float SCROLL_BAR_W = 1.f;

ItemKind kindOf(int slot) { return itemAt(slot); }
bool isPotion(int slot) { return ::isPotion(kindOf(slot)); }
ItemGroup groupOf(int slot) { return itemGroup(kindOf(slot)); }
int positionOf(int slot) { return slot - groupItems(groupOf(slot)).first; } // in its tab's grid
ItemGroup groupAt(int index) { return static_cast<ItemGroup>(index); }

constexpr std::array<Color, POTION_KIND_COUNT> POTION_COLORS = {{
	{1.f, 0.f, 0.f},	   // small health
	{0.7f, 0.f, 0.3f},	   // large health
	{0.4f, 0.f, 0.6f},	   // might
	{1.f, 0.6f, 0.f},	   // armor
	{0.7f, 0.6f, 0.3f},	   // life
	{0.45f, 0.85f, 0.25f}, // small stamina: green faience
	{0.15f, 0.78f, 0.72f}, // large stamina: turquoise
	{0.05f, 0.45f, 0.2f},  // antidote: dark malachite
}};

Color potionColor(ItemKind potion) { return POTION_COLORS[static_cast<size_t>(itemIndex(potion) - WEAPON_KIND_COUNT)]; }

Vitals playerVitals() {
	const PlayerStats& s = Game().player->stats;
	return {Game().player->Alive(), s.CurrentHP(), s.CurrentMaxHP(), s.Stamina(), s.MaxStamina(), s.poison.Any()};
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
	case UseBlock::None:
		break;
	}
	return "";
}

constexpr float GRID_W = COLUMNS * SLOT_W + (COLUMNS - 1) * SLOT_GAP;

int rowsOf(ItemGroup group) { return (groupItems(group).count + COLUMNS - 1) / COLUMNS; }
int rowOf(int slot) { return positionOf(slot) / COLUMNS; }

// Its tab scrolled down by `scroll` rows.
Rect slotRect(int slot, int scroll) {
	const int position = positionOf(slot);
	const float x0 = ITEMS_PANEL.cx() - GRID_W / 2;
	const auto column = static_cast<float>(position % COLUMNS);
	const auto row = static_cast<float>(rowOf(slot) - scroll);
	return {x0 + column * (SLOT_W + SLOT_GAP), TOP_ROW_Y - row * (SLOT_H + SLOT_GAP), SLOT_W, SLOT_H};
}

bool slotVisible(int slot, int scroll) { return rowOf(slot) >= scroll && rowOf(slot) < scroll + VISIBLE_ROWS; }

// The question mark of an item not found yet, centred in `r`.
void unknownMark(Font& font, const Rect& r, float alpha) {
	textCentered(font, r.cx(), r.cy() - 2.5f, "?", {0.45f, 0.42f, 0.38f}, alpha);
}

Rect tabRect(int group) {
	const float w = (GRID_W - (ITEM_GROUP_COUNT - 1) * TAB_GAP) / ITEM_GROUP_COUNT;
	return {ITEMS_PANEL.cx() - GRID_W / 2 + static_cast<float>(group) * (w + TAB_GAP), TAB_Y, w, TAB_H};
}

Rect visibleArea() { return ui::visibleArea(CANVAS_W, CANVAS_H, Game().render.resX, Game().render.resY); }

void toCanvas(int mouseX, int mouseY, float& x, float& y) {
	ui::toCanvas(visibleArea(), Game().render.resX, Game().render.resY, mouseX, mouseY, x, y);
}

} // namespace

Inventory::Inventory() {
	loadScreenFonts(title, heading, body, small, 7.f);

	Reset();
	for (float& angle : slotAngle)
		angle = REST_ANGLE;
}

void Inventory::Reset() {
	bag.Reset();
	for (int group = 0; group < ITEM_GROUP_COUNT; group++)
		tabSlot[group] = groupItems(groupAt(group)).first;
	Select(itemIndex(ItemKind::Club));
	toast = ui::Toast{};
	quickDrinkMs.reset();
}

Inventory::~Inventory() {}

void Inventory::AddItem(ItemKind kind) { bag.Find(kind, Game().journal); }

Item* Inventory::Model(ItemKind kind) { return Game().assets.items.Of(kind); }

int Inventory::EquippedDamage() const {
	return weaponDamage(bag.Equipped(), Model(bag.Equipped())->damage, bag.Level(bag.Equipped()));
}

// ---- actions ---------------------------------------------------------------

bool Inventory::CanUse(int slot, const char** reason) const {
	UseBlock block = bag.Block(kindOf(slot), playerVitals());
	if (reason != nullptr)
		*reason = block != UseBlock::None ? blockReason(block) : (isPotion(slot) ? "Drink" : "Equip");
	return block == UseBlock::None;
}

bool Inventory::CanUpgrade(int slot) const { return bag.CanUpgrade(kindOf(slot), Game().player->Alive()); }

void Inventory::Upgrade(int slot) {
	if (!bag.Upgrade(kindOf(slot), Game().player->Alive()))
		return;
	char buf[64];
	snprintf(buf, sizeof(buf), "%s reaches level %d", itemText(kindOf(slot)).name, bag.Level(kindOf(slot)));
	ShowToast(buf);
}

void Inventory::Use(int slot) {
	ItemKind kind = kindOf(slot);
	if (!CanUse(slot, nullptr))
		return;
	if (::isPotion(kind)) {
		ShowToast(DrinkPotion(kind));
		return;
	}
	bag.Use(kind, playerVitals());
	ShowToast(std::string(itemText(kind).name) + " equipped");
}

std::string Inventory::DrinkPotion(ItemKind potion) {
	PlayerStats* s = &Game().player->stats;
	int hpBefore = s->CurrentHP();
	int staminaBefore = s->Stamina();

	Game().assets.sounds.drink_s.Play();
	bag.Use(potion, playerVitals());

	PotionGain gain = potionGain(potion);
	char buf[64];
	if (gain.healPercent > 0) {
		s->Heal(gain.healPercent);
		snprintf(buf, sizeof(buf), "Healed %d health", s->CurrentHP() - hpBefore);
	} else if (gain.might > 0) {
		s->AddMight(gain.might);
		snprintf(buf, sizeof(buf), "Might rises to %d", s->CurrentMight());
	} else if (gain.armor > 0) {
		s->AddArmor(gain.armor);
		snprintf(buf, sizeof(buf), "Armor rises to %d", s->CurrentArmor());
	} else if (gain.cure) {
		s->poison.Cure();
		snprintf(buf, sizeof(buf), "The poison is gone");
	} else if (gain.maxHpPercent > 0) {
		s->AddMaxHP(gain.maxHpPercent);
		snprintf(buf, sizeof(buf), "Max health rises to %d", s->CurrentMaxHP());
	} else {
		s->AddStamina(s->MaxStamina() * gain.staminaPercent / 100);
		snprintf(buf, sizeof(buf), "Restored %d stamina", s->Stamina() - staminaBefore);
	}
	return buf;
}

std::optional<ItemKind> Inventory::QuickChoice(QuickKind kind) const {
	const PlayerStats& s = Game().player->stats;
	bool health = kind == QuickKind::Health;
	int current = health ? s.CurrentHP() : s.Stamina();
	int max = health ? s.CurrentMaxHP() : s.MaxStamina();
	// At full, still show what a hit would make it drink: the pick for one point missing.
	return bag.QuickChoice(kind, std::min(current, max - 1), max);
}

void Inventory::QuickDrink(QuickKind kind) {
	int now = GameClock::now();
	if (quickDrinkMs && now - *quickDrinkMs < QUICK_DRINK_COOLDOWN_MS)
		return;
	const PlayerStats& s = Game().player->stats;
	bool health = kind == QuickKind::Health;
	bool full = health ? s.CurrentHP() >= s.CurrentMaxHP() : s.Stamina() >= s.MaxStamina();
	std::optional<ItemKind> potion = QuickChoice(kind);
	if (full) {
		Game().ShowStatus(health ? "You are at full health" : "You are at full energy");
		return;
	}
	if (!potion) {
		Game().ShowStatus(health ? "No healing potion left" : "No stamina potion left");
		return;
	}
	quickDrinkMs = now;
	quickDrinkKind = kind;
	Game().ShowStatus("%s", DrinkPotion(*potion).c_str());
}

void Inventory::QuickAntidote() {
	const UseBlock block = bag.Block(ItemKind::Antidote, playerVitals());
	if (block == UseBlock::Dead)
		return;
	if (block != UseBlock::None) {
		Game().ShowStatus("%s", block == UseBlock::NotPoisoned ? "You are not poisoned" : "No antidote left");
		return;
	}
	Game().ShowStatus("%s", DrinkPotion(ItemKind::Antidote).c_str());
}

std::optional<int> Inventory::QuickDrinkMs(QuickKind kind) const {
	return quickDrinkKind == kind ? quickDrinkMs : std::nullopt;
}

Color Inventory::PotionColor(ItemKind potion) { return potionColor(potion); }

void Inventory::Select(int slot) {
	if (slot < 0 || slot >= ITEM_KIND_COUNT)
		return;
	selectedSlot = slot;
	tab = groupOf(slot);
	tabSlot[static_cast<int>(tab)] = slot;
	int& scroll = scrollRow[static_cast<int>(tab)]; // the selected slot in view
	scroll = std::clamp(scroll, rowOf(slot) - VISIBLE_ROWS + 1, rowOf(slot));
}

void Inventory::ScrollBy(int rows) {
	int& scroll = scrollRow[static_cast<int>(tab)];
	scroll = std::clamp(scroll + rows, 0, std::max(0, rowsOf(tab) - VISIBLE_ROWS));
}

void Inventory::SwitchTab(ItemGroup group) {
	if (TabEnabled(group))
		Select(tabSlot[static_cast<int>(group)]);
}

// Arrow keys stay in the open tab: left / right walk its slots in order, up / down move a row (down onto a shorter
// last row: its last slot).
void Inventory::MoveSelection(int dx, int dy) {
	const ItemRange range = groupItems(tab);
	const int position = selectedSlot - range.first;
	int target = position + dx;
	if (dy > 0)
		target = position - COLUMNS;
	else if (dy < 0 && position / COLUMNS < (range.count - 1) / COLUMNS)
		target = std::min(position + COLUMNS, range.count - 1);
	if (target >= 0 && target < range.count)
		Select(range.first + target);
}

void Inventory::ShowToast(const std::string& text) { toast.Show(text, GameClock::now()); }

// ---- input -----------------------------------------------------------------

void Inventory::UpdateHover(float x, float y) {
	hoveredSlot = NO_SLOT;
	for (int slot = 0; slot < ITEM_KIND_COUNT; slot++)
		if (groupOf(slot) == tab && slotVisible(slot, Scroll()) && slotRect(slot, Scroll()).contains(x, y))
			hoveredSlot = slot;
	hoveredTab = NO_TAB;
	for (int group = 0; group < ITEM_GROUP_COUNT; group++)
		if (tabRect(group).contains(x, y))
			hoveredTab = group;
	hoveredButton = Target::None;
	if (isPotion(selectedSlot)) {
		if (WIDE_BUTTON.contains(x, y))
			hoveredButton = Target::UseButton;
	} else if (EQUIP_BUTTON.contains(x, y)) {
		hoveredButton = Target::UseButton;
	} else if (UPGRADE_BUTTON.contains(x, y)) {
		hoveredButton = Target::UpgradeButton;
	}
}

void Inventory::MouseMotion(int x, int y) {
	float cx = 0.f;
	float cy = 0.f;
	toCanvas(x, y, cx, cy);
	UpdateHover(cx, cy);
}

// Left click selects a slot (on press) or presses a button (fires on release over it).
// Right click on a slot selects and uses it straight away.
void Inventory::MouseFunction(int button, int state, int x, int y) {
	float cx = 0.f;
	float cy = 0.f;
	toCanvas(x, y, cx, cy);
	UpdateHover(cx, cy);

	if (button == MOUSE_WHEEL_UP || button == MOUSE_WHEEL_DOWN) {
		if (state == GLUT_DOWN)
			ScrollBy(button == MOUSE_WHEEL_UP ? -1 : 1);
		UpdateHover(cx, cy);
		return;
	}

	if (state == GLUT_DOWN) {
		pressedMouseButton = button;
		pressed = Target::None;
		if (hoveredSlot != NO_SLOT) {
			pressed = Target::Slot;
			pressedSlot = hoveredSlot;
			Select(hoveredSlot);
		} else if (hoveredTab != NO_TAB && button == MOUSE_LEFT_BUTTON) {
			if (TabEnabled(groupAt(hoveredTab))) {
				pressed = Target::Tab;
				pressedSlot = hoveredTab;
			}
		} else if (button == MOUSE_LEFT_BUTTON) {
			pressed = hoveredButton;
		}
		return;
	}

	if (state != GLUT_UP || button != pressedMouseButton)
		return;

	if (pressed == Target::UseButton && hoveredButton == pressed)
		Use(selectedSlot);
	else if (pressed == Target::UpgradeButton && hoveredButton == pressed)
		Upgrade(selectedSlot);
	else if (pressed == Target::Tab && hoveredTab == pressedSlot)
		SwitchTab(groupAt(pressedSlot));
	else if (pressed == Target::Slot && button == MOUSE_RIGHT_BUTTON && hoveredSlot == pressedSlot)
		Use(pressedSlot);
	pressed = Target::None;
	pressedSlot = NO_SLOT;
}

void Inventory::KeyPressed(unsigned char key) {
	switch (key) {
	case KEY_ENTER:
	case KEY_SPACE:
	case 'e':
	case 'E':
		Use(selectedSlot);
		return;
	case 'u':
	case 'U':
		Upgrade(selectedSlot);
		return;
	case KEY_MOVE_LEFT:
	case KEY_MOVE_LEFT_UPPER:
		MoveSelection(-1, 0);
		return;
	case KEY_MOVE_RIGHT:
	case KEY_MOVE_RIGHT_UPPER:
		MoveSelection(1, 0);
		return;
	case KEY_MOVE_UP:
	case KEY_MOVE_UP_UPPER:
		MoveSelection(0, 1);
		return;
	case KEY_MOVE_DOWN:
	case KEY_MOVE_DOWN_UPPER:
		MoveSelection(0, -1);
		return;
	default:
		// The number row picks one of the open tab's first slots, in ItemKind order.
		if (const char* hotkey = key != 0 ? strchr(HOTKEYS, key) : nullptr) {
			const int position = static_cast<int>(hotkey - HOTKEYS);
			const ItemRange range = groupItems(tab);
			if (position < range.count)
				Select(range.first + position);
		}
		return;
	}
}

bool Inventory::Equip(ItemKind weapon) {
	if (::isPotion(weapon))
		return false;
	Use(itemIndex(weapon));
	return bag.Equipped() == weapon;
}

void Inventory::EquipNext(bool ranged) {
	const ItemKind held = bag.Equipped();
	const int from = isRanged(held) == ranged ? itemIndex(held) : -1;
	for (int step = 1; step < WEAPON_KIND_COUNT + 1; step++) {
		const ItemKind kind = itemAt((from + step) % WEAPON_KIND_COUNT);
		if (isRanged(kind) != ranged)
			continue;
		if (kind == held) // round to it again: no other one of its class
			return;
		if (bag.Count(kind) > 0) {
			Use(itemIndex(kind));
			return;
		}
	}
}

void Inventory::SpecialKeyPressed(int key) {
	switch (key) {
	case SPECIAL_MOVE_LEFT:
		MoveSelection(-1, 0);
		return;
	case SPECIAL_MOVE_RIGHT:
		MoveSelection(1, 0);
		return;
	case SPECIAL_MOVE_UP:
		MoveSelection(0, 1);
		return;
	case SPECIAL_MOVE_DOWN:
		MoveSelection(0, -1);
		return;
	default:
		return;
	}
}

// ---- drawing ---------------------------------------------------------------

bool Inventory::SlotHeld(int slot) const {
	return pressed == Target::Slot && pressedSlot == slot && slot == hoveredSlot;
}

void Inventory::Draw() {
	int now = GameClock::now();
	int elapsed = now - lastFrameMs;
	lastFrameMs = now;
	if (elapsed < 0 || elapsed > 100)
		elapsed = 0;
	for (int slot = 0; slot < ITEM_KIND_COUNT; slot++)
		if (slot == selectedSlot || slot == hoveredSlot)
			slotAngle[slot] = std::fmod(slotAngle[slot] + SPIN_DEG_PER_MS * static_cast<float>(elapsed), 360.f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	Rect area = visibleArea();
	glOrtho(area.x, area.x + area.w, area.y, area.y + area.h, -200, 200);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);

	const ItemRange group = groupItems(tab);
	const int shownFirst = group.first + Scroll() * COLUMNS;
	const int shownEnd = std::min(group.first + group.count, shownFirst + COLUMNS * VISIBLE_ROWS);

	DrawBackground();
	DrawTabs();
	for (int slot = shownFirst; slot < shownEnd; slot++)
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
	for (int slot = shownFirst; slot < shownEnd; slot++)
		DrawSlotModel(slot);
	DrawDetailModel();
	glDisable(GL_DEPTH_TEST);

	beginText();
	for (int slot = shownFirst; slot < shownEnd; slot++)
		DrawSlotLabels(slot);
	DrawTabHint();
	DrawFooter();
	ScreenTabs::Draw();

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
	backdrop(visibleArea(), Game().assets.textures.loadingBackground.ID());
	titleBar(title, CANVAS_W / 2, "Inventory", SCREEN_TABS_TITLE_REACH);

	panel(ITEMS_PANEL, 0.9f);

	// Papyrus scroll for the details, in a frame matching the items panel.
	glEnable(GL_TEXTURE_2D);
	Game().assets.textures.papyrus.Bind();
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
	const int icons = Game().assets.textures.hudIcons.ID();
	for (int group = 0; group < ITEM_GROUP_COUNT; group++) {
		const bool enabled = TabEnabled(groupAt(group));
		const bool active = groupAt(group) == tab;
		const bool hovered = enabled && !active && hoveredTab == group;
		const bool held = hovered && pressed == Target::Tab && pressedSlot == group;
		TileStyle style = !enabled ? TileStyle::Disabled : (active ? TileStyle::Lapis : TileStyle::Stone);
		Rect r = tile(tabRect(group), style, hovered, held);

		Color tint = groupAt(group) == ItemGroup::Potions ? potionColor(ItemKind::SmallHealth) : Color{1, 1, 1};
		if (!enabled)
			tint = {0.3f, 0.26f, 0.22f};
		PlayerHud::drawIcon(GROUP_ICONS[group], {r.x + 0.6f, r.y + 0.3f, r.h - 0.6f, r.h - 0.6f}, icons, tint);
		beginText();
		Color c = !enabled ? Color{0.36f, 0.29f, 0.20f} : (hovered ? TEXT_HOVER : GOLD);
		text(small, r.x + r.h + 0.6f, r.y + 1.7f, GROUP_NAMES[group], c);
		beginShapes();
	}
}

// Over a dimmed tab: why it does not open.
void Inventory::DrawTabHint() {
	if (hoveredTab == NO_TAB || TabEnabled(groupAt(hoveredTab)))
		return;
	char hint[32];
	snprintf(hint, sizeof(hint), "%s: none yet", GROUP_NAMES[hoveredTab]);
	const Rect tabR = tabRect(hoveredTab);
	const float w = small.TextWidth(hint) + 3.f;
	const Rect label = {std::min(tabR.cx() - w / 2, ITEMS_PANEL.x + ITEMS_PANEL.w - 2 - w), tabR.y - 5.4f, w, 4.4f};
	beginShapes();
	fillRect(label, PANEL_TOP, PANEL_BOTTOM, 0.95f);
	strokeRect(label, GOLD_DIM, 1.f, 1.f);
	beginText();
	text(small, label.x + 1.5f, label.y + 1.2f, hint, GOLD);
}

void Inventory::DrawSlot(int slot) {
	Rect r = slotRect(slot, Scroll());
	bool hovered = slot == hoveredSlot;
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
	if (kindOf(slot) == bag.Equipped())
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
	if (isPotion(slot) && owned) {
		Rect badge = {r.x + r.w - 4.6f, r.y + r.h - 4.f, 4.1f, 3.5f};
		fillRect(badge, {0.05f, 0.04f, 0.03f}, {0.05f, 0.04f, 0.03f}, 0.85f);
		strokeRect(badge, GOLD_DIM, 1.f, 1.f);
	}

	// Upgrade ready: a pulsing gold arrow under the level.
	if (CanUpgrade(slot)) {
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
	Rect r = slotRect(slot, Scroll());
	bool held = SlotHeld(slot);
	Item* model = Model(kindOf(slot));

	Color tint = {1, 1, 1};
	if (isPotion(slot))
		tint = potionColor(kindOf(slot));
	if (bag.Count(kindOf(slot)) <= 0)
		tint = {0.07f, 0.055f, 0.04f};
	else if (slot != selectedSlot && slot != hoveredSlot)
		tint = {tint.r * 0.8f, tint.g * 0.8f, tint.b * 0.8f};

	float savedScale = model->scale;
	float savedAngle = model->rotA;
	model->scale = SLOT_SCALE;
	model->rotA = slotAngle[slot];
	glColor3f(tint.r, tint.g, tint.b);
	glPushMatrix();
	glTranslatef(r.cx(), r.y + NAME_BAND_H + 1.2f - (held ? TILE_SINK : 0.f), 0);
	model->Draw();
	glPopMatrix();
	model->scale = savedScale;
	model->rotA = savedAngle;
}

void Inventory::DrawSlotLabels(int slot) {
	Rect r = slotRect(slot, Scroll());
	if (SlotHeld(slot))
		r.y -= TILE_SINK;
	bool owned = bag.Count(kindOf(slot)) > 0;
	bool lit = slot == selectedSlot || slot == hoveredSlot;

	Color nameColor = lit ? GOLD : Color{0.78f, 0.64f, 0.40f};
	if (!owned)
		nameColor = {0.36f, 0.29f, 0.20f};
	if (bag.Found(kindOf(slot)))
		textCentered(small, r.cx(), r.y + 0.7f, itemText(kindOf(slot)).shortName, nameColor);
	else // not even its name: the player does not know it exists
		unknownMark(title, {r.x, r.y + NAME_BAND_H, r.w, r.h - NAME_BAND_H}, lit ? 0.9f : 0.6f);

	if (positionOf(slot) < HOTKEY_COUNT) {
		char key[2] = {HOTKEYS[positionOf(slot)], '\0'};
		text(small, r.x + 1.f, r.y + r.h - 3.8f, key, lit ? GOLD : GOLD_DIM, owned ? 1.f : 0.5f);
	}

	if (isPotion(slot) && owned) {
		char count[8];
		snprintf(count, sizeof(count), "%d", bag.Count(kindOf(slot)));
		textCentered(small, r.x + r.w - 2.55f, r.y + r.h - 3.7f, count, GOLD);
	}

	if (!isPotion(slot) && owned) {
		char level[8];
		snprintf(level, sizeof(level), "Lv %d", bag.Level(kindOf(slot)));
		text(small, r.x + r.w - small.TextWidth(level) - 1.f, r.y + r.h - 3.8f, level, lit ? GOLD : GOLD_DIM);
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
	char kind[48] = "Potion";
	if (!isPotion(selectedSlot)) {
		const char* weaponKind = !isRanged(kindOf(selectedSlot)) ? "Close combat" : "Ranged";
		snprintf(kind, sizeof(kind), "%s, level %d", weaponKind, bag.Level(kindOf(selectedSlot)));
	}
	textCentered(small, cx, 73.8f, kind, INK_RED);

	constexpr float STAT_Y = 37.4f;
	if (isPotion(selectedSlot)) {
		textCentered(body, cx, STAT_Y, info.effect, INK);
	} else {
		Item* shown = Model(kindOf(selectedSlot));
		Item* current = Equipped();
		int shownDamage = weaponDamage(kindOf(selectedSlot), shown->damage, bag.Level(kindOf(selectedSlot)));
		char buf[48];
		float labelX = DETAIL_PANEL.x + 9;
		float valueX = DETAIL_PANEL.x + 30;
		auto statRow = [&](float y, const char* label, int value, int currentValue) {
			text(body, labelX, y, label, INK_FADED);
			snprintf(buf, sizeof(buf), "%d", value);
			text(body, valueX, y, buf, INK);
			if (kindOf(selectedSlot) == bag.Equipped() || value == currentValue)
				return;
			snprintf(buf, sizeof(buf), "%+d", value - currentValue);
			text(body, valueX + body.TextWidth("000") + 1, y, buf, value > currentValue ? INK_GREEN : INK_RED);
		};
		// The damage mix, the main type first: what the monsters' resistances work on.
		const DamageMix& mix = shown->mix;
		const auto main = static_cast<size_t>(mainType(mix));
		text(body, labelX, STAT_Y + 7.f, "Type", INK_FADED);
		snprintf(buf, sizeof(buf), "%s %d%%", DAMAGE_TYPE_NAMES[main], mix[main]);
		text(body, valueX, STAT_Y + 7.f, buf, INK);
		float minorX = valueX + body.TextWidth(buf) + 1.5f;
		for (size_t t = 0; t < DAMAGE_TYPE_COUNT; t++) {
			if (t == main || mix[t] <= 0)
				continue;
			snprintf(buf, sizeof(buf), "%s %d%%", DAMAGE_TYPE_NAMES[t], mix[t]);
			text(small, minorX, STAT_Y + 7.2f, buf, INK_FADED);
			minorX += small.TextWidth(buf) + 1.5f;
		}
		statRow(STAT_Y + 3.5f, "Damage", shownDamage, EquippedDamage());
		statRow(STAT_Y, "Range", shown->range, current->range);

		// Progress towards the next level: copies collected / copies needed.
		int level = bag.Level(kindOf(selectedSlot));
		text(body, labelX, STAT_Y - 3.5f, "Copies", INK_FADED);
		if (level >= MAX_WEAPON_LEVEL) {
			text(body, valueX, STAT_Y - 3.5f, "Max level", INK);
		} else {
			int cost = upgradeCost(level);
			snprintf(buf, sizeof(buf), "%d / %d", bag.Count(kindOf(selectedSlot)), cost);
			text(body, valueX, STAT_Y - 3.5f, buf, bag.Count(kindOf(selectedSlot)) >= cost ? INK_GREEN : INK);
			snprintf(buf, sizeof(buf), "next %d dmg", weaponDamage(kindOf(selectedSlot), shown->damage, level + 1));
			text(small, valueX + body.TextWidth("00 / 00") + 1.5f, STAT_Y - 3.3f, buf, INK_FADED);
		}
	}
	textCentered(small, cx, LORE_Y, info.lore1, INK_FADED);
	textCentered(small, cx, LORE_Y - 3.4f, info.lore2, INK_FADED);
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
	bool potion = isPotion(selectedSlot);
	Color tint = potion ? potionColor(kindOf(selectedSlot)) : Color{1, 1, 1};
	if (bag.Count(kindOf(selectedSlot)) <= 0)
		tint = {0.12f, 0.08f, 0.05f};

	float savedScale = model->scale;
	float savedAngle = model->rotA;
	model->scale = potion ? POTION_DETAIL_SCALE : WEAPON_DETAIL_SCALE;
	model->rotA = slotAngle[selectedSlot];
	glColor3f(tint.r, tint.g, tint.b);
	glPushMatrix();
	glTranslatef(DETAIL_PANEL.cx(), PLINTH_Y, 0);
	model->Draw();
	glPopMatrix();
	model->scale = savedScale;
	model->rotA = savedAngle;
}

void Inventory::DrawButtons() {
	const char* label = nullptr;
	bool canUse = CanUse(selectedSlot, &label);
	DrawButton(Target::UseButton, label, canUse);
	if (!isPotion(selectedSlot))
		DrawButton(Target::UpgradeButton, "Upgrade", CanUpgrade(selectedSlot));
}

void Inventory::DrawButton(Target which, const char* label, bool enabled) {
	bool hovered = enabled && hoveredButton == which;
	bool held = hovered && pressed == which;
	Rect r = which == Target::UpgradeButton ? UPGRADE_BUTTON : (isPotion(selectedSlot) ? WIDE_BUTTON : EQUIP_BUTTON);
	r = tile(r, enabled ? TileStyle::Lapis : TileStyle::PapyrusDisabled, hovered, held);

	beginText();
	if (enabled)
		textCentered(heading, r.cx(), r.y + 1.3f, label, hovered ? TEXT_HOVER : GOLD);
	else
		textCentered(body, r.cx(), r.y + 1.9f, label, INK_FADED);
	beginShapes();
}

// Two rows under the potions: level and XP with the attributes, then the health and stamina bars.
void Inventory::DrawStatus() {
	const PlayerStats* s = &Game().player->stats;
	constexpr float ROW_A = 20.2f;
	constexpr float ROW_B = 15.4f;
	constexpr float BAR_H = 2.4f;
	float x0 = ITEMS_PANEL.x + 5;

	line(x0, 25.6f, ITEMS_PANEL.x + ITEMS_PANEL.w - 5, 25.6f, BRONZE, 1.f, 1.f);

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
	bar(66.f, ROW_B, 14.f, ratioOf(Game().player->stats.Stamina(), Game().player->stats.MaxStamina()),
		{0.95f, 0.85f, 0.3f}, STAMINA);

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
	labelValue(62.f, ROW_A, "Might", buf);
	snprintf(buf, sizeof(buf), "%d", s->CurrentArmor());
	labelValue(74.f, ROW_A, "Armor", buf);
	snprintf(buf, sizeof(buf), "%d", s->Damage(EquippedDamage()));
	labelValue(86.f, ROW_A, "Dmg", buf);

	text(small, x0, ROW_B, "Health", GOLD_DIM);
	snprintf(buf, sizeof(buf), "%d/%d", s->CurrentHP(), s->CurrentMaxHP());
	text(small, 41.5f, ROW_B, buf, GOLD);
	text(small, 54.f, ROW_B, "Stamina", GOLD_DIM);
	snprintf(buf, sizeof(buf), "%d/%d", Game().player->stats.Stamina(), Game().player->stats.MaxStamina());
	text(small, 81.5f, ROW_B, buf, GOLD);
	beginShapes();
}

void Inventory::DrawFooter() {
	constexpr float CENTRE = CANVAS_W / 2;
	if (float alpha = toast.Alpha(GameClock::now()); alpha > 0.f)
		textCentered(body, CENTRE, 7.2f, toast.text.c_str(), {1.f, 0.9f, 0.6f}, alpha);
	textCentered(small, CENTRE, 2.2f,
				 "Click: select    Right click / Enter: use    U: upgrade    Arrows / 1-0: browse    I / Esc: close",
				 {0.55f, 0.45f, 0.30f});
}

// ---- saves -----------------------------------------------------------------

void Inventory::LoadDump(std::istream& f) {
	bag.Load(f);
	selectedSlot = itemIndex(bag.Equipped());
}
