#include "menu.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <GL/gl.h>
#include "../graphics/gl_includes.h"
#include "../state/game_state.h"
#include "../state/settings.h"
#include "ui_draw.h"
#include "../input/input.h"
#include <optional>
#include <string>

// Same 160 x 100 canvas (y up) and look as the inventory: a framed panel on the carved slate, lapis and stone tiles.
namespace {
using namespace ui;

constexpr float CENTRE = CANVAS_W / 2;

constexpr Color LABEL = {0.86f, 0.72f, 0.47f};
constexpr Color LABEL_DIM = {0.55f, 0.45f, 0.30f};
constexpr Color WELL = {0.07f, 0.055f, 0.04f};

// ---- layout ----------------------------------------------------------------

constexpr Rect MENU_PANEL = {46, 19, 68, 64};
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

constexpr Rect OPTIONS_PANEL = {6, 20, 148, 64};
constexpr float TAB_W = 30.f;
constexpr float TAB_H = 7.f;
constexpr float OPTIONS_LEFT = OPTIONS_PANEL.x + 5; // the tab contents, the rule under the tabs
constexpr float OPTIONS_RIGHT = OPTIONS_PANEL.x + OPTIONS_PANEL.w - 5;

// Controls tab: two columns of rows; a row is the action, its two key cells and its mouse cell.
constexpr float TABLE_TOP = 70.2f; // header baseline
constexpr float TABLE_RULE = TABLE_TOP - 1.1f;
constexpr float ROW_H = 3.8f;
constexpr int ROWS_PER_COLUMN = 11;
constexpr float COLUMN_W = 68.f;
constexpr float COLUMN_GAP = 2.f;
constexpr float CELL_X = 29.f; // from the column's left edge
constexpr float CELL_STEP = 13.f;
constexpr float CELL_W = 12.f;
constexpr float FIXED_KEYS_Y = 23.f; // the line naming the keys that cannot be changed

// Display and Sound tabs: a row is the setting's name and hint, its switch, choice or slider on the right.
constexpr float OPTION_TOP = 71.5f;
constexpr float OPTION_H = 8.f;
constexpr float SWITCH_W = 22.f;
constexpr float CHOICE_W = 30.f;
constexpr float SLIDER_W = 44.f;
constexpr float SLIDER_VALUE_W = 9.f; // the number right of the slider

// The credits sheet is square; it sits in its own frame, not stretched over the window.
constexpr Rect CREDITS_SHEET = {CENTRE - 30, 22, 60, 60};
constexpr Rect CREDITS_PANEL = {CREDITS_SHEET.x - 3, CREDITS_SHEET.y - 3, CREDITS_SHEET.w + 6, CREDITS_SHEET.h + 6};

// The buttons sit in the middle of the panel; more of them are closer together.
Rect buttonRect(int index, int count) {
	float step = std::min(BUTTON_STEP, (MENU_PANEL.h - 2 * BUTTON_MARGIN - BUTTON_H) / static_cast<float>(count - 1));
	float first = MENU_PANEL.cy() + step * static_cast<float>(count - 1) / 2 - BUTTON_H / 2;
	return {CENTRE - BUTTON_W / 2, first - static_cast<float>(index) * step, BUTTON_W, BUTTON_H};
}

// Slots 1-3 in the left column, 4-6 in the right one, top to bottom.
Rect slotRect(int slot) {
	float x0 = SLOT_PANEL.cx() - SLOT_W - SLOT_GAP_X / 2;
	float top = SLOT_PANEL.y + SLOT_PANEL.h - 4.f;
	int column = slot / 3;
	int row = slot % 3;
	return {x0 + static_cast<float>(column) * (SLOT_W + SLOT_GAP_X),
			top - SLOT_H - static_cast<float>(row) * (SLOT_H + SLOT_GAP_Y), SLOT_W, SLOT_H};
}

Rect tabRect(int tab) {
	float y = OPTIONS_PANEL.y + OPTIONS_PANEL.h - 4.f - TAB_H;
	return {OPTIONS_PANEL.x + 5 + static_cast<float>(tab) * (TAB_W + 2), y, TAB_W, TAB_H};
}

Rect visibleArea() { return ui::visibleArea(CANVAS_W, CANVAS_H, Game().render.resX, Game().render.resY); }

// ---- icons -----------------------------------------------------------------

enum class Icon : std::uint8_t { Play, Save, Load, Gear, Ankh, Exit, Pyramid, Back, Book };
enum class MenuAction : std::uint8_t { NewGame, Resume, Journal, Save, Load, Options, Credits, MainMenu, Quit };

struct MenuButton {
	const char* label;
	Icon icon;
	MenuAction action;
	bool primary;
};

constexpr std::array<MenuButton, 5> MAIN_BUTTONS = {{
	{"New Game", Icon::Play, MenuAction::NewGame, true},
	{"Load Game", Icon::Load, MenuAction::Load, false},
	{"Options", Icon::Gear, MenuAction::Options, false},
	{"Credits", Icon::Ankh, MenuAction::Credits, false},
	{"Exit", Icon::Exit, MenuAction::Quit, false},
}};
constexpr std::array<MenuButton, 6> IN_GAME_BUTTONS = {{
	{"Return to Game", Icon::Play, MenuAction::Resume, true},
	{"Journal", Icon::Book, MenuAction::Journal, false},
	{"Save Game", Icon::Save, MenuAction::Save, false},
	{"Load Game", Icon::Load, MenuAction::Load, false},
	{"Options", Icon::Gear, MenuAction::Options, false},
	{"Main Menu", Icon::Pyramid, MenuAction::MainMenu, false},
}};

// ---- options ----------------------------------------------------------------

constexpr std::array<const char*, 3> TABS = {"Controls", "Display", "Sound"};
constexpr int CONTROLS_TAB = 0;
constexpr int DISPLAY_TAB = 1;
constexpr int SOUND_TAB = 2;

enum class MouseInput : std::uint8_t { None, Left, Middle, Right };

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

enum class OptionKind : std::uint8_t { Switch, Choice, Slider };
enum class OptionId : std::uint8_t { WindowSize, Fullscreen, MotionEffects, Toon, Blood, LightFlicker, Music, Effects };

struct OptionRow {
	OptionId id;
	OptionKind kind;
	const char* name;
	const char* hint;
};

constexpr std::array<OptionRow, 6> DISPLAY_ROWS = {{
	{OptionId::WindowSize, OptionKind::Choice, "Window size", "Click for the next size, or drag the window"},
	{OptionId::Fullscreen, OptionKind::Switch, "Fullscreen", "The whole screen, no window frame"},
	{OptionId::MotionEffects, OptionKind::Switch, "Motion effects", "Sprint blur, wider view and darker edges"},
	{OptionId::Toon, OptionKind::Switch, "Toon shading", "Cel bands and ink outlines (F1)"},
	{OptionId::Blood, OptionKind::Switch, "Blood", "Blood splashes of hits and deaths"},
	{OptionId::LightFlicker, OptionKind::Switch, "Light flicker", "Torches, braziers and lamps flicker"},
}};
constexpr std::array<OptionRow, 2> SOUND_ROWS = {{
	{OptionId::Music, OptionKind::Slider, "Music volume", "The soundtrack"},
	{OptionId::Effects, OptionKind::Slider, "Effects volume", "Hits, jumps, levers and gates"},
}};

struct OptionRows {
	const OptionRow* rows;
	int count;
	[[nodiscard]] const OptionRow& operator[](int i) const { return rows[i]; }
};

OptionRows optionRows(int tab) {
	if (tab == SOUND_TAB)
		return {SOUND_ROWS.data(), static_cast<int>(SOUND_ROWS.size())};
	if (tab == DISPLAY_TAB)
		return {DISPLAY_ROWS.data(), static_cast<int>(DISPLAY_ROWS.size())};
	return {DISPLAY_ROWS.data(), 0}; // the Controls tab has its own table
}

// The window sizes the Window size row steps through.
struct WindowSize {
	int w, h;
};
constexpr std::array<WindowSize, 5> WINDOW_SIZES = {{{800, 500}, {1024, 640}, {1280, 720}, {1600, 900}, {1920, 1080}}};

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
	default:
		return s.graphics.motionEffects;
	}
}

int& sliderOf(OptionId id) {
	return id == OptionId::Music ? Game().settings.sound.music : Game().settings.sound.effects;
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

struct ButtonList {
	const MenuButton* buttons;
	int count;
	[[nodiscard]] const MenuButton& operator[](int i) const { return buttons[i]; }
};

ButtonList menuButtons(bool inGame) {
	if (inGame)
		return {IN_GAME_BUTTONS.data(), static_cast<int>(IN_GAME_BUTTONS.size())};
	return {MAIN_BUTTONS.data(), static_cast<int>(MAIN_BUTTONS.size())};
}

void bar(float x0, float y0, float x1, float y1, Color c) { fillRect({x0, y0, x1 - x0, y1 - y0}, c, c, 1.f); }

// Open tray under the arrows of the save / load icons.
void tray(float cx, float cy, float s, Color c) {
	float t = 0.2f * s;
	bar(cx - s, cy - s, cx + s, cy - s + t, c);
	bar(cx - s, cy - s, cx - s + t, cy - 0.3f * s, c);
	bar(cx + s - t, cy - s, cx + s, cy - 0.3f * s, c);
}

// Flat glyph in a 2s x 2s box around (cx, cy).
void drawIcon(Icon icon, float cx, float cy, float s, Color c) {
	float t = 0.2f * s; // stroke
	switch (icon) {
	case Icon::Play:
		triangle(cx - 0.55f * s, cy - 0.8f * s, cx + 0.8f * s, cy, cx - 0.55f * s, cy + 0.8f * s, c, 1.f);
		break;
	case Icon::Save: // arrow down into the tray
		tray(cx, cy, s, c);
		bar(cx - t, cy - 0.05f * s, cx + t, cy + s, c);
		triangle(cx - 0.55f * s, cy, cx + 0.55f * s, cy, cx, cy - 0.6f * s, c, 1.f);
		break;
	case Icon::Load: // arrow up out of the tray
		tray(cx, cy, s, c);
		bar(cx - t, cy - 0.6f * s, cx + t, cy + 0.35f * s, c);
		triangle(cx - 0.55f * s, cy + 0.3f * s, cx + 0.55f * s, cy + 0.3f * s, cx, cy + s, c, 1.f);
		break;
	case Icon::Gear: { // eight teeth around a ring
		constexpr int TEETH = 8;
		constexpr int SEGMENTS = 24;
		float outer = 0.62f * s;
		float inner = 0.28f * s;
		glColor4f(c.r, c.g, c.b, 1.f);
		glBegin(GL_QUAD_STRIP);
		for (int i = 0; i <= SEGMENTS; i++) {
			float a = 2.f * static_cast<float>(M_PI) * static_cast<float>(i) / SEGMENTS;
			glVertex2f(cx + outer * std::cos(a), cy + outer * std::sin(a));
			glVertex2f(cx + inner * std::cos(a), cy + inner * std::sin(a));
		}
		glEnd();
		glBegin(GL_QUADS);
		for (int i = 0; i < TEETH; i++) {
			float a = 2.f * static_cast<float>(M_PI) * static_cast<float>(i) / TEETH;
			float dx = std::cos(a);
			float dy = std::sin(a);
			float w = 0.17f * s;
			float r0 = 0.5f * s;
			float r1 = 0.95f * s;
			glVertex2f(cx + dx * r0 - dy * w, cy + dy * r0 + dx * w);
			glVertex2f(cx + dx * r1 - dy * w, cy + dy * r1 + dx * w);
			glVertex2f(cx + dx * r1 + dy * w, cy + dy * r1 - dx * w);
			glVertex2f(cx + dx * r0 + dy * w, cy + dy * r0 - dx * w);
		}
		glEnd();
		break;
	}
	case Icon::Ankh: {
		constexpr int SEGMENTS = 24;
		float lx = cx;
		float ly = cy + 0.5f * s;
		glColor4f(c.r, c.g, c.b, 1.f);
		glBegin(GL_QUAD_STRIP);
		for (int i = 0; i <= SEGMENTS; i++) {
			float a = 2.f * static_cast<float>(M_PI) * static_cast<float>(i) / SEGMENTS;
			glVertex2f(lx + 0.38f * s * std::sin(a), ly + 0.48f * s * std::cos(a));
			glVertex2f(lx + (0.38f * s - t) * std::sin(a), ly + (0.48f * s - t) * std::cos(a));
		}
		glEnd();
		bar(cx - t, cy - s, cx + t, cy + 0.05f * s, c);
		bar(cx - 0.75f * s, cy - 0.12f * s, cx + 0.75f * s, cy - 0.12f * s + 1.3f * t, c);
		break;
	}
	case Icon::Exit: // door frame with an arrow walking out of it
		bar(cx - 0.9f * s, cy - s, cx - 0.9f * s + t, cy + s, c);
		bar(cx - 0.9f * s, cy + s - t, cx + 0.2f * s, cy + s, c);
		bar(cx - 0.9f * s, cy - s, cx + 0.2f * s, cy - s + t, c);
		bar(cx + 0.2f * s - t, cy + 0.4f * s, cx + 0.2f * s, cy + s, c);
		bar(cx + 0.2f * s - t, cy - s, cx + 0.2f * s, cy - 0.4f * s, c);
		bar(cx - 0.45f * s, cy - 0.8f * t, cx + 0.45f * s, cy + 0.8f * t, c);
		triangle(cx + 0.4f * s, cy - 0.5f * s, cx + s, cy, cx + 0.4f * s, cy + 0.5f * s, c, 1.f);
		break;
	case Icon::Pyramid: // lit face and shaded face
		triangle(cx - s, cy - 0.8f * s, cx + 0.15f * s, cy - 0.8f * s, cx, cy + 0.85f * s, c, 1.f);
		triangle(cx + 0.15f * s, cy - 0.8f * s, cx + s, cy - 0.8f * s, cx, cy + 0.85f * s,
				 {c.r * 0.6f, c.g * 0.6f, c.b * 0.6f}, 1.f);
		break;
	case Icon::Back:
		bar(cx - 0.3f * s, cy - 0.8f * t, cx + 0.9f * s, cy + 0.8f * t, c);
		triangle(cx - 0.9f * s, cy, cx - 0.25f * s, cy - 0.6f * s, cx - 0.25f * s, cy + 0.6f * s, c, 1.f);
		break;
	case Icon::Book: // open book: two pages either side of the spine, with a ribbon hanging out
		bar(cx - 0.95f * s, cy - 0.6f * s, cx - 0.1f * s, cy + 0.75f * s, c);
		bar(cx + 0.1f * s, cy - 0.6f * s, cx + 0.95f * s, cy + 0.75f * s, c);
		bar(cx + 0.45f * s, cy - 0.95f * s, cx + 0.45f * s + t, cy - 0.6f * s, c);
		break;
	}
}

constexpr float MOUSE_W = 2.6f;
constexpr float MOUSE_H = 3.6f;

// Small mouse with the used button lit, bottom left corner at (x, y), `scale` times MOUSE_W x MOUSE_H.
void drawMouse(float x, float y, MouseInput m, float scale) {
	const float w = MOUSE_W * scale;
	const float h = MOUSE_H * scale;
	constexpr float SPLIT = 0.55f; // buttons take the top 45 %
	Rect body = {x, y, w, h};
	fillRect(body, WELL, WELL, 1.f);
	float by = y + h * SPLIT;
	float third = w / 3;
	if (m == MouseInput::Left)
		fillRect({x, by, third * 1.5f, h - h * SPLIT}, GOLD, GOLD, 1.f);
	if (m == MouseInput::Right)
		fillRect({x + third * 1.5f, by, third * 1.5f, h - h * SPLIT}, GOLD, GOLD, 1.f);
	if (m == MouseInput::Middle)
		fillRect({x + third * 1.1f, by + 0.2f, third * 0.8f, h * 0.3f}, GOLD, GOLD, 1.f);
	line(x, by, x + w, by, GOLD_DIM, 1.f, 1.f);
	if (m != MouseInput::Middle)
		line(x + w / 2, by, x + w / 2, y + h, GOLD_DIM, 1.f, 1.f);
	strokeRect(body, GOLD_DIM, 1.f, 1.f);
}

// ---- save slots ------------------------------------------------------------

void saveToSlot(int slot) {
	Game().Save(SaveSlots::FileName(slot).c_str());
	Game().saves.Record(slot, Game().dungeon.LevelNumber());
}
} // namespace

MainMenu::MainMenu() {
	inGame = false;
	saveD = false;
	loadD = false;
}

MainMenu::~MainMenu() {}

// The menu exists before the GL context, so the fonts and the credits sheet load on the first frame.
void MainMenu::LoadAssets() {
	if (assetsLoaded)
		return;
	assetsLoaded = true;
	creditsSheet.LoadPNG("textures/ui/credits.png", TexFilter::Flat);
	loadScreenFonts(title, heading, body, small, 8.f);
}

void MainMenu::ShowToast(const std::string& text) { toast.Show(text, GameClock::now()); }

// ---- drawing ---------------------------------------------------------------

void MainMenu::Draw() {
	LoadAssets();
	BeginCanvas();
	if (creditsD) {
		DrawBackground("Credits");
		DrawCredits();
		DrawBackButton();
		DrawFooter("Esc: back");
	} else if (optionsD) {
		DrawBackground("Options");
		DrawOptions();
		DrawBackButton();
		DrawFooterHint();
	} else if (saveD || loadD) {
		DrawBackground(saveD ? "Save Game" : "Load Game");
		DrawSlots();
		DrawBackButton();
		DrawFooter(saveD ? "Click a slot to save    Esc: back" : "Click a slot to load    Esc: back");
	} else {
		DrawBackground("Dungeon Crawl");
		DrawButtons();
		DrawFooter(inGame ? "Esc: return to game" : "");
	}
	EndFrame();
}

void MainMenu::BeginCanvas() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	Rect area = visibleArea();
	glOrtho(area.x, area.x + area.w, area.y, area.y + area.h, -200, 200);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
}

void MainMenu::EndFrame() {
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glColor3f(1, 1, 1);
}

void MainMenu::DrawBackground(const char* caption) {
	bool wide = saveD || loadD || optionsD;
	backdrop(visibleArea(), Game().assets.textures.loadingBackground.ID());
	titleBar(title, CENTRE, caption, wide ? 58.f : 44.f);
	panel(creditsD ? CREDITS_PANEL : optionsD ? OPTIONS_PANEL : (wide ? SLOT_PANEL : MENU_PANEL), 0.9f);
}

void MainMenu::DrawButtons() {
	ButtonList buttons = menuButtons(inGame);
	for (int i = 0; i < buttons.count; i++) {
		const MenuButton& b = buttons[i];
		bool isHovered = hovered == i;
		bool held = isHovered && pressed == i;
		Rect r = tile(buttonRect(i, buttons.count), b.primary ? TileStyle::Lapis : TileStyle::Stone, isHovered, held);

		Rect well = iconWell(r, 1.3f);
		fillRect(well, b.primary ? LAPIS_DARK : WELL, b.primary ? LAPIS_HELD_BOTTOM : WELL, 0.9f);
		strokeRect(well, isHovered ? GOLD : GOLD_DIM, 0.8f, 1.f);
		drawIcon(b.icon, well.cx(), well.cy(), well.h * 0.3f, isHovered ? GOLD_BRIGHT : GOLD);

		beginText();
		Color c = isHovered ? TEXT_HOVER : (b.primary ? GOLD : LABEL);
		text(heading, well.x + well.w + 4.f, r.y + 2.1f, b.label, c);
		beginShapes();
	}
}

void MainMenu::DrawSlots() {
	for (int slot = 0; slot < SaveSlots::COUNT; slot++)
		DrawSlot(slot);
}

void MainMenu::DrawSlot(int slot) {
	SaveSlots::Info info = Game().saves.Describe(slot);
	bool enabled = saveD || info.used;
	bool isHovered = enabled && hovered == slot;
	bool held = isHovered && pressed == slot;
	TileStyle style = !enabled ? TileStyle::Disabled : TileStyle::Stone;
	Rect r = tile(slotRect(slot), style, isHovered, held);

	// Slot number on a lapis seal when the slot holds a save.
	Rect well = iconWell(r, 2.f);
	if (info.used)
		fillRect(well, isHovered ? LAPIS_HOVER_TOP : LAPIS, isHovered ? LAPIS_HOVER_BOTTOM : LAPIS_DARK, 1.f);
	else
		fillRect(well, WELL, WELL, 0.9f);
	strokeRect(well, info.used ? GOLD : BRONZE, 1.f, info.used ? 1.5f : 1.f);

	float textX = well.x + well.w + 3.5f;
	if (info.used) // divider between the level and the date
		line(textX, r.y + 7.4f, r.x + r.w - 3.f, r.y + 7.4f, BRONZE, 1.f, 1.f);

	beginText();
	char number[12];
	std::snprintf(number, sizeof(number), "%d", slot + 1);
	textCentered(title, well.cx(), well.y + 2.2f, number, info.used ? GOLD : LABEL_DIM);

	if (info.used) {
		char level[24] = "Saved game";
		if (info.level > 0)
			std::snprintf(level, sizeof(level), "Level %d", info.level);
		text(heading, textX, r.y + 8.8f, level, isHovered ? GOLD_BRIGHT : GOLD);
		text(body, textX, r.y + 3.f, info.when.c_str(), isHovered ? LABEL : LABEL_DIM);
		if (saveD && isHovered) {
			const char* overwrite = "Overwrite";
			text(small, r.x + r.w - small.TextWidth(overwrite) - 2.5f, r.y + 10.f, overwrite, {0.9f, 0.45f, 0.3f});
		}
	} else {
		text(heading, textX, r.y + 5.6f, "Empty slot", isHovered ? GOLD_BRIGHT : LABEL_DIM);
	}
	beginShapes();
}

void MainMenu::DrawOptions() {
	// Tab strip along the top of the panel; the active tab is lapis and sits on the rule.
	for (int tab = 0; tab < static_cast<int>(TABS.size()); tab++) {
		bool active = tab == optionsTab;
		bool isHovered = hovered == TAB_BASE + tab;
		Rect r = tile(tabRect(tab), active ? TileStyle::Lapis : TileStyle::Stone, isHovered && !active, false);
		beginText();
		textCentered(heading, r.cx(), r.y + 1.4f, TABS[static_cast<size_t>(tab)], active ? GOLD : LABEL);
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
		const bool isHovered = hovered == target || dragging == target;
		Rect band = optionBand(i);
		Rect control = optionControl(i, row.kind);
		if (i % 2 == 0)
			fillRect(band, {1, 1, 1}, {1, 1, 1}, 0.035f);

		char value[24] = "";
		bool on = false;
		bool enabled = true;
		if (row.kind == OptionKind::Switch) {
			on = switchOf(row.id);
			Rect r = tile(control, on ? TileStyle::Lapis : TileStyle::Stone, isHovered, isHovered && pressed == target);
			std::snprintf(value, sizeof(value), "%s", on ? "On" : "Off");
			control = r;
		} else if (row.kind == OptionKind::Choice) {
			enabled = !settings.display.fullscreen; // the window size only matters in a window
			Rect r = tile(control, enabled ? TileStyle::Stone : TileStyle::Disabled, enabled && isHovered,
						  enabled && isHovered && pressed == target);
			std::snprintf(value, sizeof(value), "%d x %d", settings.display.width, settings.display.height);
			control = r;
		} else {
			// Track, the filled part, the knob; the number to the right.
			int level = sliderOf(row.id);
			float t = static_cast<float>(level) / 100.f;
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
		text(body, band.x + 3.f, band.y + 3.4f, row.name, LABEL);
		text(small, band.x + 3.f, band.y + 0.6f, row.hint, LABEL_DIM);
		if (row.kind == OptionKind::Slider)
			text(heading, control.x + control.w + 2.5f, control.y + 1.2f, value, isHovered ? GOLD_BRIGHT : GOLD);
		else
			textCentered(heading, control.cx(), control.y + 1.2f, value,
						 !enabled ? LABEL_DIM : (on || row.kind == OptionKind::Choice ? GOLD : LABEL));
		beginShapes();
	}
}

// A key cell: the key cap, or the mouse with its button lit; a dash when unbound; lapis while it waits for input.
void MainMenu::DrawControlCell(int target, const Rect& r) {
	const int action = (target - BIND_BASE) / BINDING_SLOTS;
	const int slot = (target - BIND_BASE) % BINDING_SLOTS;
	const InputKey& key = Game().settings.controls.Of(static_cast<BindAction>(action)).Slot(slot);
	const bool waiting = capturing == target;
	const bool isHovered = hovered == target;

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
		textCentered(small, r.cx(), r.y + 0.6f, slot == MOUSE_SLOT ? "Click" : "Press", GOLD_BRIGHT);
		beginShapes();
	} else if (slot == MOUSE_SLOT && key.Bound()) {
		constexpr float SCALE = 0.75f;
		drawMouse(r.cx() - MOUSE_W * SCALE / 2, r.cy() - MOUSE_H * SCALE / 2, mouseInput(key), SCALE);
	} else {
		beginText();
		textCentered(small, r.cx(), r.y + 0.6f, key.Bound() ? keyCap(key).c_str() : "-",
					 key.Bound() ? (isHovered ? GOLD_BRIGHT : GOLD) : LABEL_DIM);
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

	bool isHovered = hovered == RESET_CONTROLS;
	Rect reset = tile(resetButton(), TileStyle::Stone, isHovered, isHovered && pressed == RESET_CONTROLS);
	beginText();
	textCentered(small, reset.cx(), reset.y + 0.7f, "Reset to defaults", isHovered ? GOLD_BRIGHT : LABEL);
	textCentered(small, (left + right) / 2, FIXED_KEYS_Y,
				 "Fixed:  Esc menu / back    F1 toon shading    F3 hitboxes    mouse movement looks around", LABEL_DIM);
	beginShapes();
}

void MainMenu::DrawCredits() {
	texturedRect(CREDITS_SHEET, creditsSheet.ID(), {1, 1, 1});
	beginShapes();
	strokeRect(CREDITS_SHEET, GOLD_DIM, 1.f, 1.f);
}

void MainMenu::DrawBackButton() {
	bool isHovered = hovered == BACK;
	Rect r = tile(BACK_BUTTON, TileStyle::Stone, isHovered, isHovered && pressed == BACK);
	Color c = isHovered ? GOLD_BRIGHT : LABEL;
	float labelW = heading.TextWidth("Back");
	float x0 = r.cx() - (labelW + 6.f) / 2;
	drawIcon(Icon::Back, x0 + 2.f, r.cy(), 2.f, c);
	beginText();
	text(heading, x0 + 6.f, r.y + 1.6f, "Back", c);
	beginShapes();
}

void MainMenu::DrawFooter(const char* hint) {
	beginText();
	if (float alpha = toast.Alpha(GameClock::now()); alpha > 0.f) // under the Back button where there is one
		textCentered(body, CENTRE, optionsD ? 5.6f : 12.f, toast.text.c_str(), {1.f, 0.9f, 0.6f}, alpha);
	textCentered(small, CENTRE, 2.2f, hint, LABEL_DIM);
}

void MainMenu::DrawFooterHint() {
	if (capturing == NONE) {
		DrawFooter(optionsTab == CONTROLS_TAB ? "Click a key to change it    Esc: back" : "Esc: back");
		return;
	}
	auto action = static_cast<BindAction>((capturing - BIND_BASE) / BINDING_SLOTS);
	bool mouse = (capturing - BIND_BASE) % BINDING_SLOTS == MOUSE_SLOT;
	std::string hint = std::string(mouse ? "Click a mouse button for " : "Press a key for ") + bindActionLabel(action) +
					   "    Delete: clear    Esc: cancel";
	DrawFooter(hint.c_str());
}

// ---- input -----------------------------------------------------------------

int MainMenu::TargetAt(int x, int y) {
	float cx = 0.f;
	float cy = 0.f;
	ui::toCanvas(visibleArea(), Game().render.resX, Game().render.resY, x, y, cx, cy);

	if (creditsD)
		return BACK_BUTTON.contains(cx, cy) ? BACK : NONE;
	if (optionsD) {
		for (int tab = 0; tab < static_cast<int>(TABS.size()); tab++)
			if (tabRect(tab).contains(cx, cy))
				return TAB_BASE + tab;
		if (optionsTab == CONTROLS_TAB) {
			for (int action = 0; action < BIND_ACTION_COUNT; action++)
				for (int slot = 0; slot < BINDING_SLOTS; slot++)
					if (controlCell(action, slot).contains(cx, cy))
						return BIND_BASE + action * BINDING_SLOTS + slot;
			if (resetButton().contains(cx, cy))
				return RESET_CONTROLS;
		}
		OptionRows rows = optionRows(optionsTab);
		for (int i = 0; i < rows.count; i++) {
			Rect r = optionControl(i, rows[i].kind);
			if (rows[i].kind == OptionKind::Slider) // the knob reaches past the ends
				r = {r.x - 1.5f, r.y, r.w + 3.f, r.h};
			if (r.contains(cx, cy))
				return OPTION_BASE + i;
		}
		return BACK_BUTTON.contains(cx, cy) ? BACK : NONE;
	}
	if (saveD || loadD) {
		for (int slot = 0; slot < SaveSlots::COUNT; slot++)
			if (slotRect(slot).contains(cx, cy))
				return saveD || Game().saves.Describe(slot).used ? slot : NONE;
		return BACK_BUTTON.contains(cx, cy) ? BACK : NONE;
	}
	int count = menuButtons(inGame).count;
	for (int i = 0; i < count; i++)
		if (buttonRect(i, count).contains(cx, cy))
			return i;
	return NONE;
}

void MainMenu::Activate(int target) {
	if (target == BACK) {
		ResetSubScreens();
		return;
	}
	if (optionsD) {
		if (target >= BIND_BASE)
			StartCapture(target);
		else if (target == RESET_CONTROLS) {
			Game().settings.controls = Bindings{};
			Game().ApplySettings(true);
			ShowToast("Controls reset to the defaults");
		} else if (target >= OPTION_BASE)
			ActivateOption(target - OPTION_BASE);
		else
			optionsTab = target - TAB_BASE;
		return;
	}
	if (saveD) {
		saveToSlot(target);
		ResetSubScreens();
		ShowToast("Saved to slot " + std::to_string(target + 1));
		return;
	}
	if (loadD) {
		Game().LoadSave(SaveSlots::FileName(target).c_str());
		ResetSubScreens();
		Game().ui.screen = Screen::Gameplay;
		inGame = true;
		return;
	}

	switch (menuButtons(inGame)[target].action) {
	case MenuAction::NewGame:
		Game().ui.screen = Screen::Gameplay;
		Game().NewGame();
		inGame = true;
		break;
	case MenuAction::Resume:
		Game().ui.screen = Screen::Gameplay;
		break;
	case MenuAction::Journal:
		Game().ui.screen = Screen::Journal;
		break;
	case MenuAction::Save:
		saveD = true;
		break;
	case MenuAction::Load:
		loadD = true;
		break;
	case MenuAction::Options:
		optionsD = true;
		break;
	case MenuAction::Credits:
		creditsD = true;
		break;
	case MenuAction::MainMenu:
		inGame = false;
		break;
	case MenuAction::Quit: // glutMainLoop returns, main cleans up
		glutLeaveMainLoop();
	}
}

void MainMenu::ActivateOption(int row) {
	const OptionRow option = optionRows(optionsTab)[row];
	Settings::Display& display = Game().settings.display;
	if (option.id == OptionId::WindowSize) {
		if (display.fullscreen) {
			ShowToast("Leave fullscreen to pick a window size");
			return;
		}
		// The next larger size, after the largest the smallest.
		WindowSize next = WINDOW_SIZES[0];
		for (const WindowSize& size : WINDOW_SIZES)
			if (size.w > display.width || (size.w == display.width && size.h > display.height)) {
				next = size;
				break;
			}
		display.width = next.w;
		display.height = next.h;
		glutReshapeWindow(next.w, next.h);
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

void MainMenu::SetSlider(int row, int x) {
	const OptionRow option = optionRows(optionsTab)[row];
	float cx = 0.f;
	float cy = 0.f;
	ui::toCanvas(visibleArea(), Game().render.resX, Game().render.resY, x, 0, cx, cy);
	Rect track = optionControl(row, option.kind);
	float t = std::clamp((cx - track.x) / track.w, 0.f, 1.f);
	sliderOf(option.id) = static_cast<int>(std::lround(t * 20.f)) * 5; // steps of 5
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

void MainMenu::MouseFunction(int button, int state, int x, int y) {
	int target = TargetAt(x, y);
	hovered = target;

	// Waiting for a binding: a mouse button binds the mouse cell; on a key cell a click cancels.
	if (capturing != NONE) {
		bool mouseCell = (capturing - BIND_BASE) % BINDING_SLOTS == MOUSE_SLOT;
		bool isButton = button == MOUSE_LEFT_BUTTON || button == MOUSE_MIDDLE_BUTTON || button == MOUSE_RIGHT_BUTTON;
		if (state == GLUT_DOWN && isButton) {
			if (mouseCell)
				Bind(InputKey::Mouse(button));
			else
				capturing = NONE;
		}
		pressed = NONE;
		return;
	}

	// A slider follows the mouse while the button is down; the file is written when it comes up.
	if (dragging != NONE) {
		if (state == GLUT_UP) {
			const OptionRow option = optionRows(optionsTab)[dragging - OPTION_BASE];
			dragging = NONE;
			Game().ApplySettings(true);
			if (option.id == OptionId::Effects)
				Game().assets.sounds.lever.Play(); // a sample at the new volume
		}
		return;
	}
	if (button != MOUSE_LEFT_BUTTON)
		return;
	if (state == GLUT_DOWN && optionsD && target >= OPTION_BASE && target < RESET_CONTROLS &&
		optionRows(optionsTab)[target - OPTION_BASE].kind == OptionKind::Slider) {
		dragging = target;
		SetSlider(target - OPTION_BASE, x);
		return;
	}

	if (state != GLUT_UP) {
		pressed = target;
		return;
	}
	bool clicked = target != NONE && target == pressed;
	pressed = NONE;
	if (clicked) {
		hovered = NONE; // the next screen has its own targets
		Activate(target);
	}
}

void MainMenu::MousePassiveMotion(int x, int y) { hovered = TargetAt(x, y); }

void MainMenu::MouseDrag(int x, int y) {
	if (dragging != NONE)
		SetSlider(dragging - OPTION_BASE, x);
	else
		hovered = TargetAt(x, y);
}

void MainMenu::ResetSubScreens() {
	creditsD = false;
	saveD = false;
	loadD = false;
	optionsD = false;
	hovered = NONE;
	pressed = NONE;
	capturing = NONE;
	dragging = NONE;
}
