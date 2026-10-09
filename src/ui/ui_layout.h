#ifndef UI_LAYOUT_H
#define UI_LAYOUT_H

// Where the UI parts go and how they fade, without GL (docs/plan/scenarios-to-unit-tests.md technique 5): the canvas,
// its rects and the window-to-canvas mapping, the screen tabs, the status box and the level gem. The drawing is
// ui_draw.h and the screens'; the unit tests check the layouts at any window size (tests/unit/ui_layout_test.cpp).

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace ui {

// The menu, inventory and riddle screens lay out on this canvas (y up); the draft map uses its height.
constexpr float CANVAS_W = 160.f;
constexpr float CANVAS_H = 100.f;

struct Rect {
	float x, y, w, h;
	[[nodiscard]] bool contains(float px, float py) const { return px >= x && px <= x + w && py >= y && py <= y + h; }
	[[nodiscard]] float cx() const { return x + w / 2; }
	[[nodiscard]] float cy() const { return y + h / 2; }
	[[nodiscard]] Rect inset(float d) const { return {x + d, y + d, w - 2 * d, h - 2 * d}; }
	// Inside `outer`, edges included.
	[[nodiscard]] bool within(const Rect& outer) const {
		return x >= outer.x && y >= outer.y && x + w <= outer.x + outer.w && y + h <= outer.y + outer.h;
	}
	// Share some area (touching edges do not).
	[[nodiscard]] bool overlaps(const Rect& o) const {
		return x < o.x + o.w && o.x < x + w && y < o.y + o.h && o.y < y + h;
	}
};

// The feedback line under a screen ("Saved to slot 2"): shown for MS, fading out over the last FADE_MS.
struct Toast {
	static constexpr int MS = 2200;
	static constexpr int FADE_MS = 600;
	std::string text;
	int startMs = 0;
	void Show(const std::string& line, int now) {
		text = line;
		startMs = now;
	}
	// 0 when not showing.
	[[nodiscard]] float Alpha(int now) const {
		const int age = now - startMs;
		if (text.empty() || age < 0 || age >= MS)
			return 0.f;
		return age > MS - FADE_MS ? static_cast<float>(MS - age) / static_cast<float>(FADE_MS) : 1.f;
	}
};

// Part of a `canvasW` x `canvasH` layout (y up) seen in a resX x resY window: the canvas keeps its aspect ratio and
// is centred, the margins of a wider or taller window are added around it.
Rect visibleArea(float canvasW, float canvasH, int resX, int resY);
// Window pixel (y down) -> canvas point inside `area`.
void toCanvas(const Rect& area, int resX, int resY, int mouseX, int mouseY, float& x, float& y);
// Canvas point inside `area` -> window pixel (y down), the nearest one: toCanvas the other way.
void toWindow(const Rect& area, int resX, int resY, float x, float y, int& mouseX, int& mouseY);

// ---- screen tabs ----
// The strip at the top right of the 160 x 100 canvas that switches between the in-game screens: one tile per screen
// with its icon and key, the open one lapis. Screens with it keep their title rule short (SCREEN_TABS_TITLE_REACH).
enum class ScreenTab : std::uint8_t { Inventory, Map, Journal };
constexpr int SCREEN_TAB_COUNT = 3;
constexpr float SCREEN_TABS_TITLE_REACH = 44.f;
Rect screenTabRect(int tab);
// -1 off the strip.
int screenTabAt(float x, float y);

} // namespace ui

// The gameplay status message ("Gained 20 XP", "Found: Sword"): a framed panel at the top centre of a square canvas
// 100 high (StatusBox::draw), sized to the text, one line per '\n'. Fades in, then out at the end of its time.
namespace StatusBox {
constexpr float CANVAS_H = 100.f;
// The message's lines, without the empty ones at its end ("Now you are level 2\n").
[[nodiscard]] std::vector<std::string> SplitLines(const std::string& message);
// ageMs since it was shown, out of shownMs: 0 .. 1.
[[nodiscard]] float Alpha(int ageMs, int shownMs);
// The panel for `lines` lines, the widest textW wide, on a canvas canvasW wide.
[[nodiscard]] ui::Rect Box(float canvasW, float textW, int lines);
// Pen y of line i (from 0, the top one).
[[nodiscard]] float LineY(int i);
} // namespace StatusBox

namespace PlayerHud {
// The health bar's lost part: after a hit it holds the old health for TRAIL_HOLD_MS, then drains down.
constexpr int TRAIL_HOLD_MS = 500;			 // the lost part stays this long after the last hit...
constexpr float TRAIL_DRAIN_PER_MS = 0.001f; // ...then drains at this ratio per ms
struct DamageTrail {
	bool started = false;
	float shown = 1.f; // the trail's end
	float last = 1.f;  // health ratio of the last frame
	int hitMs = 0;
	int lastMs = 0;

	// The health ratio now (0..1), at GameClock time now: the trail's end.
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
};
} // namespace PlayerHud

// The HUD's level badge: a gem that gets more precious every LEVELS_PER_GEM levels (level_gem.h).
namespace LevelGem {
constexpr int LEVELS_PER_GEM = 5;
constexpr int GEM_COUNT = 6; // carnelian, turquoise, lapis lazuli, malachite, amethyst, obsidian
// The gem of player level `level` (from 1): 0 carnelian .. GEM_COUNT - 1, the last one from there on.
[[nodiscard]] int GemOf(int level);
} // namespace LevelGem

#endif
