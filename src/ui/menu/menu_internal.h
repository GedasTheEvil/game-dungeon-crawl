#ifndef MENU_INTERNAL_H
#define MENU_INTERNAL_H
#include <array>
#include <cstdint>
#include <string>
#include "../../level/input/bindings.h"
#include "../../level/files/save_slots.h"
#include "../../render/ui_draw.h"

/// @file menu_internal.h
/// What the main menu's files share (menu.cpp, menu_options.cpp, menu_icons.cpp, menu_slots.cpp): the layout, the
/// option rows, the icons and the slot helpers. Only for these files.

// Same 160 x 100 canvas (y up) and look as the inventory: a framed panel on the carved slate, lapis and stone tiles.
struct RenderSettings; // state/game_state.h

namespace menu_internal {
using namespace ui;

constexpr float CENTRE = CANVAS_W / 2;

// ---- layout ----------------------------------------------------------------

constexpr Rect MENU_PANEL = {46, 19, 68, 64};
constexpr Rect TALL_MENU_PANEL = {46, 10, 68, 74}; // seven buttons (plan autosave)
constexpr Rect FULL_MENU_PANEL = {46, 7, 68, 78}; // the main menu with Continue: eight buttons (plan controls-help)
constexpr float FULL_BUTTON_H = 8.f; // eight buttons are a little lower
constexpr float BUTTON_W = 56.f;
constexpr float BUTTON_H = 9.f;
constexpr float BUTTON_STEP = 11.5f;
constexpr float BUTTON_MARGIN = 2.f; // panel edge to the first and last button, when they are close

constexpr Rect SLOT_PANEL = {12, 22, 136, 62};
constexpr float SLOT_W = 62.f;
constexpr float SLOT_H = 16.f;
constexpr float SLOT_GAP_X = 4.f;
constexpr float SLOT_GAP_Y = 3.5f;
constexpr Rect BACK_BUTTON = {63, 9.5f, 34, 8};
// Load Game (plan autosave): the autosave and the quick save (plan quick-save) across the top, the six slots in the
// three rows under it, a little lower.
constexpr Rect LOAD_PANEL = {12, 20, 136, 65};
constexpr float LOAD_SLOT_H = 12.5f;
constexpr float LOAD_GAP_Y = 2.5f;
constexpr int AUTOSAVE_TARGET = SaveSlots::COUNT; // the click target after the slots'
constexpr int QUICKSAVE_TARGET = AUTOSAVE_TARGET + 1; // plan quick-save: right of the autosave

constexpr Rect OPTIONS_PANEL = {6, 20, 148, 64};
constexpr float TAB_W = 26.f; // five tabs across OPTIONS_PANEL
constexpr float TAB_H = 7.f;
constexpr float OPTIONS_LEFT = OPTIONS_PANEL.x + 5; // the tab contents, the rule under the tabs
constexpr float OPTIONS_RIGHT = OPTIONS_PANEL.x + OPTIONS_PANEL.w - 5;

// Controls tab: two columns of rows; a row is the action, its two key cells and its mouse cell.
constexpr float TABLE_TOP = 70.2f; // header baseline
constexpr float TABLE_RULE = TABLE_TOP - 1.1f;
constexpr float ROW_H = 3.5f;
constexpr int ROWS_PER_COLUMN = 12;
constexpr float COLUMN_W = 68.f;
constexpr float COLUMN_GAP = 2.f;
constexpr float CELL_X = 29.f; // from the column's left edge
constexpr float CELL_STEP = 13.f;
constexpr float CELL_W = 12.f;
constexpr float FIXED_KEYS_Y = 23.f; // the line naming the keys that cannot be changed

// Display and Sound tabs: a row is the setting's name and hint, its switch, choice or slider on the right.
constexpr float OPTION_TOP = 71.5f;
constexpr float OPTION_H = 5.04f; // ten rows above the panel's bottom edge
constexpr float SWITCH_W = 22.f;
constexpr float CHOICE_W = 30.f;
constexpr float SLIDER_W = 44.f;
constexpr float SLIDER_VALUE_W = 9.f; // the number right of the slider

// The credits sheet is square; it sits in its own frame, not stretched over the window.
constexpr Rect CREDITS_SHEET = {CENTRE - 30, 22, 60, 60};
constexpr Rect CREDITS_PANEL = {CREDITS_SHEET.x - 3, CREDITS_SHEET.y - 3, CREDITS_SHEET.w + 6, CREDITS_SHEET.h + 6};

// The credits' second page (plan credits-second-section): rows of name, author and licence in a wide panel, scrolled.
constexpr Rect CREDITS_TEXT_PANEL = OPTIONS_PANEL;
constexpr Rect CREDITS_PAGE_BUTTON = {BACK_BUTTON.x + BACK_BUTTON.w + 4, BACK_BUTTON.y, 34, BACK_BUTTON.h};
constexpr float CREDITS_ROW_H = 3.4f;
constexpr int CREDITS_VISIBLE_ROWS = 16;
constexpr float CREDITS_AUTHOR_X = 60.f; // from the panel's inner left edge

// How to Play (plan controls-help): the bound keys in three columns of groups, the hints switch and a button to
// Options > Controls at the foot of the right column.
constexpr Rect HELP_PANEL = OPTIONS_PANEL;
constexpr float HELP_LEFT = HELP_PANEL.x + 5;
constexpr float HELP_COLUMN_W = 44.f;
constexpr float HELP_COLUMN_GAP = 2.f;
constexpr float HELP_TOP = HELP_PANEL.y + HELP_PANEL.h - 5.f; // the first heading's top
constexpr float HELP_ROW_H = 3.4f;
constexpr float HELP_GROUP_GAP = 2.f;
constexpr float HELP_TILE_H = 6.f;

Rect helpHintsSwitch();
Rect helpKeysButton();

Rect slotRect(int slot, bool load);
Rect tabRect(int tab);
Rect visibleArea(const RenderSettings& render);

// ---- options ----------------------------------------------------------------

// Display holds [display] of settings.ini, Graphics [graphics] (plan ui-font-setting split them: the Text font row
// did not fit an eleventh row).
constexpr std::array<const char*, 5> TABS = {"Controls", "Mouse", "Display", "Graphics", "Sound"};
constexpr int CONTROLS_TAB = 0;
constexpr int MOUSE_TAB = 1;
constexpr int DISPLAY_TAB = 2;
constexpr int GRAPHICS_TAB = 3;
constexpr int SOUND_TAB = 4;

enum class MouseInput : std::uint8_t { None, Left, Middle, Right };

MouseInput mouseInput(const InputKey& key);

enum class OptionKind : std::uint8_t { Switch, Choice, Slider };
enum class OptionId : std::uint8_t {
	WindowSize,
	Fullscreen,
	AnimationRate,
	FrameRate,
	Brightness,
	UiFont,
	MotionEffects,
	Toon,
	Blood,
	LightFlicker,
	CutScenes,
	CameraRecenter,
	MouseSensitivity,
	InvertY,
	Master,
	Music,
	Effects,
	Mute
};

struct OptionRow {
	OptionId id;
	OptionKind kind;
	const char* name;
	const char* hint;
};

struct OptionRows {
	const OptionRow* rows;
	int count;
	[[nodiscard]] const OptionRow& operator[](int i) const { return rows[i]; }
};

OptionRows optionRows(int tab);
Rect optionControl(int row, OptionKind kind);
Rect controlCell(int action, int slot);
Rect resetButton();

// ---- icons -----------------------------------------------------------------

enum class Icon : std::uint8_t { Play, Save, Load, Gear, Ankh, Exit, Pyramid, Back, Book, Podium, KeyCap };
void drawIcon(Icon icon, float cx, float cy, float s, Color c);

constexpr float MOUSE_W = 2.6f;
constexpr float MOUSE_H = 3.6f;

void drawMouse(float x, float y, MouseInput m, float scale);

// ---- save slots ------------------------------------------------------------

Rect deleteRect(int slot, bool load);
std::string saveName(int target);
} // namespace menu_internal

#endif
