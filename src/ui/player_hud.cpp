#include "player_hud.h"
#include "../core/timer.h"
#include "../graphics/font.h"
#include "../world/level.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace ui;

namespace {
// ---- layout ----
constexpr Rect HEALTH_BAR = {10.f, 18.4f, 27.f, 2.6f};
constexpr Rect STAMINA_BAR = {10.f, 15.6f, 27.f, 1.6f};
constexpr float ICON_X = 6.3f;		 // heart and bolt, left of the bars
constexpr float NUMBERS_X = 39.f;	 // "96 / 134"
constexpr float NUMBERS_DROP = 2.5f; // pen y below the bar centre, so the digits sit in the middle
constexpr float SLOT_X = 5.f;		 // first quick slot
constexpr float SLOT_Y = 8.2f;
constexpr float SLOT_SIZE = 6.6f;
constexpr float SLOT_STEP = 8.2f;
constexpr float CAP_Y = 4.6f; // key cap under each slot
constexpr float CAP_H = 2.6f;
constexpr float CAP_PEN_DROP = 1.8f; // cap centre to the small font's pen y
constexpr float SOCKET_X = 34.f;	 // first key socket centre
constexpr float SOCKET_STEP = 4.5f;
constexpr float SOCKET_Y = 11.5f;
constexpr float SOCKET_SIZE = 1.9f;
constexpr Rect XP_LINE = {4.f, 2.9f, 47.f, 0.7f};
constexpr float POISON_Y = PlayerHud::PANEL.y + PlayerHud::PANEL.h + 4.6f; // the poison tiles, above the panel
constexpr float POISON_X = PlayerHud::PANEL.x + 1.f;
constexpr float POISON_SIZE = 6.f;
constexpr float POISON_STEP = 9.f;
constexpr float ICON_INSET = 0.3f; // the icon in its slot
constexpr int ICON_COLUMNS = 4;	   // atlas grid
constexpr int ICON_ROWS = 2;

// ---- timing ----
constexpr int TRAIL_HOLD_MS = 500;			 // the lost part stays this long after the last hit...
constexpr float TRAIL_DRAIN_PER_MS = 0.001f; // ...then drains at this ratio per ms
constexpr float LOW_HEALTH = 0.25f;			 // under this the health bar pulses
constexpr int REFUSED_FLASH_MS = 600;
constexpr int DRINK_FLASH_MS = 500;

constexpr Color BLOOD_TOP = {0.85f, 0.14f, 0.08f}; // the boss bar's
constexpr Color BLOOD_BOTTOM = {0.45f, 0.05f, 0.03f};
constexpr Color LOST = {0.95f, 0.62f, 0.42f};
constexpr Color POISON_TOP = {0.45f, 0.82f, 0.2f}; // the health bar while poisoned
constexpr Color POISON_BOTTOM = {0.1f, 0.36f, 0.05f};
// The drops, weak to strong: pale lime, venom green, deep malachite.
constexpr Color POISON_DROPS[POISON_TIER_COUNT] = {{0.72f, 0.9f, 0.3f}, {0.35f, 0.78f, 0.2f}, {0.08f, 0.5f, 0.18f}};
constexpr Color AMBER_TOP = {0.98f, 0.76f, 0.26f};
constexpr Color AMBER_BOTTOM = {0.58f, 0.38f, 0.07f};
constexpr Color TROUGH = {0.05f, 0.03f, 0.02f};
constexpr Color CAP_TOP = {0.19f, 0.15f, 0.10f}; // the options key caps
constexpr Color CAP_BOTTOM = {0.10f, 0.08f, 0.055f};
constexpr Color BADGE = {0.05f, 0.04f, 0.03f};
// Key gems in LOCK_COLOURS order: carnelian, lapis, turquoise, amber.
constexpr Color GEMS[LOCK_COLOUR_COUNT] = {
	{0.85f, 0.2f, 0.12f}, {0.2f, 0.35f, 0.95f}, {0.15f, 0.8f, 0.6f}, {1.f, 0.78f, 0.2f}};

// The health bar's lost part: after a hit it holds the old health for TRAIL_HOLD_MS, then drains down.
struct DamageTrail {
	bool started = false;
	float shown = 1.f; // the trail's end
	float last = 1.f;  // health ratio of the last frame
	int hitMs = 0;
	int lastMs = 0;

	float update(float ratio, int now) {
		if (!started) {
			started = true;
			shown = last = ratio;
			lastMs = now;
		}
		if (ratio < last)
			hitMs = now;
		if (now - hitMs > TRAIL_HOLD_MS)
			shown -= TRAIL_DRAIN_PER_MS * static_cast<float>(now - lastMs);
		shown = std::max(shown, ratio); // a heal jumps past it
		last = ratio;
		lastMs = now;
		return shown;
	}
} trail;

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
			diamond(r.cx() + (static_cast<float>(p) - static_cast<float>(t) / 2.f) * 1.4f, r.y + 0.9f, 0.45f,
					GOLD_BRIGHT, 1.f);
	}
}

void drawPoisonText(const PlayerHud::View& v, Font& small) {
	int shown = 0;
	for (int t = 0; t < POISON_TIER_COUNT; t++) {
		if (v.poisonLeftMs[t] <= 0)
			continue;
		const Rect r = poisonRect(shown++);
		char secs[12];
		snprintf(secs, sizeof(secs), "%ds", (v.poisonLeftMs[t] + 999) / 1000);
		textCentered(small, r.cx() + 0.2f, r.y - 3.4f - 0.2f, secs, BLACK);
		textCentered(small, r.cx(), r.y - 3.4f, secs, GOLD_BRIGHT);
	}
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

Rect slotRect(int i) { return {SLOT_X + SLOT_STEP * static_cast<float>(i), SLOT_Y, SLOT_SIZE, SLOT_SIZE}; }

Rect badgeRect(const Rect& slot) { return {slot.x + slot.w - 3.1f, slot.y + slot.h - 2.6f, 3.3f, 2.8f}; }

Rect capRect(const Rect& slot, Font& small, const char* key) {
	float w = small.TextWidth(key) + 2.2f;
	return {slot.cx() - w / 2, CAP_Y, w, CAP_H};
}

void drawSlot(const PlayerHud::Slot& s, const Rect& r, Font& small) {
	float flash = fade(s.flashAgeMs, DRINK_FLASH_MS);
	if (flash > 0.f)
		additiveRing(r, 2.2f, GOLD, 0.8f * flash);
	tile(r, TileStyle::Stone, flash > 0.f, false);
	if (s.icon != PlayerHud::Icon::None && s.count >= 0) {
		Rect badge = badgeRect(r);
		fillRect(badge, BADGE, BADGE, 0.85f);
		strokeRect(badge, GOLD_DIM, 1.f, 1.f);
	}
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
	Rect cap = capRect(r, small, s.key.c_str());
	textCentered(small, cap.cx(), cap.cy() - CAP_PEN_DROP + 0.5f, s.key.c_str(), GOLD);
	if (s.icon != PlayerHud::Icon::None && s.count >= 0) {
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
		diamond(x, SOCKET_Y, SOCKET_SIZE + 0.5f, GOLD_DIM, 1.f);			   // bezel
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
	fillRect({XP_LINE.x, XP_LINE.y, XP_LINE.w * std::clamp(v.xpRatio, 0.f, 1.f), XP_LINE.h}, GOLD_BRIGHT, GOLD_DIM,
			 1.f);
}
} // namespace

namespace PlayerHud {
void tick(int hp, int maxHp) { trail.update(ratioOf(hp, maxHp), GameClock::now()); }

void reset() { trail = DamageTrail{}; }

void drawIcon(Icon icon, const Rect& r, int icons, Color tint) {
	int cell = static_cast<int>(icon);
	int row = cell / ICON_COLUMNS;
	float w = 1.f / ICON_COLUMNS;
	float h = 1.f / ICON_ROWS;
	float u = w * static_cast<float>(cell % ICON_COLUMNS);
	float v = 1.f - h * static_cast<float>(row + 1); // the PNG's first row is the top
	texturedRect(r, icons, tint, u, v, u + w, v + h);
}

void draw(const View& view, int resX, int resY, Font& numbers, Font& small, int icons) {
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
	for (int i = 0; i < 3; i++)
		drawSlot(view.slots[i], slotRect(i), small);
	drawKeys(view);
	drawXp(view);
	drawPoison(view);
	for (int i = 0; i < 3; i++)
		drawSlotIcon(view.slots[i], slotRect(i), icons);

	beginText();
	char hp[24];
	snprintf(hp, sizeof(hp), "%d / %d", std::max(view.hp, 0), view.maxHp);
	float y = HEALTH_BAR.cy() - NUMBERS_DROP;
	text(numbers, NUMBERS_X + 0.3f, y - 0.3f, hp, BLACK);
	text(numbers, NUMBERS_X, y, hp, GOLD_BRIGHT);
	for (int i = 0; i < 3; i++)
		drawSlotText(view.slots[i], slotRect(i), small);
	drawPoisonText(view, small);

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glColor3f(1, 1, 1);
}
} // namespace PlayerHud
