#include "player_hud.h"
#include "../core/timer.h"
#include "../graphics/font.h"
#include "../world/level.h"
#include "../world/skills.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace ui;

namespace {
// ---- layout ----
constexpr Rect HEALTH_BAR = {10.f, 18.4f, 27.f, 2.6f};
constexpr Rect STAMINA_BAR = {10.f, 15.6f, 27.f, 1.6f};
constexpr float ICON_X = 6.3f; // heart and bolt, left of the bars
constexpr float NUMBERS_X = 39.f; // "96 / 134"
constexpr float NUMBERS_DROP = 2.5f; // pen y below the bar centre, so the digits sit in the middle
constexpr float SLOT_X = 5.f; // first quick slot
constexpr float SLOT_Y = 8.2f;
constexpr float SLOT_SIZE = 6.6f;
constexpr float SLOT_STEP = 8.2f;
constexpr float CAP_Y = 4.6f; // key cap under each slot
constexpr float SOCKET_X = 40.5f; // first key socket centre, right of the amulet slot
constexpr float SOCKET_STEP = 4.5f;
constexpr float SOCKET_Y = 11.5f;
constexpr float SOCKET_SIZE = 1.9f;
constexpr Rect XP_LINE = {4.f, 2.9f, 52.f, 0.7f};
constexpr float POISON_Y = PlayerHud::PANEL.y + PlayerHud::PANEL.h + 4.6f; // the poison tiles, above the panel
constexpr float POISON_X = PlayerHud::PANEL.x + 1.f;
constexpr float POISON_SIZE = 6.f;
constexpr float POISON_STEP = 9.f;
// The waiting heka, above the panel's right end: clear of the poison and resistance tiles.
constexpr Rect HEKA_TILE = {PlayerHud::PANEL.x + PlayerHud::PANEL.w - 7.f, POISON_Y, 6.f, 6.f};
constexpr Rect STAMINA_SKILL_SIGN = {51.f, 13.6f, 3.6f, 3.6f}; // right of the stamina bar, under the health numbers
constexpr Color FLAME = {0.98f, 0.45f, 0.1f};
constexpr float HEKA_PULSE_RATE = 0.004f; // radians per ms
constexpr float ICON_INSET = 0.3f; // the icon in its slot
constexpr int ICON_COLUMNS = 8; // atlas grid
constexpr int ICON_ROWS = 6;

// ---- timing ----
constexpr float LOW_HEALTH = 0.25f; // under this the health bar pulses
constexpr int REFUSED_FLASH_MS = 600;
constexpr int DRINK_FLASH_MS = 500;

using PlayerHud::BLOOD_BOTTOM;
using PlayerHud::BLOOD_TOP;
using PlayerHud::CAP_BOTTOM;
using PlayerHud::CAP_H;
using PlayerHud::CAP_PAD;
using PlayerHud::CAP_PEN_DROP;
using PlayerHud::CAP_TOP;
using PlayerHud::POISON_BOTTOM;
using PlayerHud::POISON_TOP;
constexpr Color LOST = {0.95f, 0.62f, 0.42f};
// The drops, weak to strong: pale lime, venom green, deep malachite.
constexpr Color POISON_DROPS[POISON_TIER_COUNT] = {{0.72f, 0.9f, 0.3f}, {0.35f, 0.78f, 0.2f}, {0.08f, 0.5f, 0.18f}};
constexpr Color CURSE_FRAME_TOP = {0.52f, 0.22f, 0.62f}; // a curse tile's frame, so it does not read as a potion
constexpr Color CURSE_FRAME_BOTTOM = {0.22f, 0.06f, 0.3f};
constexpr Color RESIST_DROPS[2] = {{0.35f, 0.62f, 0.92f}, {0.12f, 0.22f, 0.78f}}; // lesser, greater (ITEMS)
constexpr Color WATER_DROP = {0.08f, 0.52f, 0.68f}; // the Blessing of Hapi's, as its potion (ITEMS)
constexpr Color AMBER_TOP = {0.98f, 0.76f, 0.26f};
constexpr Color AMBER_BOTTOM = {0.58f, 0.38f, 0.07f};
constexpr Color TROUGH = {0.05f, 0.03f, 0.02f};
constexpr Color BADGE = {0.05f, 0.04f, 0.03f};
// Key gems in LOCK_COLOURS order: carnelian, lapis, turquoise, amber.
constexpr Color GEMS[LOCK_COLOUR_COUNT] =
	{{0.85f, 0.2f, 0.12f}, {0.2f, 0.35f, 0.95f}, {0.15f, 0.8f, 0.6f}, {1.f, 0.78f, 0.2f}};

PlayerHud::DamageTrail trail;

float ratioOf(int value, int max) {
	return max > 0 ? std::clamp(static_cast<float>(value) / static_cast<float>(max), 0.f, 1.f) : 0.f;
}

// 0..1, fading out over `lengthMs`.
float fade(int ageMs, int lengthMs) {
	return ageMs >= 0 && ageMs < lengthMs ? 1.f - static_cast<float>(ageMs) / static_cast<float>(lengthMs) : 0.f;
}

void additiveRing(const Rect& r, float grow, Color c, float alpha) {
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	ring(r, grow, c, alpha, 0.f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void heart(float x, float y, Color c) {
	ellipse(x - 0.55f, y + 0.3f, 0.7f, 0.7f, c, 1.f);
	ellipse(x + 0.55f, y + 0.3f, 0.7f, 0.7f, c, 1.f);
	triangle(x - 1.22f, y + 0.12f, x + 1.22f, y + 0.12f, x, y - 1.2f, c, 1.f);
}

void bolt(float x, float y, Color c) {
	triangle(x + 0.45f, y + 1.3f, x - 0.75f, y - 0.15f, x + 0.15f, y - 0.15f, c, 1.f);
	triangle(x - 0.15f, y + 0.15f, x + 0.75f, y + 0.15f, x - 0.45f, y - 1.3f, c, 1.f);
}

void barFrame(const Rect& bar) {
	strokeRect(bar, GOLD_DIM, 1.f, 1.5f);
	diamond(bar.x, bar.cy(), 0.9f, GOLD, 1.f);
	diamond(bar.x + bar.w, bar.cy(), 0.9f, GOLD, 1.f);
}

bool poisoned(const PlayerHud::View& v) {
	for (int left : v.poisonLeftMs)
		if (left > 0)
			return true;
	return false;
}

void drawHealth(const PlayerHud::View& v, int now) {
	const bool poison = poisoned(v);
	const Color top = poison ? POISON_TOP : BLOOD_TOP;
	const Color bottom = poison ? POISON_BOTTOM : BLOOD_BOTTOM;
	float ratio = ratioOf(v.hp, v.maxHp);
	float lost = std::max(trail.shown, ratio);
	const Rect& bar = HEALTH_BAR;
	if (ratio < LOW_HEALTH && v.hp > 0) {
		float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(now) * 0.008f);
		additiveRing(bar, 1.8f, top, 0.25f + 0.45f * pulse);
	}
	fillRect(bar, TROUGH, TROUGH, 1.f);
	fillRect({bar.x + bar.w * ratio, bar.y, bar.w * (lost - ratio), bar.h}, LOST, LOST, 0.9f);
	fillRect({bar.x, bar.y, bar.w * ratio, bar.h}, top, bottom, 1.f);
	fillRect({bar.x, bar.y + bar.h - 0.6f, bar.w * ratio, 0.6f}, {1, 1, 1}, {1, 1, 1}, 0.12f); // shine
	barFrame(bar);
	heart(ICON_X, bar.cy(), top);
}

// A drop of poison, its round belly at (x, y): solid (ellipse() is a soft glow).
void drop(float x, float y, float r, Color c) {
	constexpr int SEGMENTS = 16;
	for (int i = 0; i < SEGMENTS; i++) {
		const float a0 = 2.f * static_cast<float>(M_PI) * static_cast<float>(i) / SEGMENTS;
		const float a1 = 2.f * static_cast<float>(M_PI) * static_cast<float>(i + 1) / SEGMENTS;
		triangle(x, y, x + r * std::cos(a0), y + r * std::sin(a0), x + r * std::cos(a1), y + r * std::sin(a1), c, 1.f);
	}
	triangle(x - r * 0.92f, y + r * 0.4f, x + r * 0.92f, y + r * 0.4f, x, y + r * 2.1f, c, 1.f);
	ellipse(x - r * 0.35f, y + r * 0.2f, r * 0.28f, r * 0.28f, {1, 1, 1}, 0.55f); // glint
}

Rect poisonRect(int i) { return {POISON_X + POISON_STEP * static_cast<float>(i), POISON_Y, POISON_SIZE, POISON_SIZE}; }

// The running tiers left to right, weak first: a stone tile with the drop and one pip per tier step.
void drawPoison(const PlayerHud::View& v) {
	int shown = 0;
	for (int t = 0; t < POISON_TIER_COUNT; t++) {
		if (v.poisonLeftMs[t] <= 0)
			continue;
		const Rect r = poisonRect(shown++);
		fillRect({r.x + 0.5f, r.y - 0.6f, r.w, r.h}, BLACK, BLACK, 0.45f); // drop shadow
		tile(r, TileStyle::Stone, false, false);
		drop(r.cx(), r.y + 2.8f, 1.3f, POISON_DROPS[t]);
		for (int p = 0; p <= t; p++)
			diamond(
				r.cx() + (static_cast<float>(p) - static_cast<float>(t) / 2.f) * 1.4f,
				r.y + 0.9f,
				0.45f,
				GOLD_BRIGHT,
				1.f
			);
	}
}

int poisonTiles(const PlayerHud::View& v) {
	int shown = 0;
	for (int left : v.poisonLeftMs)
		shown += left > 0 ? 1 : 0;
	return shown;
}

// A running resistance potion, right of the poison tiles: a blue drop, one pip lesser, two greater.
void drawResist(const PlayerHud::View& v) {
	if (v.resistLeftMs <= 0)
		return;
	const Rect r = poisonRect(poisonTiles(v));
	const int pips = v.resistGreater ? 2 : 1;
	fillRect({r.x + 0.5f, r.y - 0.6f, r.w, r.h}, BLACK, BLACK, 0.45f); // drop shadow
	tile(r, TileStyle::Stone, false, false);
	drop(r.cx(), r.y + 2.8f, 1.3f, RESIST_DROPS[pips - 1]);
	for (int p = 0; p < pips; p++)
		diamond(
			r.cx() + (static_cast<float>(p) - static_cast<float>(pips - 1) / 2.f) * 1.4f,
			r.y + 0.9f,
			0.45f,
			GOLD_BRIGHT,
			1.f
		);
}

// A running Blessing of Hapi (plan water-blessing-potion), right of the resistance tile: a blue-green drop.
void drawWaterBlessing(const PlayerHud::View& v) {
	if (v.waterLeftMs <= 0)
		return;
	const Rect r = poisonRect(poisonTiles(v) + (v.resistLeftMs > 0 ? 1 : 0));
	fillRect({r.x + 0.5f, r.y - 0.6f, r.w, r.h}, BLACK, BLACK, 0.45f); // drop shadow
	tile(r, TileStyle::Stone, false, false);
	drop(r.cx(), r.y + 2.8f, 1.3f, WATER_DROP);
}

// The poison, resistance and water blessing tiles shown, left of the curses.
int potionTiles(const PlayerHud::View& v) {
	return poisonTiles(v) + (v.resistLeftMs > 0 ? 1 : 0) + (v.waterLeftMs > 0 ? 1 : 0);
}

// The running curses right of the resistance, in Curse order: a stone tile in a purple frame with the curse's icon, one
// pip lesser, two greater.
void drawCurses(const PlayerHud::View& v, int icons) {
	int shown = potionTiles(v);
	for (int c = 0; c < CURSE_COUNT; c++) {
		if (v.curseLeftMs[c] <= 0)
			continue;
		const Rect r = poisonRect(shown++);
		fillRect({r.x + 0.5f, r.y - 0.6f, r.w, r.h}, BLACK, BLACK, 0.45f); // drop shadow
		fillRect({r.x - 0.6f, r.y - 0.6f, r.w + 1.2f, r.h + 1.2f}, CURSE_FRAME_TOP, CURSE_FRAME_BOTTOM, 1.f);
		tile(r, TileStyle::Stone, false, false);
		atlasIcon(
			{r.x + 0.6f, r.y + 1.3f, r.w - 1.2f, r.h - 1.6f},
			icons,
			static_cast<int>(PlayerHud::Icon::FirstCurse) + c,
			ICON_COLUMNS,
			ICON_ROWS,
			{1, 1, 1}
		);
		beginShapes();
		const int pips = v.curseGreater[c] ? 2 : 1;
		for (int p = 0; p < pips; p++)
			diamond(
				r.cx() + (static_cast<float>(p) - static_cast<float>(pips - 1) / 2.f) * 1.4f,
				r.y + 0.9f,
				0.45f,
				GOLD_BRIGHT,
				1.f
			);
	}
}

// A heka waits to be spent: a lapis tile with the heka mark and a breathing gold halo, the Heka key under it.
void drawHeka(const PlayerHud::View& v, int now, int skillIcons) {
	if (!v.hekaWaiting)
		return;
	const Rect& r = HEKA_TILE;
	const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(now) * HEKA_PULSE_RATE);
	fillRect({r.x + 0.5f, r.y - 0.6f, r.w, r.h}, BLACK, BLACK, 0.45f); // drop shadow
	additiveRing(r, 1.6f, GOLD, 0.25f + 0.35f * pulse);
	tile(r, TileStyle::Lapis, false, false);
	atlasIcon(r.inset(0.5f), skillIcons, HEKA_ICON, SKILL_ICON_COLUMNS, SKILL_ICON_ROWS, {1, 1, 1});
	beginShapes();
}

// The stamina-skill toggle's sign: a flame on a lapis tile while on, on stone while off; a refused skill blinks it red.
void drawStaminaSkill(const PlayerHud::View& v) {
	if (!v.staminaSkillShown)
		return;
	const Rect& r = STAMINA_SKILL_SIGN;
	const float refused = fade(v.staminaSkillRefusedAgeMs, REFUSED_FLASH_MS);
	if (refused > 0.f) {
		const float blink = 0.5f + 0.5f * std::cos(static_cast<float>(v.staminaSkillRefusedAgeMs) * 0.03f);
		additiveRing(r, 1.2f, BLOOD_TOP, refused * blink);
	}
	tile(r, v.staminaSkillOn ? TileStyle::Lapis : TileStyle::Stone, false, false);
	const Color flame = v.staminaSkillOn ? FLAME : Color{FLAME.r * 0.4f, FLAME.g * 0.4f, FLAME.b * 0.4f};
	drop(r.cx(), r.y + 1.5f, 0.95f, flame);
}

// Seconds left over a status tile.
void tileSeconds(const Rect& r, int leftMs, Font& small) {
	char secs[12];
	snprintf(secs, sizeof(secs), "%ds", (leftMs + 999) / 1000);
	textCentered(small, r.cx() + 0.2f, r.y - 3.4f - 0.2f, secs, BLACK);
	textCentered(small, r.cx(), r.y - 3.4f, secs, GOLD_BRIGHT);
}

void drawPoisonText(const PlayerHud::View& v, Font& small) {
	int shown = 0;
	for (int t = 0; t < POISON_TIER_COUNT; t++)
		if (v.poisonLeftMs[t] > 0)
			tileSeconds(poisonRect(shown++), v.poisonLeftMs[t], small);
	if (v.resistLeftMs > 0)
		tileSeconds(poisonRect(shown++), v.resistLeftMs, small);
	if (v.waterLeftMs > 0)
		tileSeconds(poisonRect(shown++), v.waterLeftMs, small);
	for (int c = 0; c < CURSE_COUNT; c++)
		if (v.curseLeftMs[c] > 0)
			tileSeconds(poisonRect(shown++), v.curseLeftMs[c], small);
}

void drawStamina(const PlayerHud::View& v) {
	float ratio = ratioOf(v.stamina, v.maxStamina);
	const Rect& bar = STAMINA_BAR;
	float refused = fade(v.staminaRefusedAgeMs, REFUSED_FLASH_MS);
	if (refused > 0.f) {
		float blink = 0.5f + 0.5f * std::cos(static_cast<float>(v.staminaRefusedAgeMs) * 0.03f);
		additiveRing(bar, 1.5f, BLOOD_TOP, refused * blink);
	}
	fillRect(bar, TROUGH, TROUGH, 1.f);
	fillRect({bar.x, bar.y, bar.w * ratio, bar.h}, AMBER_TOP, AMBER_BOTTOM, 1.f);
	if (refused > 0.f)
		fillRect(bar, BLOOD_TOP, BLOOD_TOP, 0.35f * refused);
	barFrame(bar);
	bolt(ICON_X, bar.cy(), AMBER_TOP);
}

constexpr int SLOT_COUNT = 4;
constexpr int AMULET_SLOT = 3;
// The amulet slot only while one is worn; the others always (an empty potion slot shows none is left).
bool shown(const PlayerHud::View& v, int i) { return i != AMULET_SLOT || v.slots[i].icon != PlayerHud::Icon::None; }

Rect slotRect(int i) { return {SLOT_X + SLOT_STEP * static_cast<float>(i), SLOT_Y, SLOT_SIZE, SLOT_SIZE}; }

Rect badgeRect(const Rect& slot) { return {slot.x + slot.w - 3.1f, slot.y + slot.h - 2.6f, 3.3f, 2.8f}; }

Rect capRect(const Rect& slot, Font& small, const char* key) {
	float w = small.TextWidth(key) + CAP_PAD;
	return {slot.cx() - w / 2, CAP_Y, w, CAP_H};
}

void drawSlot(const PlayerHud::Slot& s, const Rect& r, Font& small) {
	float flash = fade(s.flashAgeMs, DRINK_FLASH_MS);
	if (flash > 0.f)
		additiveRing(r, 2.2f, GOLD, 0.8f * flash);
	tile(r, TileStyle::Stone, flash > 0.f, false);
	if (s.icon != PlayerHud::Icon::None && (s.count >= 0 || !s.badge.empty())) {
		Rect badge = badgeRect(r);
		if (!s.badge.empty()) // wider for "III"
			badge = {
				badge.x + badge.w - small.TextWidth(s.badge.c_str()) - 1.6f,
				badge.y,
				small.TextWidth(s.badge.c_str()) + 1.6f,
				badge.h
			};
		fillRect(badge, BADGE, BADGE, 0.85f);
		strokeRect(badge, GOLD_DIM, 1.f, 1.f);
	}
	if (s.key.empty())
		return;
	Rect cap = capRect(r, small, s.key.c_str());
	fillRect({cap.x, cap.y - 0.25f, cap.w, cap.h}, BLACK, BLACK, 0.5f);
	fillRect(cap, CAP_TOP, CAP_BOTTOM, 1.f);
	strokeRect(cap, GOLD_DIM, 1.f, 1.f);
}

void drawSlotIcon(const PlayerHud::Slot& s, const Rect& r, int icons) {
	if (s.icon != PlayerHud::Icon::None)
		PlayerHud::drawIcon(s.icon, r.inset(ICON_INSET), icons, s.tint);
}

void drawSlotText(const PlayerHud::Slot& s, const Rect& r, Font& small) {
	if (!s.key.empty()) {
		Rect cap = capRect(r, small, s.key.c_str());
		textCentered(small, cap.cx(), cap.cy() - CAP_PEN_DROP + 0.5f, s.key.c_str(), GOLD);
	}
	if (s.icon != PlayerHud::Icon::None && !s.badge.empty()) {
		Rect badge = badgeRect(r);
		const float w = small.TextWidth(s.badge.c_str()) + 1.6f;
		textCentered(small, badge.x + badge.w - w / 2, badge.cy() - CAP_PEN_DROP + 0.5f, s.badge.c_str(), GOLD);
	} else if (s.icon != PlayerHud::Icon::None && s.count >= 0) {
		char count[12];
		snprintf(count, sizeof(count), "%d", s.count);
		Rect badge = badgeRect(r);
		textCentered(small, badge.cx(), badge.cy() - CAP_PEN_DROP + 0.5f, count, GOLD);
	}
}

// One socket per key the level has, the held ones set with their gem.
void drawKeys(const PlayerHud::View& v) {
	float x = SOCKET_X;
	for (int c = 0; c < LOCK_COLOUR_COUNT; c++) {
		int bit = 1 << c;
		if (((v.levelKeys | v.keysHeld) & bit) == 0)
			continue;
		diamond(x + 0.25f, SOCKET_Y - 0.35f, SOCKET_SIZE + 0.5f, BLACK, 0.5f); // shadow
		diamond(x, SOCKET_Y, SOCKET_SIZE + 0.5f, GOLD_DIM, 1.f); // bezel
		if ((v.keysHeld & bit) != 0) {
			const Color& gem = GEMS[c];
			diamond(x, SOCKET_Y, SOCKET_SIZE, {gem.r * 0.55f, gem.g * 0.55f, gem.b * 0.55f}, 1.f);
			diamond(x, SOCKET_Y + 0.15f, SOCKET_SIZE * 0.72f, gem, 1.f);
			diamond(x - 0.45f, SOCKET_Y + 0.55f, 0.35f, {1, 1, 1}, 0.8f); // glint
		} else {
			diamond(x, SOCKET_Y, SOCKET_SIZE, TROUGH, 1.f);
		}
		x += SOCKET_STEP;
	}
}

void drawXp(const PlayerHud::View& v) {
	fillRect(XP_LINE, TROUGH, TROUGH, 0.9f);
	fillRect(
		{XP_LINE.x, XP_LINE.y, XP_LINE.w * std::clamp(v.xpRatio, 0.f, 1.f), XP_LINE.h},
		GOLD_BRIGHT,
		GOLD_DIM,
		1.f
	);
}
} // namespace

namespace PlayerHud {
void tick(int hp, int maxHp) { trail.update(ratioOf(hp, maxHp), GameClock::now()); }

void reset() { trail = PlayerHud::DamageTrail{}; }

void drawIcon(Icon icon, const Rect& r, int icons, Color tint) {
	atlasIcon(r, icons, static_cast<int>(icon), ICON_COLUMNS, ICON_ROWS, tint);
}

void draw(const View& view, int resX, int resY, Font& numbers, Font& small, int icons, int skillIcons) {
	int now = GameClock::now();
	// Font::print resets the modelview, so the canvas is set through the projection.
	float canvasH = 100.f / SCALE;
	beginSquareCanvas(canvasH, resX, resY, 200.f);
	glLoadIdentity();

	beginShapes();
	fillRect({PANEL.x + 0.7f, PANEL.y - 0.9f, PANEL.w, PANEL.h}, BLACK, BLACK, 0.45f); // drop shadow
	panel(PANEL, 0.9f);
	drawHealth(view, now);
	drawStamina(view);
	drawStaminaSkill(view);
	for (int i = 0; i < SLOT_COUNT; i++)
		if (shown(view, i))
			drawSlot(view.slots[i], slotRect(i), small);
	drawKeys(view);
	drawXp(view);
	drawPoison(view);
	drawResist(view);
	drawWaterBlessing(view);
	drawCurses(view, icons);
	drawHeka(view, now, skillIcons);
	for (int i = 0; i < SLOT_COUNT; i++)
		if (shown(view, i))
			drawSlotIcon(view.slots[i], slotRect(i), icons);

	beginText();
	char hp[24];
	snprintf(hp, sizeof(hp), "%d / %d", std::max(view.hp, 0), view.maxHp);
	float y = HEALTH_BAR.cy() - NUMBERS_DROP;
	text(numbers, NUMBERS_X + 0.3f, y - 0.3f, hp, BLACK);
	text(numbers, NUMBERS_X, y, hp, GOLD_BRIGHT);
	for (int i = 0; i < SLOT_COUNT; i++)
		if (shown(view, i))
			drawSlotText(view.slots[i], slotRect(i), small);
	drawPoisonText(view, small);
	if (view.staminaSkillShown && !view.staminaSkillKey.empty())
		textCentered(
			small,
			STAMINA_SKILL_SIGN.x - 2.2f,
			STAMINA_SKILL_SIGN.y + 0.6f,
			view.staminaSkillKey.c_str(),
			view.staminaSkillOn ? GOLD_BRIGHT : LABEL_DIM
		);
	if (view.hekaWaiting && !view.hekaKey.empty()) {
		textCentered(small, HEKA_TILE.cx() + 0.2f, HEKA_TILE.y - 3.6f, view.hekaKey.c_str(), BLACK);
		textCentered(small, HEKA_TILE.cx(), HEKA_TILE.y - 3.4f, view.hekaKey.c_str(), GOLD_BRIGHT);
	}

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glColor3f(1, 1, 1);
}
} // namespace PlayerHud
