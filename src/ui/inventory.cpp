#include "inventory.h"
#include "stats.h"
#include "../entities/item.h"
#include "../state/game_state.h"
#include "../core/service_locator.h"
#include "../core/logger.h"
#include "../core/timer.h"
#include "../input/input.h"
#include "../test/scenario.h"
#include <GL/gl.h>
#include "../graphics/gl_includes.h"
#include <array>
#include <cmath>
#include <cstdio>

// Layout works on a 160 x 100 canvas (y up) that keeps its aspect ratio and is centred on the window;
// the backdrop fills whatever the window adds around it.
namespace {
constexpr float CANVAS_W = 160.f;
constexpr float CANVAS_H = 100.f;

struct Rect {
	float x, y, w, h;
	[[nodiscard]] bool contains(float px, float py) const { return px >= x && px <= x + w && py >= y && py <= y + h; }
	[[nodiscard]] float cx() const { return x + w / 2; }
	[[nodiscard]] Rect inset(float d) const { return {x + d, y + d, w - 2 * d, h - 2 * d}; }
};

struct Color {
	float r, g, b;
};

// Egyptian palette: gold leaf, bronze, lapis lazuli, basalt, papyrus inks.
constexpr Color GOLD = {0.95f, 0.76f, 0.38f};
constexpr Color GOLD_DIM = {0.58f, 0.44f, 0.22f};
constexpr Color BRONZE = {0.42f, 0.30f, 0.15f};
constexpr Color LAPIS = {0.12f, 0.27f, 0.58f};
constexpr Color LAPIS_DARK = {0.05f, 0.12f, 0.30f};
constexpr Color STONE_TOP = {0.20f, 0.155f, 0.11f};
constexpr Color STONE_BOTTOM = {0.12f, 0.09f, 0.065f};
constexpr Color PANEL_TOP = {0.14f, 0.11f, 0.08f};
constexpr Color PANEL_BOTTOM = {0.08f, 0.06f, 0.045f};
constexpr Color BLACK = {0.f, 0.f, 0.f};
constexpr Color INK = {0.24f, 0.14f, 0.07f};
constexpr Color INK_RED = {0.62f, 0.17f, 0.08f};
constexpr Color INK_GREEN = {0.16f, 0.45f, 0.12f};
constexpr Color INK_FADED = {0.52f, 0.40f, 0.26f};
constexpr Color HEALTH = {0.72f, 0.14f, 0.09f};

constexpr Rect ITEMS_PANEL = {4, 13, 92, 72};
constexpr Rect DETAIL_PANEL = {100, 13, 56, 72};
constexpr Rect BUTTON = {107, 16, 42, 7};
constexpr float WEAPON_ROW_Y = 51.5f;
constexpr float POTION_ROW_Y = 22.5f;
constexpr float NAME_BAND_H = 4.4f;

constexpr float WEAPON_SLOT_SCALE = 15.f;
constexpr float POTION_SLOT_SCALE = 8.f;
constexpr float WEAPON_DETAIL_SCALE = 23.f;
constexpr float POTION_DETAIL_SCALE = 17.f;
constexpr float PLINTH_Y = 45.5f;		 // detail model base
constexpr float REST_ANGLE = 25.f;		 // degrees, idle slots show the model a little turned
constexpr float SPIN_DEG_PER_MS = 0.09f; // hovered / selected models
constexpr int TOAST_MS = 2200;
constexpr int TOAST_FADE_MS = 600;

constexpr int GLUT_BUTTON_DOWN = 0; // GLUT_DOWN / GLUT_UP
constexpr int GLUT_BUTTON_UP = 1;

struct SlotDef {
	int type;
	int id;
};

constexpr std::array<SlotDef, InvSlot::COUNT> SLOTS = {{
	{ItemType::MELEE_WEAPON, WeaponId::CLUB},
	{ItemType::MELEE_WEAPON, WeaponId::SWORD},
	{ItemType::MELEE_WEAPON, WeaponId::SPEAR},
	{ItemType::RANGED_WEAPON, WeaponId::BOW},
	{ItemType::POTION, PotionId::SMALL_HEALTH},
	{ItemType::POTION, PotionId::LARGE_HEALTH},
	{ItemType::POTION, PotionId::STRENGTH},
	{ItemType::POTION, PotionId::ARMOR},
	{ItemType::POTION, PotionId::LIFE},
}};

struct ItemInfo {
	const char* name;
	const char* shortName; // fits the slot's name band
	const char* effect;	   // potions only
	const char* lore1;
	const char* lore2;
};

constexpr std::array<ItemInfo, InvSlot::COUNT> INFO = {{
	{"Club", "Club", "", "Good old club.", "Now with spikes."},
	{"Sword", "Sword", "", "Bronze blade of a", "forgotten guard."},
	{"Spear", "Spear", "", "Long reach.", "None shall pass!"},
	{"Bow", "Bow", "", "The simple bow.", "For slow monsters."},
	{"Small Health", "Small", "Heals 25% of max health", "Bitter herbs from", "the Nile marshes."},
	{"Large Health", "Large", "Heals 50% of max health", "Brewed by the priests", "of Sekhmet."},
	{"Aphethamine", "Might", "Might +2, for good", "It tingles. Best not", "ask what is in it."},
	{"Stone Skin", "Armor", "Armor +2, for good", "Skin as hard as", "temple granite."},
	{"Elixir of Life", "Life", "Max health +5%, full heal", "The breath of Osiris,", "sealed in a flask."},
}};

bool isPotion(int slot) { return slot >= InvSlot::FIRST_POTION; }

Color potionColor(int potionId) {
	int shade = potionId % 3;
	int row = potionId / 3;
	return {1.f - 0.3f * static_cast<float>(shade), 0.6f * static_cast<float>(row), 0.3f * static_cast<float>(shade)};
}

Rect slotRect(int slot) {
	if (!isPotion(slot)) {
		constexpr float W = 19.f;
		constexpr float GAP = 3.f;
		float x0 = ITEMS_PANEL.cx() - (4 * W + 3 * GAP) / 2;
		return {x0 + static_cast<float>(slot) * (W + GAP), WEAPON_ROW_Y, W, 23.f};
	}
	constexpr float W = 15.5f;
	constexpr float GAP = 2.f;
	float x0 = ITEMS_PANEL.cx() - (PotionId::COUNT * W + (PotionId::COUNT - 1) * GAP) / 2;
	int column = slot - InvSlot::FIRST_POTION;
	return {x0 + static_cast<float>(column) * (W + GAP), POTION_ROW_Y, W, 18.f};
}

// Visible canvas area: the 160 x 100 layout plus the margins of a wider or taller window.
Rect visibleArea() {
	float aspect = static_cast<float>(GAME_STATE.render.resX) / static_cast<float>(GAME_STATE.render.resY);
	if (aspect >= CANVAS_W / CANVAS_H) {
		float w = CANVAS_H * aspect;
		return {(CANVAS_W - w) / 2, 0, w, CANVAS_H};
	}
	float h = CANVAS_W / aspect;
	return {0, (CANVAS_H - h) / 2, CANVAS_W, h};
}

void toCanvas(int mouseX, int mouseY, float& x, float& y) {
	Rect area = visibleArea();
	x = area.x + area.w * static_cast<float>(mouseX) / static_cast<float>(GAME_STATE.render.resX);
	y = area.y + area.h - area.h * static_cast<float>(mouseY) / static_cast<float>(GAME_STATE.render.resY);
}

// ---- drawing primitives (texturing off) ------------------------------------

void fillRect(const Rect& r, Color top, Color bottom, float alpha) {
	glBegin(GL_QUADS);
	glColor4f(bottom.r, bottom.g, bottom.b, alpha);
	glVertex2f(r.x, r.y);
	glVertex2f(r.x + r.w, r.y);
	glColor4f(top.r, top.g, top.b, alpha);
	glVertex2f(r.x + r.w, r.y + r.h);
	glVertex2f(r.x, r.y + r.h);
	glEnd();
}

void strokeRect(const Rect& r, Color c, float alpha, float width) {
	glLineWidth(width);
	glColor4f(c.r, c.g, c.b, alpha);
	glBegin(GL_LINE_LOOP);
	glVertex2f(r.x, r.y);
	glVertex2f(r.x + r.w, r.y);
	glVertex2f(r.x + r.w, r.y + r.h);
	glVertex2f(r.x, r.y + r.h);
	glEnd();
	glLineWidth(1);
}

void line(float x0, float y0, float x1, float y1, Color c, float alpha, float width) {
	glLineWidth(width);
	glColor4f(c.r, c.g, c.b, alpha);
	glBegin(GL_LINES);
	glVertex2f(x0, y0);
	glVertex2f(x1, y1);
	glEnd();
	glLineWidth(1);
}

void diamond(float x, float y, float size, Color c, float alpha) {
	glColor4f(c.r, c.g, c.b, alpha);
	glBegin(GL_QUADS);
	glVertex2f(x - size, y);
	glVertex2f(x, y - size);
	glVertex2f(x + size, y);
	glVertex2f(x, y + size);
	glEnd();
}

// Band around `inner`, `grow` wide, fading from alphaIn at the rect to alphaOut at the outer edge.
void ring(const Rect& inner, float grow, Color c, float alphaIn, float alphaOut) {
	glBegin(GL_QUAD_STRIP);
	for (int corner = 0; corner <= 4; corner++) { // counter-clockwise from bottom left, back to the start
		float sx = corner == 1 || corner == 2 ? 1.f : 0.f;
		float sy = corner == 2 || corner == 3 ? 1.f : 0.f;
		glColor4f(c.r, c.g, c.b, alphaIn);
		glVertex2f(inner.x + sx * inner.w, inner.y + sy * inner.h);
		glColor4f(c.r, c.g, c.b, alphaOut);
		glVertex2f(inner.x + sx * inner.w + (2 * sx - 1) * grow, inner.y + sy * inner.h + (2 * sy - 1) * grow);
	}
	glEnd();
}

void ellipse(float x, float y, float rx, float ry, Color c, float alpha) {
	constexpr int SEGMENTS = 32;
	glBegin(GL_TRIANGLE_FAN);
	glColor4f(c.r, c.g, c.b, alpha);
	glVertex2f(x, y);
	glColor4f(c.r, c.g, c.b, 0.f);
	for (int i = 0; i <= SEGMENTS; i++) {
		float a = 2.f * static_cast<float>(M_PI) * static_cast<float>(i) / SEGMENTS;
		glVertex2f(x + rx * std::cos(a), y + ry * std::sin(a));
	}
	glEnd();
}

// Framed panel: gradient fill, bronze outer frame, thin gold inner line and gold corner studs.
void panel(const Rect& r, float alpha) {
	fillRect(r, PANEL_TOP, PANEL_BOTTOM, alpha);
	strokeRect(r, BRONZE, 1.f, 3.f);
	strokeRect(r.inset(1.1f), GOLD_DIM, 0.8f, 1.f);
	diamond(r.x, r.y, 1.2f, GOLD, 1.f);
	diamond(r.x + r.w, r.y, 1.2f, GOLD, 1.f);
	diamond(r.x + r.w, r.y + r.h, 1.2f, GOLD, 1.f);
	diamond(r.x, r.y + r.h, 1.2f, GOLD, 1.f);
}

// ---- text (texturing on) ---------------------------------------------------

// The font sheets have no alpha, so the glyph brightness is used as coverage in two passes:
// first cut the glyph out of what is below, then add the colour into the hole.
void text(Font& font, float x, float y, const char* str, Color c, float alpha = 1.f) {
	glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
	glColor3f(alpha, alpha, alpha);
	font.print(x, y, "%s", str);
	glBlendFunc(GL_ONE, GL_ONE);
	glColor3f(c.r * alpha, c.g * alpha, c.b * alpha);
	font.print(x, y, "%s", str);
}

void textCentered(Font& font, float cx, float y, const char* str, Color c, float alpha = 1.f) {
	text(font, cx - font.TextWidth(str) / 2, y, str, c, alpha);
}

void beginShapes() {
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void beginText() {
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
}
} // namespace

inventory::inventory() {
	title.Load("Fonts/papyrus.png", 7.f, 0.3f, true);
	heading.Load("Fonts/papyrus.png", 5.f, 0.16f, true);
	body.Load("Fonts/papyrus.png", 3.6f, 0.1f, true);
	small.Load("Fonts/papyrus.png", 3.f, 0.08f, true);

	counts[0] = 1; // everyone starts with the club
	show = false;
	for (float& angle : slotAngle)
		angle = REST_ANGLE;
}

inventory::~inventory() {}

item* inventory::SlotItem(int slot) {
	switch (slot) {
	case 0:
		return GAME_STATE.items.club.get();
	case 1:
		return GAME_STATE.items.sword.get();
	case 2:
		return GAME_STATE.items.spear.get();
	case 3:
		return GAME_STATE.items.bow.get();
	default:
		return GAME_STATE.items.potion.get();
	}
}

int inventory::SlotFromItem(int type, int id) {
	for (int slot = 0; slot < InvSlot::COUNT; slot++)
		if (SLOTS[slot].type == type && SLOTS[slot].id == id)
			return slot;
	return InvSlot::NONE;
}

void inventory::GetItem(int type, int id) {
	int slot = SlotFromItem(type, id);
	if (slot == InvSlot::NONE) {
		LOG_ERRORF("ui", "Unknown item type %d id %d", type, id);
		return;
	}
	counts[slot]++;
}

int inventory::Count(int type, int id) const {
	int slot = SlotFromItem(type, id);
	return slot == InvSlot::NONE ? 0 : counts[slot];
}

item* inventory::Equipped() { return SlotItem(equippedSlot); }

int inventory::EquippedType() const { return SLOTS[equippedSlot].type; }

int inventory::EquippedId() const { return SLOTS[equippedSlot].id; }

// ---- actions ---------------------------------------------------------------

bool inventory::CanUse(int slot, const char** reason) const {
	const stats* s = GAME_STATE.ui.Stats.get();
	const char* why = nullptr;
	if (!GAME_STATE.Player->Alive())
		why = "You are dead";
	else if (counts[slot] <= 0)
		why = isPotion(slot) ? "None left" : "Not found yet";
	else if (slot == equippedSlot)
		why = "Equipped";
	else if ((SLOTS[slot].id == PotionId::SMALL_HEALTH || SLOTS[slot].id == PotionId::LARGE_HEALTH) && isPotion(slot) &&
			 s->CurrentHP() >= s->CurrentMaxHP())
		why = "Health is full";

	if (reason != nullptr)
		*reason = why != nullptr ? why : (isPotion(slot) ? "Drink" : "Equip");
	return why == nullptr;
}

void inventory::Use(int slot) {
	if (!CanUse(slot, nullptr))
		return;

	if (isPotion(slot)) {
		DrinkPotion(SLOTS[slot].id);
		return;
	}
	equippedSlot = slot;
	ShowToast(std::string(INFO[slot].name) + " equipped");
}

void inventory::DrinkPotion(int potionId) {
	using PotionAction = void (stats::*)(int);
	struct PotionEffect {
		PotionAction action;
		int delta;
	};
	static const std::array<PotionEffect, PotionId::COUNT> POTION_EFFECTS = {{
		{&stats::Heal, 25},		  // SMALL_HEALTH
		{&stats::Heal, 50},		  // LARGE_HEALTH
		{&stats::GetStronger, 2}, // STRENGTH
		{&stats::GetArmored, 2},  // ARMOR
		{&stats::GetTougher, 5},  // LIFE
	}};

	stats* s = GAME_STATE.ui.Stats.get();
	int hpBefore = s->CurrentHP();

	GAME_STATE.sounds.drink_s.Play();
	counts[InvSlot::FIRST_POTION + potionId]--;
	const auto& effect = POTION_EFFECTS[potionId];
	(s->*(effect.action))(effect.delta);

	char buf[64];
	switch (potionId) {
	case PotionId::STRENGTH:
		snprintf(buf, sizeof(buf), "Might rises to %d", s->CurrentMight());
		break;
	case PotionId::ARMOR:
		snprintf(buf, sizeof(buf), "Armor rises to %d", s->CurrentArmor());
		break;
	case PotionId::LIFE:
		snprintf(buf, sizeof(buf), "Max health rises to %d", s->CurrentMaxHP());
		break;
	default:
		snprintf(buf, sizeof(buf), "Healed %d health", s->CurrentHP() - hpBefore);
		break;
	}
	ShowToast(buf);
}

void inventory::Select(int slot) {
	if (slot >= 0 && slot < InvSlot::COUNT)
		selectedSlot = slot;
}

// Arrow keys: left / right walk the slots in order, up / down jump between the weapon and potion rows.
void inventory::MoveSelection(int dx, int dy) {
	int slot = selectedSlot + dx;
	if (dy != 0) {
		bool onPotions = isPotion(selectedSlot);
		int column = onPotions ? selectedSlot - InvSlot::FIRST_POTION : selectedSlot;
		if (dy < 0 && !onPotions)
			slot = InvSlot::FIRST_POTION + column;
		else if (dy > 0 && onPotions)
			slot = column < InvSlot::FIRST_POTION ? column : InvSlot::FIRST_POTION - 1;
	}
	if (slot >= 0 && slot < InvSlot::COUNT)
		selectedSlot = slot;
}

void inventory::ShowToast(const std::string& text) {
	toast = text;
	toastStartMs = GameClock::now();
}

// ---- input -----------------------------------------------------------------

void inventory::UpdateHover(float x, float y) {
	hoveredSlot = InvSlot::NONE;
	for (int slot = 0; slot < InvSlot::COUNT; slot++)
		if (slotRect(slot).contains(x, y))
			hoveredSlot = slot;
	hoveredButton = BUTTON.contains(x, y);
}

void inventory::MouseMotion(int x, int y) {
	float cx = 0.f;
	float cy = 0.f;
	toCanvas(x, y, cx, cy);
	UpdateHover(cx, cy);
}

// Left click selects a slot (on press) or presses the button (fires on release over it).
// Right click on a slot selects and uses it straight away.
void inventory::MouseFunction(int button, int state, int x, int y) {
	float cx = 0.f;
	float cy = 0.f;
	toCanvas(x, y, cx, cy);
	UpdateHover(cx, cy);

	if (state == GLUT_BUTTON_DOWN) {
		pressedMouseButton = button;
		pressed = Target::None;
		if (hoveredSlot != InvSlot::NONE) {
			pressed = Target::Slot;
			pressedSlot = hoveredSlot;
			Select(hoveredSlot);
		} else if (hoveredButton && button == MOUSE_LEFT_BUTTON) {
			pressed = Target::Button;
		}
		return;
	}

	if (state != GLUT_BUTTON_UP || button != pressedMouseButton)
		return;

	if (pressed == Target::Button && hoveredButton)
		Use(selectedSlot);
	else if (pressed == Target::Slot && button == MOUSE_RIGHT_BUTTON && hoveredSlot == pressedSlot)
		Use(pressedSlot);
	pressed = Target::None;
	pressedSlot = InvSlot::NONE;
}

void inventory::KeyPressed(unsigned char key) {
	switch (key) {
	case KEY_ENTER:
	case KEY_SPACE:
	case 'e':
	case 'E':
		Use(selectedSlot);
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
		if (key >= '1' && key <= '9')
			Select(key - '1');
		return;
	}
}

void inventory::SpecialKeyPressed(int key) {
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

void inventory::Draw() {
	int now = GameClock::now();
	int elapsed = now - lastFrameMs;
	lastFrameMs = now;
	if (elapsed < 0 || elapsed > 100)
		elapsed = 0;
	for (int slot = 0; slot < InvSlot::COUNT; slot++)
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

	DrawBackground();
	for (int slot = 0; slot < InvSlot::COUNT; slot++)
		DrawSlot(slot);
	DrawDetails();
	DrawButton();
	DrawStatus();

	// Models get their own depth buffer so they never cut into the flat UI drawn before them.
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	for (int slot = 0; slot < InvSlot::COUNT; slot++)
		DrawSlotModel(slot);
	DrawDetailModel();
	glDisable(GL_DEPTH_TEST);

	beginText();
	for (int slot = 0; slot < InvSlot::COUNT; slot++)
		DrawSlotLabels(slot);
	DrawFooter();

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glColor3f(1, 1, 1);
	glFlush();

	Scenario::onFrameRendered();
	glutSwapBuffers();
}

void inventory::DrawBackground() {
	Rect area = visibleArea();

	// Carved tomb wall in torchlight.
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	GAME_STATE.textures.load_bg.Bind();
	glColor3f(0.34f, 0.27f, 0.20f);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(area.x, area.y);
	glTexCoord2f(1, 0);
	glVertex2f(area.x + area.w, area.y);
	glTexCoord2f(1, 1);
	glVertex2f(area.x + area.w, area.y + area.h);
	glTexCoord2f(0, 1);
	glVertex2f(area.x, area.y + area.h);
	glEnd();

	beginShapes();
	constexpr float VIGNETTE = 22.f;
	ring(area.inset(VIGNETTE), VIGNETTE, BLACK, 0.f, 0.85f);

	// Title ornament: gold rules ending in diamonds either side of the title.
	float titleHalf = title.TextWidth("Inventory") / 2 + 4;
	constexpr float RULE_Y = 91.5f;
	constexpr float CENTRE = CANVAS_W / 2;
	line(CENTRE - 58, RULE_Y, CENTRE - titleHalf, RULE_Y, GOLD_DIM, 1.f, 2.f);
	line(CENTRE + titleHalf, RULE_Y, CENTRE + 58, RULE_Y, GOLD_DIM, 1.f, 2.f);
	diamond(CENTRE - 58, RULE_Y, 1.1f, GOLD, 1.f);
	diamond(CENTRE + 58, RULE_Y, 1.1f, GOLD, 1.f);
	diamond(CENTRE - titleHalf + 1.5f, RULE_Y, 0.7f, GOLD, 1.f);
	diamond(CENTRE + titleHalf - 1.5f, RULE_Y, 0.7f, GOLD, 1.f);

	panel(ITEMS_PANEL, 0.9f);

	// Section rules under the "Arms" and "Elixirs" headings.
	float headingRight = ITEMS_PANEL.x + 5 + heading.TextWidth("Elixirs") + 2;
	line(headingRight, 78.2f, ITEMS_PANEL.x + ITEMS_PANEL.w - 5, 78.2f, BRONZE, 1.f, 1.f);
	line(headingRight, 44.2f, ITEMS_PANEL.x + ITEMS_PANEL.w - 5, 44.2f, BRONZE, 1.f, 1.f);

	// Papyrus scroll for the details, in a frame matching the items panel.
	glEnable(GL_TEXTURE_2D);
	GAME_STATE.textures.bg.Bind();
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

	beginText();
	textCentered(title, CENTRE, 88.f, "Inventory", GOLD);
	text(heading, ITEMS_PANEL.x + 5, 77.f, "Arms", GOLD);
	text(heading, ITEMS_PANEL.x + 5, 43.f, "Elixirs", GOLD);
	beginShapes();
}

void inventory::DrawSlot(int slot) {
	Rect r = slotRect(slot);
	bool hovered = slot == hoveredSlot;
	bool selected = slot == selectedSlot;
	bool held = pressed == Target::Slot && pressedSlot == slot && hovered;
	bool owned = counts[slot] > 0;

	if (selected) {
		// Soft gold halo that breathes.
		float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GameClock::now()) * 0.004f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		ring(r, 2.6f, GOLD, 0.35f + 0.25f * pulse, 0.f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	Color top = STONE_TOP;
	Color bottom = STONE_BOTTOM;
	if (held) {
		top = STONE_BOTTOM;
		bottom = {0.07f, 0.05f, 0.035f};
	} else if (hovered || selected) {
		top = {0.33f, 0.25f, 0.15f};
		bottom = {0.17f, 0.13f, 0.08f};
	}
	fillRect(r, top, bottom, 1.f);

	if (held) // inner shadow along the top edge: the tile looks pushed in
		fillRect({r.x, r.y + r.h - 1.5f, r.w, 1.5f}, {0, 0, 0}, {0, 0, 0}, 0.45f);

	// Name band: lapis for the weapon in hand, dark stone otherwise.
	Rect band = {r.x, r.y, r.w, NAME_BAND_H};
	if (slot == equippedSlot)
		fillRect(band, LAPIS, LAPIS_DARK, 1.f);
	else
		fillRect(band, {0.09f, 0.07f, 0.05f}, {0.06f, 0.045f, 0.03f}, 1.f);
	line(r.x, r.y + NAME_BAND_H, r.x + r.w, r.y + NAME_BAND_H, selected ? GOLD : BRONZE, 1.f, 1.f);

	if (!owned) // unfound items stay in shadow
		fillRect(r, BLACK, BLACK, 0.35f);

	if (selected) {
		strokeRect(r, GOLD, 1.f, 3.f);
		strokeRect(r.inset(0.9f), GOLD, 0.45f, 1.f);
	} else if (hovered) {
		strokeRect(r, {1.f, 0.88f, 0.55f}, 1.f, 2.f);
	} else {
		strokeRect(r, BRONZE, 1.f, 1.5f);
	}

	// Count badge for stacks.
	if (isPotion(slot) && owned) {
		Rect badge = {r.x + r.w - 5.2f, r.y + r.h - 4.2f, 4.6f, 3.6f};
		fillRect(badge, {0.05f, 0.04f, 0.03f}, {0.05f, 0.04f, 0.03f}, 0.85f);
		strokeRect(badge, GOLD_DIM, 1.f, 1.f);
	}
}

void inventory::DrawSlotModel(int slot) {
	Rect r = slotRect(slot);
	bool held = pressed == Target::Slot && pressedSlot == slot && slot == hoveredSlot;
	item* model = SlotItem(slot);

	Color tint = {1, 1, 1};
	if (isPotion(slot))
		tint = potionColor(SLOTS[slot].id);
	if (counts[slot] <= 0)
		tint = {0.07f, 0.055f, 0.04f};
	else if (slot != selectedSlot && slot != hoveredSlot)
		tint = {tint.r * 0.8f, tint.g * 0.8f, tint.b * 0.8f};

	float savedScale = model->scale;
	float savedAngle = model->rotA;
	model->scale = isPotion(slot) ? POTION_SLOT_SCALE : WEAPON_SLOT_SCALE;
	model->rotA = slotAngle[slot];
	glColor3f(tint.r, tint.g, tint.b);
	glPushMatrix();
	glTranslatef(r.cx(), r.y + NAME_BAND_H + 1.2f - (held ? 0.4f : 0.f), 0);
	model->Draw();
	glPopMatrix();
	model->scale = savedScale;
	model->rotA = savedAngle;
}

void inventory::DrawSlotLabels(int slot) {
	Rect r = slotRect(slot);
	bool owned = counts[slot] > 0;
	bool lit = slot == selectedSlot || slot == hoveredSlot;

	Color nameColor = lit ? GOLD : Color{0.78f, 0.64f, 0.40f};
	if (!owned)
		nameColor = {0.36f, 0.29f, 0.20f};
	textCentered(small, r.cx(), r.y + 0.7f, INFO[slot].shortName, nameColor);

	char key[2] = {static_cast<char>('1' + slot), '\0'};
	text(small, r.x + 1.f, r.y + r.h - 3.8f, key, lit ? GOLD : GOLD_DIM, owned ? 1.f : 0.5f);

	if (isPotion(slot) && owned) {
		char count[8];
		snprintf(count, sizeof(count), "%d", counts[slot]);
		textCentered(small, r.x + r.w - 2.9f, r.y + r.h - 3.9f, count, GOLD);
	}
}

void inventory::DrawDetails() {
	const ItemInfo& info = INFO[selectedSlot];
	bool owned = counts[selectedSlot] > 0;
	float cx = DETAIL_PANEL.cx();

	// Shadow under the model on its plinth line.
	ellipse(cx, PLINTH_Y, 12.f, 1.8f, INK, 0.45f);

	beginText();
	textCentered(heading, cx, 77.5f, info.name, owned ? INK : INK_FADED);
	const char* kind = "Potion";
	if (SLOTS[selectedSlot].type == ItemType::MELEE_WEAPON)
		kind = "Close combat weapon";
	else if (SLOTS[selectedSlot].type == ItemType::RANGED_WEAPON)
		kind = "Ranged weapon";
	textCentered(small, cx, 73.8f, kind, INK_RED);

	constexpr float STAT_Y = 38.5f;
	constexpr float LORE_Y = 29.5f;
	if (!owned) {
		textCentered(body, cx, STAT_Y, isPotion(selectedSlot) ? "Effect unknown" : "Strength unknown", INK_FADED);
	} else if (isPotion(selectedSlot)) {
		textCentered(body, cx, STAT_Y, info.effect, INK);
	} else {
		item* shown = SlotItem(selectedSlot);
		item* current = Equipped();
		char buf[48];
		float labelX = DETAIL_PANEL.x + 9;
		float valueX = DETAIL_PANEL.x + 30;
		auto statRow = [&](float y, const char* label, int value, int currentValue) {
			text(body, labelX, y, label, INK_FADED);
			snprintf(buf, sizeof(buf), "%d", value);
			text(body, valueX, y, buf, INK);
			if (selectedSlot == equippedSlot || value == currentValue)
				return;
			snprintf(buf, sizeof(buf), "%+d", value - currentValue);
			text(body, valueX + body.TextWidth("000") + 1, y, buf, value > currentValue ? INK_GREEN : INK_RED);
		};
		statRow(STAT_Y + 2.f, "Damage", shown->damage, current->damage);
		statRow(STAT_Y - 2.f, "Range", shown->range, current->range);
	}
	if (owned) {
		textCentered(small, cx, LORE_Y, info.lore1, INK_FADED);
		textCentered(small, cx, LORE_Y - 3.4f, info.lore2, INK_FADED);
	} else {
		textCentered(small, cx, LORE_Y, "Still hidden somewhere", INK_FADED);
		textCentered(small, cx, LORE_Y - 3.4f, "in the tomb...", INK_FADED);
	}

	beginShapes();
	line(DETAIL_PANEL.x + 8, 72.5f, DETAIL_PANEL.x + DETAIL_PANEL.w - 8, 72.5f, INK_FADED, 0.8f, 1.f);
	diamond(cx, 72.5f, 0.6f, INK_RED, 1.f);
	line(DETAIL_PANEL.x + 8, 35.f, DETAIL_PANEL.x + DETAIL_PANEL.w - 8, 35.f, INK_FADED, 0.5f, 1.f);
}

void inventory::DrawDetailModel() {
	item* model = SlotItem(selectedSlot);
	bool potion = isPotion(selectedSlot);
	Color tint = potion ? potionColor(SLOTS[selectedSlot].id) : Color{1, 1, 1};
	if (counts[selectedSlot] <= 0)
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

void inventory::DrawButton() {
	const char* label = nullptr;
	bool enabled = CanUse(selectedSlot, &label);
	bool hovered = enabled && hoveredButton;
	bool held = hovered && pressed == Target::Button;
	Rect r = BUTTON;
	float sink = held ? 0.4f : 0.f;

	if (enabled) {
		// Drop shadow, then the lapis tile; it sinks onto the shadow while held.
		fillRect({r.x + 0.5f, r.y - 0.7f, r.w, r.h}, BLACK, BLACK, held ? 0.f : 0.35f);
		r.y -= sink;
		if (hovered) {
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			ring(r, 2.f, GOLD, 0.4f, 0.f);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		Color top = hovered ? Color{0.20f, 0.40f, 0.78f} : LAPIS;
		Color bottom = hovered ? Color{0.09f, 0.20f, 0.46f} : LAPIS_DARK;
		if (held) {
			top = LAPIS_DARK;
			bottom = {0.03f, 0.07f, 0.18f};
		}
		fillRect(r, top, bottom, 1.f);
		if (!held) // top highlight
			fillRect({r.x, r.y + r.h - 1.f, r.w, 1.f}, {1, 1, 1}, {1, 1, 1}, hovered ? 0.18f : 0.1f);
		strokeRect(r, hovered ? Color{1.f, 0.88f, 0.55f} : GOLD, 1.f, 2.5f);
		strokeRect(r.inset(0.8f), GOLD_DIM, 0.6f, 1.f);
	} else {
		fillRect(r, {0.62f, 0.52f, 0.38f}, {0.55f, 0.45f, 0.32f}, 0.6f);
		strokeRect(r, INK_FADED, 0.9f, 1.5f);
	}

	beginText();
	if (enabled)
		textCentered(heading, r.cx(), r.y + 1.3f, label, hovered ? Color{1.f, 0.92f, 0.65f} : GOLD);
	else
		textCentered(body, r.cx(), r.y + 1.9f, label, INK_FADED);
	beginShapes();
}

void inventory::DrawStatus() {
	const stats* s = GAME_STATE.ui.Stats.get();
	constexpr float Y = 15.5f;
	constexpr float BAR_X = 22.f;
	constexpr float BAR_W = 24.f;
	constexpr float BAR_H = 2.6f;
	float x0 = ITEMS_PANEL.x + 5;

	line(x0, 20.5f, ITEMS_PANEL.x + ITEMS_PANEL.w - 5, 20.5f, BRONZE, 1.f, 1.f);

	float ratio =
		static_cast<float>(s->CurrentHP()) / static_cast<float>(s->CurrentMaxHP() > 0 ? s->CurrentMaxHP() : 1);
	ratio = ratio < 0.f ? 0.f : (ratio > 1.f ? 1.f : ratio);
	Rect bar = {BAR_X, Y + 0.3f, BAR_W, BAR_H};
	fillRect(bar, {0.05f, 0.03f, 0.02f}, {0.05f, 0.03f, 0.02f}, 1.f);
	fillRect({bar.x, bar.y, bar.w * ratio, bar.h}, {0.85f, 0.25f, 0.15f}, HEALTH, 1.f);
	strokeRect(bar, GOLD_DIM, 1.f, 1.f);

	char buf[32];
	beginText();
	text(small, x0, Y, "Health", GOLD_DIM);
	snprintf(buf, sizeof(buf), "%d/%d", s->CurrentHP(), s->CurrentMaxHP());
	text(small, BAR_X + BAR_W + 1.5f, Y, buf, GOLD);

	auto stat = [&](float x, const char* label, int value) {
		text(small, x, Y, label, GOLD_DIM);
		snprintf(buf, sizeof(buf), "%d", value);
		text(small, x + small.TextWidth(label) + 1.2f, Y, buf, GOLD);
	};
	stat(58.f, "Might", s->CurrentMight());
	stat(71.f, "Armor", s->CurrentArmor());
	stat(84.f, "Dmg", s->Damage());
	beginShapes();
}

void inventory::DrawFooter() {
	constexpr float CENTRE = CANVAS_W / 2;
	int age = GameClock::now() - toastStartMs;
	if (!toast.empty() && age < TOAST_MS) {
		float alpha = age > TOAST_MS - TOAST_FADE_MS
						  ? static_cast<float>(TOAST_MS - age) / static_cast<float>(TOAST_FADE_MS)
						  : 1.f;
		textCentered(body, CENTRE, 7.2f, toast.c_str(), {1.f, 0.9f, 0.6f}, alpha);
	}
	textCentered(small, CENTRE, 2.2f,
				 "Click: select     Right click / Enter: use     Arrows / 1-9: browse     I / Esc: close",
				 {0.55f, 0.45f, 0.30f});
}

// ---- saves -----------------------------------------------------------------

void inventory::Dump(std::ofstream& f) {
	for (int count : counts)
		f << count << " ";
	f << EquippedType() << " " << EquippedId() << "\n";
}

void inventory::LoadDump(std::ifstream& f) {
	for (int& count : counts)
		f >> count;

	// equipped.type/id were added after older saves were written.
	// Old saves have mapX (a float like "3.32501") at this position.
	// Peek at the next token: if it contains '.', it's mapX — rewind and skip.
	auto pos = f.tellg();
	std::string tok;
	int type = ItemType::MELEE_WEAPON;
	int id = WeaponId::CLUB;
	if ((f >> tok) && tok.find('.') == std::string::npos) {
		type = std::stoi(tok);
		if (!(f >> id))
			f.clear();
	} else {
		f.clear();
		f.seekg(pos);
	}

	int slot = SlotFromItem(type, id);
	equippedSlot = slot != InvSlot::NONE && !isPotion(slot) ? slot : 0;
	selectedSlot = equippedSlot;
}
