#include "menu.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>
#include "../graphics/gl_includes.h"
#include "../state/game_state.h"
#include "../state/settings.h"
#include "../level/input.h"
#include "menu_internal.h"

namespace menu_internal {
// ---- options ----------------------------------------------------------------

MouseInput mouseInput(const InputKey& key) {
	if (key.kind != InputKey::Kind::Mouse)
		return MouseInput::None;
	switch (key.code) {
	case MOUSE_LEFT_BUTTON:
		return MouseInput::Left;
	case MOUSE_MIDDLE_BUTTON:
		return MouseInput::Middle;
	case MOUSE_RIGHT_BUTTON:
		return MouseInput::Right;
	default:
		return MouseInput::None;
	}
}

constexpr std::array<OptionRow, 10> DISPLAY_ROWS = {{
	{OptionId::WindowSize, OptionKind::Choice, "Window size", "Click for the next size, or drag the window"},
	{OptionId::Fullscreen, OptionKind::Switch, "Fullscreen", "The whole screen, no window frame"},
	{OptionId::Brightness, OptionKind::Slider, "Brightness", "Set the ankh in the black box barely visible"},
	{OptionId::AnimationRate,
	 OptionKind::Choice,
	 "Animation rate",
	 "Poses a second of the figures, or key frames only"},
	{OptionId::FrameRate, OptionKind::Choice, "Frame rate", "Frames a second; lower saves power and heat"},
	{OptionId::MotionEffects, OptionKind::Switch, "Motion effects", "Sprint blur, wider view and darker edges"},
	{OptionId::Toon, OptionKind::Switch, "Toon shading", "Cel bands and ink outlines (F1)"},
	{OptionId::Blood, OptionKind::Switch, "Blood", "Blood splashes of hits and deaths"},
	{OptionId::LightFlicker, OptionKind::Switch, "Light flicker", "Torches, braziers and lamps flicker"},
	{OptionId::CutScenes, OptionKind::Switch, "Cut-scenes", "The view goes to a far gate as it opens"},
}};
// Plan settings-extras: the mouse rows, and the camera's recentre beside them.
constexpr std::array<OptionRow, 3> MOUSE_ROWS = {{
	{OptionId::MouseSensitivity, OptionKind::Slider, "Mouse sensitivity", "How far the view turns as the mouse moves"},
	{OptionId::InvertY, OptionKind::Switch, "Invert mouse Y", "Moving the mouse up looks down"},
	{OptionId::CameraRecenter, OptionKind::Switch, "Camera recentre", "A look-key peek eases back when left alone"},
}};
constexpr std::array<OptionRow, 4> SOUND_ROWS = {{
	{OptionId::Master, OptionKind::Slider, "Master volume", "Scales the music and the effects"},
	{OptionId::Music, OptionKind::Slider, "Music volume", "The soundtrack"},
	{OptionId::Effects, OptionKind::Slider, "Effects volume", "Hits, jumps, levers and gates"},
	{OptionId::Mute, OptionKind::Switch, "Mute", "Silence, the volumes kept"},
}};

OptionRows optionRows(int tab) {
	if (tab == SOUND_TAB)
		return {SOUND_ROWS.data(), static_cast<int>(SOUND_ROWS.size())};
	if (tab == DISPLAY_TAB)
		return {DISPLAY_ROWS.data(), static_cast<int>(DISPLAY_ROWS.size())};
	if (tab == MOUSE_TAB)
		return {MOUSE_ROWS.data(), static_cast<int>(MOUSE_ROWS.size())};
	return {DISPLAY_ROWS.data(), 0}; // the Controls tab has its own table
}

// The poses a second the Animation rate row steps through; 0: the key frames only (plan model-frame-interpolation).
constexpr std::array<int, 4> ANIMATION_RATES = {0, 24, 30, 60};
// The choices the Frame rate row steps through (plan frame-rate-cap).
constexpr std::array<int, 4> FRAME_RATES = {FRAME_RATE_MONITOR, 30, 60, FRAME_RATE_UNLIMITED};

bool& switchOf(OptionId id) {
	Settings& s = Game().settings;
	switch (id) {
	case OptionId::Fullscreen:
		return s.display.fullscreen;
	case OptionId::Toon:
		return s.graphics.toon;
	case OptionId::Blood:
		return s.graphics.blood;
	case OptionId::LightFlicker:
		return s.graphics.lightFlicker;
	case OptionId::CutScenes:
		return s.graphics.cutScenes;
	case OptionId::CameraRecenter:
		return s.graphics.cameraRecenter;
	case OptionId::InvertY:
		return s.mouse.invertY;
	case OptionId::Mute:
		return s.sound.mute;
	default:
		return s.graphics.motionEffects;
	}
}

int& sliderOf(OptionId id) {
	Settings& s = Game().settings;
	switch (id) {
	case OptionId::Brightness:
		return s.display.brightness;
	case OptionId::MouseSensitivity:
		return s.mouse.sensitivity;
	case OptionId::Master:
		return s.sound.master;
	case OptionId::Music:
		return s.sound.music;
	default:
		return s.sound.effects;
	}
}

// A slider's values: lowest, highest, and the step of a key press or a drag.
struct SliderRange {
	int lo, hi, step;
};

SliderRange sliderRange(OptionId id) {
	if (id == OptionId::Brightness)
		return {MIN_BRIGHTNESS, MAX_BRIGHTNESS, 5};
	if (id == OptionId::MouseSensitivity)
		return {MIN_MOUSE_SENSITIVITY, MAX_MOUSE_SENSITIVITY, 10};
	return {0, 100, 5};
}

// A sound slider plays a sample at its new volume.
bool samplesEffects(OptionId id) { return id == OptionId::Effects || id == OptionId::Master; }

// The brightness row's calibration (plan settings-extras): a black box with an ankh a shade above black. The
// brightness pass lightens or darkens it with the rest of the picture.
void drawCalibration(const Rect& control) {
	constexpr float NEAR_BLACK = 0.06f;
	const Rect box = {control.x - 11.f, control.y - 0.2f, 8.f, control.h + 0.4f};
	fillRect(box, BLACK, BLACK, 1.f);
	strokeRect(box, GOLD_DIM, 1.f, 1.f);
	drawIcon(Icon::Ankh, box.cx(), box.cy(), box.h * 0.45f, {NEAR_BLACK, NEAR_BLACK, NEAR_BLACK});
}

Rect optionBand(int row) {
	float y = OPTION_TOP - static_cast<float>(row + 1) * OPTION_H;
	return {OPTIONS_LEFT, y + 0.5f, OPTIONS_RIGHT - OPTIONS_LEFT, OPTION_H - 1.f};
}

// The switch, the choice or the slider's track (the mouse target) of a row.
Rect optionControl(int row, OptionKind kind) {
	Rect band = optionBand(row);
	float w = kind == OptionKind::Switch ? SWITCH_W : kind == OptionKind::Choice ? CHOICE_W : SLIDER_W;
	float right = band.x + band.w - 3.f - (kind == OptionKind::Slider ? SLIDER_VALUE_W : 0.f);
	return {right - w, band.y + 0.5f, w, band.h - 1.f};
}

// Controls tab: row `index` (an action, or the reset button after the last) in its column.
Rect controlRow(int index) {
	const int column = index / ROWS_PER_COLUMN;
	float x = OPTIONS_LEFT + static_cast<float>(column) * (COLUMN_W + COLUMN_GAP);
	float y = TABLE_RULE - static_cast<float>(index % ROWS_PER_COLUMN + 1) * ROW_H;
	return {x, y, COLUMN_W, ROW_H};
}

Rect controlCell(int action, int slot) {
	Rect row = controlRow(action);
	return {row.x + CELL_X + static_cast<float>(slot) * CELL_STEP, row.y + 0.45f, CELL_W, ROW_H - 0.9f};
}

Rect resetButton() {
	Rect row = controlRow(BIND_ACTION_COUNT);
	return {row.x + CELL_X, row.y + 0.2f, 2 * CELL_STEP + CELL_W, ROW_H - 0.4f};
}
} // namespace menu_internal
using namespace menu_internal;

void MainMenu::DrawOptions() {
	// Tab strip along the top of the panel; the active tab is lapis and sits on the rule.
	for (int tab = 0; tab < static_cast<int>(TABS.size()); tab++) {
		bool active = tab == optionsTab;
		bool isHovered = pointer.hovered == TAB_BASE + tab;
		Rect r = tile(tabRect(tab), active ? TileStyle::Lapis : TileStyle::Stone, isHovered && !active, false);
		beginText();
		textCenteredIn(heading, r, TABS[static_cast<size_t>(tab)], active ? GOLD : LABEL);
		beginShapes();
	}
	float ruleY = tabRect(0).y;
	line(OPTIONS_LEFT, ruleY, OPTIONS_RIGHT, ruleY, GOLD_DIM, 1.f, 1.5f);

	if (optionsTab == CONTROLS_TAB)
		DrawControls(OPTIONS_LEFT, OPTIONS_RIGHT);
	else
		DrawRows();
}

// One row a setting: its name and what it does, its switch, choice or slider on the right.
void MainMenu::DrawRows() {
	OptionRows rows = optionRows(optionsTab);
	const Settings& settings = Game().settings;
	for (int i = 0; i < rows.count; i++) {
		const OptionRow& row = rows[i];
		const int target = OPTION_BASE + i;
		const bool isHovered = pointer.hovered == target || dragging == target;
		Rect band = optionBand(i);
		Rect control = optionControl(i, row.kind);
		if (i % 2 == 0)
			fillRect(band, {1, 1, 1}, {1, 1, 1}, 0.035f);

		char value[24] = "";
		bool on = false;
		bool enabled = true;
		if (row.kind == OptionKind::Switch) {
			on = switchOf(row.id);
			Rect r = tile(control, on ? TileStyle::Lapis : TileStyle::Stone, isHovered, pointer.Held(target));
			std::snprintf(value, sizeof(value), "%s", on ? "On" : "Off");
			control = r;
		} else if (row.kind == OptionKind::Choice) {
			const bool windowSize = row.id == OptionId::WindowSize;
			enabled = !windowSize || !settings.display.fullscreen; // the window size only matters in a window
			Rect r = tile(
				control,
				enabled ? TileStyle::Stone : TileStyle::Disabled,
				enabled && isHovered,
				enabled && pointer.Held(target)
			);
			if (windowSize)
				std::snprintf(value, sizeof(value), "%d x %d", settings.display.width, settings.display.height);
			else if (row.id == OptionId::FrameRate) {
				const int rate = settings.display.frameRate;
				if (rate == FRAME_RATE_MONITOR)
					std::snprintf(value, sizeof(value), "Monitor");
				else if (rate == FRAME_RATE_UNLIMITED)
					std::snprintf(value, sizeof(value), "Unlimited");
				else
					std::snprintf(value, sizeof(value), "%d", rate);
			} else if (settings.display.animationRate <= 0)
				std::snprintf(value, sizeof(value), "Key frames");
			else
				std::snprintf(value, sizeof(value), "%d", settings.display.animationRate);
			control = r;
		} else {
			// Track, the filled part, the knob; the number to the right.
			const int level = sliderOf(row.id);
			const SliderRange range = sliderRange(row.id);
			const float t = static_cast<float>(level - range.lo) / static_cast<float>(range.hi - range.lo);
			if (row.id == OptionId::Brightness)
				drawCalibration(control);
			Rect track = {control.x, control.cy() - 0.6f, control.w, 1.2f};
			fillRect(track, WELL, WELL, 1.f);
			fillRect({track.x, track.y, track.w * t, track.h}, LAPIS_HOVER_TOP, LAPIS, 1.f);
			strokeRect(track, GOLD_DIM, 1.f, 1.f);
			Rect knob = {track.x + track.w * t - 1.f, control.cy() - 1.8f, 2.f, 3.6f};
			fillRect(knob, isHovered ? GOLD_BRIGHT : GOLD, GOLD_DIM, 1.f);
			strokeRect(knob, BRONZE, 1.f, 1.f);
			std::snprintf(value, sizeof(value), "%d", level);
		}

		beginText();
		text(body, band.x + 3.f, band.y + 2.4f, row.name, LABEL);
		text(small, band.x + 3.f, band.y + 0.3f, row.hint, LABEL_DIM);
		if (row.kind == OptionKind::Slider)
			textMiddle(heading, control.x + control.w + 2.5f, control.cy(), value, isHovered ? GOLD_BRIGHT : GOLD);
		else
			textCenteredIn(
				heading,
				control,
				value,
				!enabled ? LABEL_DIM : (on || row.kind == OptionKind::Choice ? GOLD : LABEL)
			);
		beginShapes();
	}
}

// A key cell: the key cap, or the mouse with its button lit; a dash when unbound; lapis while it waits for input.
void MainMenu::DrawControlCell(int target, const Rect& r) {
	const int action = (target - BIND_BASE) / BINDING_SLOTS;
	const int slot = (target - BIND_BASE) % BINDING_SLOTS;
	const InputKey& key = Game().settings.controls.Of(static_cast<BindAction>(action)).Slot(slot);
	const bool waiting = capturing == target;
	const bool isHovered = pointer.hovered == target;

	fillRect({r.x, r.y - 0.25f, r.w, r.h}, BLACK, BLACK, 0.5f);
	if (waiting) {
		float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GameClock::now()) * 0.008f);
		fillRect(r, LAPIS_HOVER_TOP, LAPIS_DARK, 0.6f + 0.4f * pulse);
	} else {
		fillRect(r, {0.19f, 0.15f, 0.10f}, {0.10f, 0.08f, 0.055f}, 1.f);
	}
	strokeRect(r, waiting || isHovered ? GOLD : GOLD_DIM, 1.f, waiting || isHovered ? 1.5f : 1.f);

	if (waiting) {
		beginText();
		textCenteredIn(small, r, slot == MOUSE_SLOT ? "Click" : "Press", GOLD_BRIGHT);
		beginShapes();
	} else if (slot == MOUSE_SLOT && key.Bound()) {
		constexpr float SCALE = 0.75f;
		drawMouse(r.cx() - MOUSE_W * SCALE / 2, r.cy() - MOUSE_H * SCALE / 2, mouseInput(key), SCALE);
	} else {
		beginText();
		textCenteredIn(
			small,
			r,
			key.Bound() ? keyCap(key).c_str() : "-",
			key.Bound() ? (isHovered ? GOLD_BRIGHT : GOLD) : LABEL_DIM
		);
		beginShapes();
	}
}

void MainMenu::DrawControls(float left, float right) {
	// Two columns: header, then one striped row per action; the reset button ends the right column.
	for (int column = 0; column < 2; column++) {
		Rect first = controlRow(column * ROWS_PER_COLUMN);
		line(first.x, TABLE_RULE, first.x + first.w, TABLE_RULE, BRONZE, 1.f, 1.f);
		beginText();
		text(small, first.x + 1.5f, TABLE_TOP, "Action", LABEL);
		text(small, first.x + CELL_X + 0.5f, TABLE_TOP, "Keys", LABEL);
		text(small, first.x + CELL_X + 2 * CELL_STEP + 0.5f, TABLE_TOP, "Mouse", LABEL);
		beginShapes();
	}
	for (int i = 0; i < BIND_ACTION_COUNT; i++) {
		Rect row = controlRow(i);
		if (i % 2 == 0)
			fillRect(row, {1, 1, 1}, {1, 1, 1}, 0.035f);
		beginText();
		text(body, row.x + 1.5f, row.y + 0.5f, bindActionLabel(static_cast<BindAction>(i)), LABEL);
		beginShapes();
		for (int slot = 0; slot < BINDING_SLOTS; slot++)
			DrawControlCell(BIND_BASE + i * BINDING_SLOTS + slot, controlCell(i, slot));
	}

	bool isHovered = pointer.hovered == RESET_CONTROLS;
	Rect reset = tile(resetButton(), TileStyle::Stone, isHovered, pointer.Held(RESET_CONTROLS));
	beginText();
	textCenteredIn(small, reset, "Reset to defaults", isHovered ? GOLD_BRIGHT : LABEL);
	textCentered(
		small,
		(left + right) / 2,
		FIXED_KEYS_Y,
		"Fixed:  Esc menu / back    F1 toon shading    F5 quick save    F9 quick load    mouse movement "
		"looks around",
		LABEL_DIM
	);
	beginShapes();
}

void MainMenu::DrawFooterHint() {
	if (capturing == NONE) {
		DrawFooter(
			optionsTab == CONTROLS_TAB ? "Click or Enter: change a key    Esc: back"
									   : "Arrows: move    Left / Right: change    Esc: back"
		);
		return;
	}
	auto action = static_cast<BindAction>((capturing - BIND_BASE) / BINDING_SLOTS);
	bool mouse = (capturing - BIND_BASE) % BINDING_SLOTS == MOUSE_SLOT;
	std::string hint = std::string(mouse ? "Click a mouse button for " : "Press a key for ") + bindActionLabel(action) +
					   "    Delete: clear    Esc: cancel";
	DrawFooter(hint.c_str());
}

void MainMenu::ActivateOption(int row, int step) {
	const OptionRow option = optionRows(optionsTab)[row];
	Settings::Display& display = Game().settings.display;
	if (option.id == OptionId::WindowSize) {
		if (display.fullscreen) {
			ShowToast("Leave fullscreen to pick a window size");
			return;
		}
		// The next larger size, after the largest the smallest; stepping back the other way round. The sizes go up
		// to the desktop's (plan settings-extras).
		const std::vector<WindowSize> sizes = windowSizes(glutGet(GLUT_SCREEN_WIDTH), glutGet(GLUT_SCREEN_HEIGHT));
		auto larger = [&](const WindowSize& size) {
			return size.w > display.width || (size.w == display.width && size.h > display.height);
		};
		auto smaller = [&](const WindowSize& size) {
			return size.w < display.width || (size.w == display.width && size.h < display.height);
		};
		WindowSize next = step > 0 ? sizes.front() : sizes.back();
		if (step > 0) {
			if (const auto it = std::ranges::find_if(sizes, larger); it != sizes.end())
				next = *it;
		} else if (const auto it = std::ranges::find_if(sizes.rbegin(), sizes.rend(), smaller); it != sizes.rend())
			next = *it;
		display.width = next.w;
		display.height = next.h;
		glutReshapeWindow(next.w, next.h);
	} else if (option.id == OptionId::AnimationRate) {
		// The next higher rate, after the highest the key frames; stepping back the other way round.
		if (step > 0) {
			const auto higher = std::ranges::find_if(ANIMATION_RATES, [&](int r) { return r > display.animationRate; });
			display.animationRate = higher == ANIMATION_RATES.end() ? ANIMATION_RATES.front() : *higher;
		} else {
			const auto lower = std::ranges::find_if(ANIMATION_RATES.rbegin(), ANIMATION_RATES.rend(), [&](int r) {
				return r < display.animationRate;
			});
			display.animationRate = lower == ANIMATION_RATES.rend() ? ANIMATION_RATES.back() : *lower;
		}
	} else if (option.id == OptionId::FrameRate) {
		// The next choice, after Unlimited the monitor's; a hand-edited rate goes to the first.
		const auto at = std::ranges::find(FRAME_RATES, display.frameRate);
		if (step > 0)
			display.frameRate = at == FRAME_RATES.end() || at + 1 == FRAME_RATES.end() ? FRAME_RATES[0] : *(at + 1);
		else
			display.frameRate = at == FRAME_RATES.end() || at == FRAME_RATES.begin() ? FRAME_RATES.back() : *(at - 1);
	} else if (option.kind == OptionKind::Switch) {
		bool& on = switchOf(option.id);
		on = !on;
		if (option.id == OptionId::Fullscreen) {
			if (on)
				glutFullScreen();
			else
				glutLeaveFullScreen();
		}
	}
	Game().ApplySettings(true);
}

void MainMenu::StepOption(int row, int step) {
	const OptionRow option = optionRows(optionsTab)[row];
	if (option.kind != OptionKind::Slider) {
		ActivateOption(row, step);
		return;
	}
	int& level = sliderOf(option.id);
	const SliderRange range = sliderRange(option.id);
	level = std::clamp(level + range.step * step, range.lo, range.hi);
	Game().ApplySettings(true);
	if (samplesEffects(option.id))
		Game().assets.sounds.lever.Play(); // a sample at the new volume
}

void MainMenu::SetSlider(int row, int x) {
	const OptionRow option = optionRows(optionsTab)[row];
	float cx = 0.f;
	float cy = 0.f;
	ui::toCanvas(visibleArea(), Game().render.resX, Game().render.resY, x, 0, cx, cy);
	Rect track = optionControl(row, option.kind);
	const float t = std::clamp((cx - track.x) / track.w, 0.f, 1.f);
	const SliderRange range = sliderRange(option.id);
	const float steps = static_cast<float>(range.hi - range.lo) / static_cast<float>(range.step);
	sliderOf(option.id) = range.lo + static_cast<int>(std::lround(t * steps)) * range.step;
	Game().ApplySettings(false);
}

void MainMenu::StartCapture(int target) { capturing = target; }

void MainMenu::CaptureKey(const InputKey& key) {
	const int slot = (capturing - BIND_BASE) % BINDING_SLOTS;
	if (key == InputKey::Char(KEY_ESCAPE)) {
		capturing = NONE;
		return;
	}
	if (key == InputKey::Char(KEY_DELETE) || key == InputKey::Special(SPECIAL_KEYPAD_DELETE)) {
		Game().settings.controls.Clear(static_cast<BindAction>((capturing - BIND_BASE) / BINDING_SLOTS), slot);
		Game().ApplySettings(true);
		capturing = NONE;
		return;
	}
	if (isReservedKey(key)) {
		ShowToast(keyCap(key) + " is fixed, pick another key");
		return;
	}
	if (slot == MOUSE_SLOT) {
		ShowToast("Click a mouse button, or Esc to cancel");
		return;
	}
	Bind(key);
}

void MainMenu::Bind(const InputKey& key) {
	auto action = static_cast<BindAction>((capturing - BIND_BASE) / BINDING_SLOTS);
	const int slot = (capturing - BIND_BASE) % BINDING_SLOTS;
	capturing = NONE;
	if (std::optional<BindAction> from = Game().settings.controls.Bind(action, slot, key))
		ShowToast(keyCap(key) + " moved here from " + bindActionLabel(*from));
	Game().ApplySettings(true);
}
