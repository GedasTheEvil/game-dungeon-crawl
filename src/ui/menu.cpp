#include "menu.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sys/stat.h>
#include <vector>
#include <GL/gl.h>
#include "../graphics/gl_includes.h"
#include "../state/game_state.h"
#include "../core/service_locator.h"
#include "../test/scenario.h"
#include "ui_draw.h"

// Same 160 x 100 canvas (y up) and look as the inventory: a framed panel on the carved slate, lapis and stone tiles.
namespace {
constexpr float CANVAS_W = 160.f;
constexpr float CANVAS_H = 100.f;
constexpr float CENTRE = CANVAS_W / 2;

using namespace ui;

constexpr int SLOT_COUNT = 6;
constexpr int TOAST_MS = 2200;
constexpr int TOAST_FADE_MS = 600;
constexpr int GLUT_BUTTON_UP = 1;

constexpr Color LABEL = {0.86f, 0.72f, 0.47f};
constexpr Color LABEL_DIM = {0.55f, 0.45f, 0.30f};
constexpr Color LAPIS_HOVER_TOP = {0.20f, 0.40f, 0.78f};
constexpr Color LAPIS_HOVER_BOTTOM = {0.09f, 0.20f, 0.46f};
constexpr Color STONE_HOVER_TOP = {0.33f, 0.25f, 0.15f};
constexpr Color STONE_HOVER_BOTTOM = {0.17f, 0.13f, 0.08f};
constexpr Color WELL = {0.07f, 0.055f, 0.04f};

// ---- layout ----------------------------------------------------------------

constexpr Rect MENU_PANEL = {46, 19, 68, 64};
constexpr float BUTTON_W = 56.f;
constexpr float BUTTON_H = 9.f;
constexpr float BUTTON_STEP = 11.5f;
constexpr float FIRST_BUTTON_Y = 69.5f;

constexpr Rect SLOT_PANEL = {12, 22, 136, 62};
constexpr float SLOT_W = 62.f;
constexpr float SLOT_H = 16.f;
constexpr float SLOT_GAP_X = 4.f;
constexpr float SLOT_GAP_Y = 3.5f;
constexpr Rect BACK_BUTTON = {63, 9.5f, 34, 8};

constexpr Rect OPTIONS_PANEL = {12, 20, 136, 64};
constexpr float TAB_W = 30.f;
constexpr float TAB_H = 7.f;
constexpr float TABLE_TOP = 68.8f; // header baseline
constexpr float ROW_H = 4.05f;
constexpr float ACTION_X = OPTIONS_PANEL.x + 8;
constexpr float KEYS_X = OPTIONS_PANEL.x + 52;
constexpr float MOUSE_X = OPTIONS_PANEL.x + 100;

Rect buttonRect(int index) {
	return {CENTRE - BUTTON_W / 2, FIRST_BUTTON_Y - static_cast<float>(index) * BUTTON_STEP, BUTTON_W, BUTTON_H};
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

Rect visibleArea() { return ui::visibleArea(CANVAS_W, CANVAS_H, GAME_STATE.render.resX, GAME_STATE.render.resY); }

// ---- icons -----------------------------------------------------------------

enum class Icon : std::uint8_t { Play, Save, Load, Gear, Ankh, Exit, Pyramid, Back };
enum class MenuAction : std::uint8_t { NewGame, Resume, Save, Load, Options, Credits, MainMenu, Quit };

struct MenuButton {
	const char* label;
	Icon icon;
	MenuAction action;
	bool primary;
};

constexpr int BUTTON_COUNT = 5;
constexpr std::array<MenuButton, BUTTON_COUNT> MAIN_BUTTONS = {{
	{"New Game", Icon::Play, MenuAction::NewGame, true},
	{"Load Game", Icon::Load, MenuAction::Load, false},
	{"Options", Icon::Gear, MenuAction::Options, false},
	{"Credits", Icon::Ankh, MenuAction::Credits, false},
	{"Exit", Icon::Exit, MenuAction::Quit, false},
}};
constexpr std::array<MenuButton, BUTTON_COUNT> IN_GAME_BUTTONS = {{
	{"Return to Game", Icon::Play, MenuAction::Resume, true},
	{"Save Game", Icon::Save, MenuAction::Save, false},
	{"Load Game", Icon::Load, MenuAction::Load, false},
	{"Options", Icon::Gear, MenuAction::Options, false},
	{"Main Menu", Icon::Pyramid, MenuAction::MainMenu, false},
}};

// ---- options: controls tab --------------------------------------------------

constexpr std::array<const char*, 1> TABS = {"Controls"};

enum class MouseInput : std::uint8_t { None, Left, Middle, Right, Move };

// `keys`: space separated key caps; "/" and "," are drawn as plain separators between them.
struct ControlRow {
	const char* action;
	const char* keys;
	MouseInput mouse;
};

constexpr std::array<ControlRow, 11> CONTROLS = {{
	{"Move left / right", "A / D , Left / Right", MouseInput::None},
	{"Climb up / down (ladders)", "W / S , Up / Down", MouseInput::None},
	{"Jump", "Space", MouseInput::Right},
	{"Sprint (hold)", "Shift", MouseInput::None},
	{"Attack", "V , Enter", MouseInput::Left},
	{"Interact: pick up, lever, riddle", "E , F12", MouseInput::Middle},
	{"Look around", "PgUp / PgDn , Home / End", MouseInput::Move},
	{"Inventory", "I", MouseInput::None},
	{"Menu / back", "Esc", MouseInput::None},
	{"Cartoon shading", "F1", MouseInput::None},
	{"Original models", "F2", MouseInput::None},
}};

const char* mouseLabel(MouseInput m) {
	switch (m) {
	case MouseInput::Left:
		return "Left button";
	case MouseInput::Middle:
		return "Middle button";
	case MouseInput::Right:
		return "Right button";
	case MouseInput::Move:
		return "Move";
	case MouseInput::None:
		break;
	}
	return "";
}

const std::array<MenuButton, BUTTON_COUNT>& menuButtons(bool inGame) { return inGame ? IN_GAME_BUTTONS : MAIN_BUTTONS; }

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
	}
}

// Small mouse with the used button lit, bottom left corner at (x, y).
void drawMouse(float x, float y, MouseInput m) {
	constexpr float W = 2.6f;
	constexpr float H = 3.6f;
	constexpr float SPLIT = 0.55f; // buttons take the top 45 %
	Rect body = {x, y, W, H};
	fillRect(body, WELL, WELL, 1.f);
	float by = y + H * SPLIT;
	float third = W / 3;
	if (m == MouseInput::Left)
		fillRect({x, by, third * 1.5f, H - H * SPLIT}, GOLD, GOLD, 1.f);
	if (m == MouseInput::Right)
		fillRect({x + third * 1.5f, by, third * 1.5f, H - H * SPLIT}, GOLD, GOLD, 1.f);
	if (m == MouseInput::Middle)
		fillRect({x + third * 1.1f, by + 0.2f, third * 0.8f, H * 0.3f}, GOLD, GOLD, 1.f);
	line(x, by, x + W, by, GOLD_DIM, 1.f, 1.f);
	if (m != MouseInput::Middle)
		line(x + W / 2, by, x + W / 2, y + H, GOLD_DIM, 1.f, 1.f);
	strokeRect(body, GOLD_DIM, 1.f, 1.f);
}

// ---- tiles -----------------------------------------------------------------

enum class TileStyle : std::uint8_t { Stone, Lapis, Disabled };

// Raised tile shared by the buttons and the save slots; returns the rect as drawn (it sinks while held).
Rect drawTile(Rect r, TileStyle style, bool hovered, bool held) {
	if (style == TileStyle::Disabled) {
		fillRect(r, {0.10f, 0.08f, 0.06f}, {0.07f, 0.055f, 0.04f}, 0.9f);
		strokeRect(r, BRONZE, 0.6f, 1.f);
		return r;
	}

	fillRect({r.x + 0.5f, r.y - 0.7f, r.w, r.h}, BLACK, BLACK, held ? 0.f : 0.4f);
	if (held)
		r.y -= 0.4f;
	if (hovered) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		ring(r, 2.2f, GOLD, 0.4f, 0.f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	bool lapis = style == TileStyle::Lapis;
	Color top = lapis ? LAPIS : STONE_TOP;
	Color bottom = lapis ? LAPIS_DARK : STONE_BOTTOM;
	if (held) {
		top = lapis ? LAPIS_DARK : STONE_BOTTOM;
		bottom = lapis ? Color{0.03f, 0.07f, 0.18f} : Color{0.07f, 0.05f, 0.035f};
	} else if (hovered) {
		top = lapis ? LAPIS_HOVER_TOP : STONE_HOVER_TOP;
		bottom = lapis ? LAPIS_HOVER_BOTTOM : STONE_HOVER_BOTTOM;
	}
	fillRect(r, top, bottom, 1.f);
	if (!held) // top highlight
		fillRect({r.x, r.y + r.h - 1.f, r.w, 1.f}, {1, 1, 1}, {1, 1, 1}, hovered ? 0.16f : 0.08f);

	if (hovered)
		strokeRect(r, GOLD_BRIGHT, 1.f, 2.5f);
	else
		strokeRect(r, lapis ? GOLD : BRONZE, 1.f, lapis ? 2.5f : 1.5f);
	if (lapis || hovered)
		strokeRect(r.inset(0.8f), GOLD_DIM, 0.6f, 1.f);
	return r;
}

// Dark square on the left of a tile that holds its icon or number.
Rect iconWell(const Rect& tile, float inset) {
	float size = tile.h - 2 * inset;
	return {tile.x + inset, tile.y + inset, size, size};
}

// ---- save slots ------------------------------------------------------------

std::string slotFilename(int slot) { return "Saves/save" + std::to_string(slot) + ".sav"; }

struct SlotInfo {
	bool used = false;
	int level = 0;		// campaign level the save was made on, 0 if unknown
	char when[40] = {}; // date and time of the save file
};

// The slot names list stores "<level>_<month>-<day>_<hh:mm>"; the file time gives the full date.
SlotInfo slotInfo(int slot) {
	SlotInfo info;
	struct stat st = {};
	if (stat(slotFilename(slot).c_str(), &st) != 0)
		return info;
	info.used = true;
	if (std::sscanf(GAME_STATE.saveNames[slot].name, "%d_", &info.level) != 1)
		info.level = 0;
	time_t t = st.st_mtime;
	std::strftime(info.when, sizeof(info.when), "%d %b %Y, %H:%M", std::localtime(&t));
	return info;
}

void persistSaveNameList() {
	std::ofstream f("Saves/gamelist.dat");
	for (int a = 0; a < SLOT_COUNT; a++)
		f << GAME_STATE.saveNames[a].name << "\n";
}

std::string saveLabel() {
	time_t t = time(nullptr);
	struct tm* lt = localtime(&t);
	char buf[25];
	std::snprintf(buf, sizeof(buf), "%02d_%02d-%02d_%02d:%02d", GAME_STATE.curMap, lt->tm_mon + 1, lt->tm_mday,
				  lt->tm_hour, lt->tm_min);
	return buf;
}

void saveToSlot(int slot) {
	GAME_STATE.Save(slotFilename(slot).c_str());
	auto& name = GAME_STATE.saveNames[slot].name;
	strncpy(name, saveLabel().c_str(), sizeof(name) - 1);
	name[sizeof(name) - 1] = '\0';
	persistSaveNameList();
}
} // namespace

MainMenu::MainMenu() {
	show = true;
	inGame = false;
	credits = false;
	saveD = false;
	loadD = false;
	creditsTimer = std::make_unique<timer>(10000);
}

MainMenu::~MainMenu() {}

// The menu exists before the GL context, so the font textures load on the first frame.
void MainMenu::LoadFonts() {
	if (fontsLoaded)
		return;
	fontsLoaded = true;
	title.Load("Fonts/papyrus.png", 8.f, 0.3f, true);
	heading.Load("Fonts/papyrus.png", 5.f, 0.16f, true);
	body.Load("Fonts/papyrus.png", 3.6f, 0.1f, true);
	small.Load("Fonts/papyrus.png", 3.f, 0.08f, true);
}

void MainMenu::ShowToast(const std::string& text) {
	toast = text;
	toastStartMs = GameClock::now();
}

// ---- drawing ---------------------------------------------------------------

void MainMenu::Draw() {
	if (credits) {
		GAME_STATE.ui.wlc->DrawCredits();
		if (creditsTimer->TimePassed())
			credits = false;
		return;
	}

	LoadFonts();
	BeginCanvas();
	if (optionsD) {
		DrawBackground("Options");
		DrawOptions();
		DrawBackButton();
		DrawFooter("Esc: back");
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
	glFlush();

	Scenario::onFrameRendered();
	glutSwapBuffers();
}

void MainMenu::DrawBackground(const char* caption) {
	Rect area = visibleArea();
	bool wide = saveD || loadD || optionsD;

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
	float titleHalf = title.TextWidth(caption) / 2 + 4;
	float reach = wide ? 58.f : 44.f;
	constexpr float RULE_Y = 92.f;
	line(CENTRE - reach, RULE_Y, CENTRE - titleHalf, RULE_Y, GOLD_DIM, 1.f, 2.f);
	line(CENTRE + titleHalf, RULE_Y, CENTRE + reach, RULE_Y, GOLD_DIM, 1.f, 2.f);
	diamond(CENTRE - reach, RULE_Y, 1.1f, GOLD, 1.f);
	diamond(CENTRE + reach, RULE_Y, 1.1f, GOLD, 1.f);
	diamond(CENTRE - titleHalf + 1.5f, RULE_Y, 0.7f, GOLD, 1.f);
	diamond(CENTRE + titleHalf - 1.5f, RULE_Y, 0.7f, GOLD, 1.f);

	panel(optionsD ? OPTIONS_PANEL : (wide ? SLOT_PANEL : MENU_PANEL), 0.9f);

	beginText();
	textCentered(title, CENTRE, 88.f, caption, GOLD);
	beginShapes();
}

void MainMenu::DrawButtons() {
	const auto& buttons = menuButtons(inGame);
	for (int i = 0; i < BUTTON_COUNT; i++) {
		const MenuButton& b = buttons[static_cast<size_t>(i)];
		bool isHovered = hovered == i;
		bool held = isHovered && pressed == i;
		Rect r = drawTile(buttonRect(i), b.primary ? TileStyle::Lapis : TileStyle::Stone, isHovered, held);

		Rect well = iconWell(r, 1.3f);
		fillRect(well, b.primary ? LAPIS_DARK : WELL, b.primary ? Color{0.03f, 0.07f, 0.18f} : WELL, 0.9f);
		strokeRect(well, isHovered ? GOLD : GOLD_DIM, 0.8f, 1.f);
		drawIcon(b.icon, well.cx(), well.cy(), well.h * 0.3f, isHovered ? GOLD_BRIGHT : GOLD);

		beginText();
		Color c = isHovered ? Color{1.f, 0.92f, 0.65f} : (b.primary ? GOLD : LABEL);
		text(heading, well.x + well.w + 4.f, r.y + 2.1f, b.label, c);
		beginShapes();
	}
}

void MainMenu::DrawSlots() {
	for (int slot = 0; slot < SLOT_COUNT; slot++)
		DrawSlot(slot);
}

void MainMenu::DrawSlot(int slot) {
	SlotInfo info = slotInfo(slot);
	bool enabled = saveD || info.used;
	bool isHovered = enabled && hovered == slot;
	bool held = isHovered && pressed == slot;
	TileStyle style = !enabled ? TileStyle::Disabled : TileStyle::Stone;
	Rect r = drawTile(slotRect(slot), style, isHovered, held);

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
		text(body, textX, r.y + 3.f, info.when, isHovered ? LABEL : LABEL_DIM);
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
		Rect r = drawTile(tabRect(tab), active ? TileStyle::Lapis : TileStyle::Stone, isHovered && !active, false);
		beginText();
		textCentered(heading, r.cx(), r.y + 1.4f, TABS[static_cast<size_t>(tab)], active ? GOLD : LABEL);
		beginShapes();
	}
	float ruleY = tabRect(0).y;
	float left = OPTIONS_PANEL.x + 5;
	float right = OPTIONS_PANEL.x + OPTIONS_PANEL.w - 5;
	line(left, ruleY, right, ruleY, GOLD_DIM, 1.f, 1.5f);

	// Controls table: header, then one striped row per action.
	float headerY = TABLE_TOP;
	line(left, headerY - 1.1f, right, headerY - 1.1f, BRONZE, 1.f, 1.f);
	for (size_t i = 0; i < CONTROLS.size(); i++) {
		float y = headerY - 1.1f - static_cast<float>(i + 1) * ROW_H;
		if (i % 2 == 0)
			fillRect({left, y, right - left, ROW_H}, {1, 1, 1}, {1, 1, 1}, 0.035f);
	}

	beginText();
	text(small, ACTION_X, headerY, "Action", LABEL);
	text(small, KEYS_X, headerY, "Keyboard", LABEL);
	text(small, MOUSE_X, headerY, "Mouse", LABEL);
	beginShapes();

	for (size_t i = 0; i < CONTROLS.size(); i++) {
		const ControlRow& row = CONTROLS[i];
		float rowY = headerY - 1.1f - static_cast<float>(i + 1) * ROW_H;
		float baseline = rowY + 0.6f;

		// Key caps: dark keys with a gold rim, separators as plain text.
		struct Word {
			std::string text;
			float x;
			bool cap;
		};
		std::vector<Word> words;
		float x = KEYS_X;
		std::string keys = row.keys;
		for (size_t pos = 0; pos < keys.size();) {
			size_t end = keys.find(' ', pos);
			if (end == std::string::npos)
				end = keys.size();
			std::string word = keys.substr(pos, end - pos);
			pos = end + 1;
			bool cap = word != "/" && word != ",";
			if (word == ",")
				x -= 1.f; // the comma hugs the key before it
			words.push_back({word, x, cap});
			float w = small.TextWidth(word.c_str());
			if (cap) {
				Rect key = {x, rowY + 0.45f, w + 2.2f, ROW_H - 0.9f};
				fillRect({key.x, key.y - 0.25f, key.w, key.h}, BLACK, BLACK, 0.5f);
				fillRect(key, {0.19f, 0.15f, 0.10f}, {0.10f, 0.08f, 0.055f}, 1.f);
				strokeRect(key, GOLD_DIM, 1.f, 1.f);
				x += key.w + 1.2f;
			} else {
				x += w + 1.2f;
			}
		}

		if (row.mouse != MouseInput::None)
			drawMouse(MOUSE_X, rowY + 0.25f, row.mouse);

		beginText();
		text(body, ACTION_X, baseline - 0.7f, row.action, LABEL);
		for (const Word& word : words)
			text(small, word.cap ? word.x + 1.1f : word.x, baseline, word.text.c_str(), word.cap ? GOLD : LABEL_DIM);
		if (row.mouse != MouseInput::None)
			text(small, MOUSE_X + 4.f, baseline, mouseLabel(row.mouse), GOLD);
		beginShapes();
	}
}

void MainMenu::DrawBackButton() {
	bool isHovered = hovered == BACK;
	Rect r = drawTile(BACK_BUTTON, TileStyle::Stone, isHovered, isHovered && pressed == BACK);
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
	int age = GameClock::now() - toastStartMs;
	if (!toast.empty() && age < TOAST_MS) {
		float alpha = age > TOAST_MS - TOAST_FADE_MS
						  ? static_cast<float>(TOAST_MS - age) / static_cast<float>(TOAST_FADE_MS)
						  : 1.f;
		textCentered(body, CENTRE, 12.f, toast.c_str(), {1.f, 0.9f, 0.6f}, alpha);
	}
	textCentered(small, CENTRE, 2.2f, hint, LABEL_DIM);
}

// ---- input -----------------------------------------------------------------

int MainMenu::TargetAt(int x, int y) {
	float cx = 0.f;
	float cy = 0.f;
	ui::toCanvas(visibleArea(), GAME_STATE.render.resX, GAME_STATE.render.resY, x, y, cx, cy);

	if (optionsD) {
		for (int tab = 0; tab < static_cast<int>(TABS.size()); tab++)
			if (tabRect(tab).contains(cx, cy))
				return TAB_BASE + tab;
		return BACK_BUTTON.contains(cx, cy) ? BACK : NONE;
	}
	if (saveD || loadD) {
		for (int slot = 0; slot < SLOT_COUNT; slot++)
			if (slotRect(slot).contains(cx, cy))
				return saveD || slotInfo(slot).used ? slot : NONE;
		return BACK_BUTTON.contains(cx, cy) ? BACK : NONE;
	}
	for (int i = 0; i < BUTTON_COUNT; i++)
		if (buttonRect(i).contains(cx, cy))
			return i;
	return NONE;
}

void MainMenu::Activate(int target) {
	if (target == BACK) {
		ResetSubScreens();
		return;
	}
	if (optionsD) {
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
		GAME_STATE.LoadSave(slotFilename(target).c_str());
		ResetSubScreens();
		show = false;
		inGame = true;
		return;
	}

	switch (menuButtons(inGame)[static_cast<size_t>(target)].action) {
	case MenuAction::NewGame:
		show = false;
		GAME_STATE.NewGame();
		inGame = true;
		break;
	case MenuAction::Resume:
		show = false;
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
		credits = true;
		creditsTimer->Reset();
		break;
	case MenuAction::MainMenu:
		inGame = false;
		break;
	case MenuAction::Quit:
		exit(666);
	}
}

void MainMenu::MouseFunction(int button, int state, int x, int y) {
	(void)button;
	if (credits) { // a click skips the credits
		if (state == GLUT_BUTTON_UP)
			credits = false;
		return;
	}

	int target = TargetAt(x, y);
	hovered = target;
	if (state != GLUT_BUTTON_UP) {
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

void MainMenu::MousePassiveMotion(int x, int y) {
	if (!credits)
		hovered = TargetAt(x, y);
}

void MainMenu::ResetSubScreens() {
	credits = false;
	saveD = false;
	loadD = false;
	optionsD = false;
	hovered = NONE;
	pressed = NONE;
}
