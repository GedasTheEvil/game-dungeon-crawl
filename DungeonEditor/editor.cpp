// Level editor. Runs from DungeonEditor/, levels go to Saved/<name>. See readme.md.

#include "tile_info.h"
#include "../src/graphics/font.h"
#include "../src/graphics/textures.h"
#include "../src/ui/ui_draw.h"
#include "../src/world/level.h"
#include "../src/world/level_check.h"
#include <GL/gl.h>
#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace ui;

namespace {

// Layout works on a 160 x 100 canvas (y up), like the inventory: kept at its aspect ratio, centred on the window.
constexpr float CANVAS_W = 160.f;
constexpr float CANVAS_H = 100.f;
constexpr int START_WIDTH = 1280;
constexpr int START_HEIGHT = 800;

constexpr float CELL = 1.8f; // map cell pitch
constexpr float MAP_X = 6.f;
constexpr float MAP_Y = 8.5f;
constexpr Rect MAP_AREA = {MAP_X, MAP_Y, LEVEL_WIDTH* CELL, LEVEL_HEIGHT* CELL};
constexpr Rect MAP_PANEL = {4.f, 6.5f, 76.f, 88.8f};

constexpr float RIGHT_X = 84.f;
constexpr float RIGHT_W = 72.f;
constexpr float RIGHT_CX = RIGHT_X + RIGHT_W / 2;
constexpr Rect PAINT_BUTTON = {86.f, 82.f, 33.f, 5.5f};
constexpr Rect CHECK_BUTTON = {121.f, 82.f, 33.f, 5.5f};
constexpr Rect TILES_PANEL = {RIGHT_X, 57.5f, RIGHT_W, 22.f};
constexpr Rect BRUSH_PANEL = {RIGHT_X, 47.f, RIGHT_W, 8.5f};
constexpr Rect ATTRIBUTE_BOX = {102.f, 49.f, 14.f, 4.5f};
constexpr Rect VALUE_BOX = {134.f, 49.f, 18.f, 4.5f};
constexpr Rect HINT_PANEL = {RIGHT_X, 16.f, RIGHT_W, 29.f};
constexpr Rect LEVEL_PANEL = {RIGHT_X, 6.5f, RIGHT_W, 7.5f};
constexpr Rect NAME_BOX = {86.5f, 8.f, 44.f, 4.5f};
constexpr Rect SAVE_BUTTON = {132.5f, 8.2f, 10.f, 4.1f};
constexpr Rect LOAD_BUTTON = {143.8f, 8.2f, 10.f, 4.1f};

constexpr int PALETTE_COLUMNS = 7;
constexpr float SLOT_W = 9.f;
constexpr float SLOT_H = 9.2f;
constexpr float SLOT_GAP = 0.8f;
constexpr float NAME_BAND_H = 2.9f;

constexpr size_t ATTRIBUTE_DIGITS = 3;
constexpr size_t VALUE_DIGITS = 4;
constexpr size_t NAME_LENGTH = 32;
constexpr int STATUS_MS = 4000;
constexpr int STATUS_FADE_MS = 800;

constexpr unsigned char KEY_TAB = 9;
constexpr unsigned char KEY_ENTER = 13;
constexpr unsigned char KEY_ESCAPE = 27;
constexpr unsigned char KEY_BACKSPACE = 8;
constexpr unsigned char KEY_DELETE = 127;
constexpr unsigned char KEY_CTRL_O = 15;
constexpr unsigned char KEY_CTRL_S = 19;

constexpr Color WALL_TOP = {0.40f, 0.31f, 0.21f};
constexpr Color WALL_BOTTOM = {0.30f, 0.23f, 0.15f};
constexpr Color OPEN_COLOR = {0.035f, 0.028f, 0.02f};

enum class Mode : unsigned char { Paint, Check };
enum class Field : unsigned char { None, Attribute, Value, Name };
// Clickable things besides the map.
enum class Target : unsigned char { None, Tile, PaintMode, CheckMode, Attribute, Value, Name, Save, Load };

Rect slotRect(int tile) {
	float x0 = RIGHT_CX - (PALETTE_COLUMNS * SLOT_W + (PALETTE_COLUMNS - 1) * SLOT_GAP) / 2;
	int column = tile % PALETTE_COLUMNS;
	int row = tile / PALETTE_COLUMNS;
	return {x0 + static_cast<float>(column) * (SLOT_W + SLOT_GAP),
			TILES_PANEL.y + TILES_PANEL.h - 2.f - SLOT_H - static_cast<float>(row) * (SLOT_H + SLOT_GAP), SLOT_W,
			SLOT_H};
}

Rect cellRect(int col, int row) {
	return {MAP_X + static_cast<float>(col) * CELL, MAP_Y + static_cast<float>(row) * CELL, CELL, CELL};
}

int parseNumber(const std::string& digits) { return digits.empty() ? 0 : std::stoi(digits); }

bool isNameChar(unsigned char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' ||
		   c == '.';
}

// Splits `str` into lines no wider than `width`, at spaces.
std::vector<std::string> wrap(const Font& font, const std::string& str, float width) {
	std::vector<std::string> lines;
	std::string current;
	size_t pos = 0;
	while (pos < str.size()) {
		size_t end = str.find(' ', pos);
		if (end == std::string::npos)
			end = str.size();
		std::string word = str.substr(pos, end - pos);
		std::string candidate = current;
		if (!candidate.empty())
			candidate += ' ';
		candidate += word;
		if (!current.empty() && font.TextWidth(candidate.c_str()) > width) {
			lines.push_back(current);
			current = word;
		} else {
			current = candidate;
		}
		pos = end + 1;
	}
	if (!current.empty())
		lines.push_back(current);
	return lines;
}

class Editor {
  public:
	Editor();
	void draw();
	void resize(int width, int height);
	void mouseButton(int button, int state, int x, int y);
	void mouseDrag(int x, int y);
	void mouseMove(int x, int y);
	void keyPressed(unsigned char key);

  private:
	LevelGrid grid; // zero-initialised: all Wall, the rock the level is carved from
	int tile = Empty;
	std::string attributeText, valueText, name;
	Mode mode = Mode::Paint;
	Field field = Field::None;

	CellPos hovered;
	CellPos checked;
	bool painting = false;
	Target hoveredTarget = Target::None;
	int hoveredTile = -1;

	LevelReport report;
	bool reportStale = true;

	std::string status;
	bool statusError = false;
	int statusStartMs = -STATUS_MS;

	int winW = START_WIDTH;
	int winH = START_HEIGHT;

	Textura icons[TILE_COUNT];
	bool hasIcon[TILE_COUNT] = {};
	Textura wallTexture, papyrus;
	Font title, heading, body, small;

	[[nodiscard]] Tint brush() const { return {tile, parseNumber(attributeText), parseNumber(valueText)}; }
	[[nodiscard]] Rect visibleArea() const;
	void toCanvas(int mouseX, int mouseY, float& x, float& y) const;
	[[nodiscard]] static CellPos cellAt(float x, float y);
	void updateHover(float x, float y);

	void setMode(Mode m);
	void paint(CellPos cell);
	void pick(CellPos cell);
	void save();
	void load();
	void showStatus(const std::string& text, bool error);
	void typeInto(std::string& text, unsigned char key, size_t maxLength, bool digitsOnly);

	void drawBackground();
	void drawMap();
	void drawMapOverlay();
	void drawPalette();
	void drawBrush();
	void drawHint();
	void drawFieldHint(float& y, const char* name, const FieldHint& hint);
	void drawLevelPanel();
	void drawFooter();
	void drawButton(const Rect& r, const char* label, bool active, Target target);
	void drawTextBox(const Rect& r, const std::string& value, Field which, Target target);
	void drawTileSwatch(int type, const Rect& r, float alpha);
};

Editor::Editor() {
	title.Load("../Fonts/papyrus.png", 6.f, 0.26f, true);
	heading.Load("../Fonts/papyrus.png", 3.6f, 0.12f, true);
	body.Load("../Fonts/papyrus.png", 2.7f, 0.08f, true);
	small.Load("../Fonts/papyrus.png", 2.3f, 0.06f, true);
	wallTexture.LoadPNG("../Textures/ui/scarab_slate.png");
	papyrus.LoadPNG("../Textures/ui/papyrus_sheet.png");
	for (int type = 0; type < TILE_COUNT; type++) {
		const char* icon = tileInfo(type).icon;
		if (icon == nullptr)
			continue;
		std::string path = std::string("Textures/") + icon;
		hasIcon[type] = icons[type].LoadPNG(path.c_str()) != 0;
		if (!hasIcon[type])
			fprintf(stderr, "Cannot load %s\n", path.c_str());
	}
}

// ---- input -----------------------------------------------------------------

Rect Editor::visibleArea() const {
	float aspect = static_cast<float>(winW) / static_cast<float>(winH);
	if (aspect >= CANVAS_W / CANVAS_H) {
		float w = CANVAS_H * aspect;
		return {(CANVAS_W - w) / 2, 0, w, CANVAS_H};
	}
	float h = CANVAS_W / aspect;
	return {0, (CANVAS_H - h) / 2, CANVAS_W, h};
}

void Editor::toCanvas(int mouseX, int mouseY, float& x, float& y) const {
	Rect area = visibleArea();
	x = area.x + area.w * static_cast<float>(mouseX) / static_cast<float>(winW);
	y = area.y + area.h - area.h * static_cast<float>(mouseY) / static_cast<float>(winH);
}

CellPos Editor::cellAt(float x, float y) {
	if (!MAP_AREA.contains(x, y))
		return {};
	CellPos cell = {static_cast<int>((x - MAP_X) / CELL), static_cast<int>((y - MAP_Y) / CELL)};
	return LevelGrid::inBounds(cell.col, cell.row) ? cell : CellPos{};
}

void Editor::updateHover(float x, float y) {
	hovered = cellAt(x, y);
	hoveredTile = -1;
	hoveredTarget = Target::None;
	for (int type = 0; type < TILE_COUNT; type++)
		if (slotRect(type).contains(x, y)) {
			hoveredTile = type;
			hoveredTarget = Target::Tile;
		}
	const std::pair<Rect, Target> TARGETS[] = {
		{PAINT_BUTTON, Target::PaintMode}, {CHECK_BUTTON, Target::CheckMode}, {ATTRIBUTE_BOX, Target::Attribute},
		{VALUE_BOX, Target::Value},		   {NAME_BOX, Target::Name},		  {SAVE_BUTTON, Target::Save},
		{LOAD_BUTTON, Target::Load},
	};
	for (const auto& [rect, target] : TARGETS)
		if (rect.contains(x, y))
			hoveredTarget = target;
}

void Editor::mouseButton(int button, int state, int x, int y) {
	float cx = 0.f;
	float cy = 0.f;
	toCanvas(x, y, cx, cy);
	updateHover(cx, cy);

	if (state == GLUT_UP) {
		painting = false;
		return;
	}

	if (hovered.col >= 0) {
		field = Field::None;
		if (button == GLUT_RIGHT_BUTTON) {
			pick(hovered);
		} else if (button == GLUT_LEFT_BUTTON && mode == Mode::Check) {
			checked = hovered;
		} else if (button == GLUT_LEFT_BUTTON) {
			painting = true;
			paint(hovered);
		}
		return;
	}

	if (button != GLUT_LEFT_BUTTON)
		return;
	field = Field::None;
	switch (hoveredTarget) {
	case Target::Tile:
		tile = hoveredTile;
		break;
	case Target::PaintMode:
		setMode(Mode::Paint);
		break;
	case Target::CheckMode:
		setMode(Mode::Check);
		break;
	case Target::Attribute:
		field = Field::Attribute;
		break;
	case Target::Value:
		field = Field::Value;
		break;
	case Target::Name:
		field = Field::Name;
		break;
	case Target::Save:
		save();
		break;
	case Target::Load:
		load();
		break;
	case Target::None:
		break;
	}
}

void Editor::mouseDrag(int x, int y) {
	float cx = 0.f;
	float cy = 0.f;
	toCanvas(x, y, cx, cy);
	updateHover(cx, cy);
	if (painting && hovered.col >= 0)
		paint(hovered);
}

void Editor::mouseMove(int x, int y) {
	float cx = 0.f;
	float cy = 0.f;
	toCanvas(x, y, cx, cy);
	updateHover(cx, cy);
}

void Editor::typeInto(std::string& text, unsigned char key, size_t maxLength, bool digitsOnly) {
	if (key == KEY_BACKSPACE || key == KEY_DELETE) {
		if (!text.empty())
			text.pop_back();
		return;
	}
	bool allowed = digitsOnly ? key >= '0' && key <= '9' : isNameChar(key);
	if (allowed && text.size() < maxLength)
		text += static_cast<char>(key);
}

void Editor::keyPressed(unsigned char key) {
	if (key == KEY_CTRL_S) {
		save();
		return;
	}
	if (key == KEY_CTRL_O) {
		load();
		return;
	}

	if (field == Field::None) {
		if (key == 'p' || key == 'P')
			setMode(Mode::Paint);
		else if (key == 'c' || key == 'C')
			setMode(Mode::Check);
		else if (key == KEY_TAB)
			field = Field::Attribute;
		return;
	}

	if (key == KEY_ENTER || key == KEY_ESCAPE) {
		field = Field::None;
		return;
	}
	if (key == KEY_TAB) {
		field = field == Field::Attribute ? Field::Value : (field == Field::Value ? Field::Name : Field::None);
		return;
	}
	if (field == Field::Attribute)
		typeInto(attributeText, key, ATTRIBUTE_DIGITS, true);
	else if (field == Field::Value)
		typeInto(valueText, key, VALUE_DIGITS, true);
	else
		typeInto(name, key, NAME_LENGTH, false);
}

// ---- actions ---------------------------------------------------------------

void Editor::setMode(Mode m) {
	mode = m;
	painting = false;
}

void Editor::paint(CellPos cell) {
	grid.set(cell.col, cell.row, brush());
	reportStale = true;
}

// Takes the cell's tile, attribute and value into the brush.
void Editor::pick(CellPos cell) {
	Tint t = grid.at(cell.col, cell.row);
	if (isTileType(t.a))
		tile = t.a;
	attributeText = t.b != 0 ? std::to_string(t.b) : "";
	valueText = t.c != 0 ? std::to_string(t.c) : "";
	showStatus("Picked " + std::string(tileInfo(t.a).name) + " " + std::to_string(t.b) + " " + std::to_string(t.c),
			   false);
}

void Editor::save() {
	if (name.empty()) {
		showStatus("Enter a level name first", true);
		return;
	}
	std::string path = "Saved/" + name;
	if (saveLevelFile(path.c_str(), grid))
		showStatus("Saved to " + path, false);
	else
		showStatus("Cannot write " + path, true);
}

void Editor::load() {
	if (name.empty()) {
		showStatus("Enter a level name first", true);
		return;
	}
	std::string path = "Saved/" + name;
	auto loaded = std::make_unique<LevelGrid>();
	std::string error = loadLevelFile(path.c_str(), *loaded);
	if (!error.empty()) {
		showStatus(path + ": " + error, true);
		return;
	}
	grid = *loaded;
	reportStale = true;
	checked = {};
	showStatus("Loaded from " + path, false);
}

void Editor::showStatus(const std::string& text, bool error) {
	status = text;
	statusError = error;
	statusStartMs = glutGet(GLUT_ELAPSED_TIME);
}

// ---- drawing ---------------------------------------------------------------

void Editor::resize(int width, int height) {
	winW = width > 0 ? width : 1;
	winH = height > 0 ? height : 1;
	glViewport(0, 0, winW, winH);
}

void Editor::draw() {
	if (mode == Mode::Check && reportStale) {
		report = checkLevel(grid);
		reportStale = false;
	}

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	Rect area = visibleArea();
	glOrtho(area.x, area.x + area.w, area.y, area.y + area.h, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	drawBackground();
	drawMap();
	drawMapOverlay();
	drawPalette();
	drawBrush();
	drawHint();
	drawLevelPanel();
	drawFooter();

	glutSwapBuffers();
}

void Editor::drawBackground() {
	Rect area = visibleArea();
	glDisable(GL_BLEND);
	texturedRect(area, wallTexture.ID(), {0.34f, 0.27f, 0.20f});

	beginShapes();
	constexpr float VIGNETTE = 22.f;
	ring(area.inset(VIGNETTE), VIGNETTE, BLACK, 0.f, 0.85f);

	// Title ornament: gold rules ending in diamonds either side of the title.
	constexpr float RULE_Y = 93.2f;
	float titleHalf = title.TextWidth("Dungeon Editor") / 2 + 2;
	line(RIGHT_X + 2, RULE_Y, RIGHT_CX - titleHalf, RULE_Y, GOLD_DIM, 1.f, 2.f);
	line(RIGHT_CX + titleHalf, RULE_Y, RIGHT_X + RIGHT_W - 2, RULE_Y, GOLD_DIM, 1.f, 2.f);
	diamond(RIGHT_X + 2, RULE_Y, 1.1f, GOLD, 1.f);
	diamond(RIGHT_X + RIGHT_W - 2, RULE_Y, 1.1f, GOLD, 1.f);

	panel(MAP_PANEL, 0.9f);
	panel(TILES_PANEL, 0.9f);
	panel(BRUSH_PANEL, 0.9f);
	panel(LEVEL_PANEL, 0.9f);

	// Papyrus scroll for the hints, as the inventory's detail panel.
	texturedRect(HINT_PANEL, papyrus.ID(), {1, 1, 1}, 0.04f, 0.07f, 0.96f, 0.93f);
	beginShapes();
	strokeRect(HINT_PANEL, BRONZE, 1.f, 3.f);
	cornerStuds(HINT_PANEL);

	beginText();
	textCentered(title, RIGHT_CX, 91.f, "Dungeon Editor", GOLD);
	beginShapes();
}

void Editor::drawTileSwatch(int type, const Rect& r, float alpha) {
	if (type == Wall) {
		fillRect(r, WALL_TOP, WALL_BOTTOM, alpha);
		return;
	}
	fillRect(r, OPEN_COLOR, OPEN_COLOR, alpha);
	if (isTileType(type) && hasIcon[type]) {
		texturedRect(r, icons[type].ID(), {alpha, alpha, alpha});
		beginShapes();
	}
}

void Editor::drawMap() {
	beginShapes();
	fillRect(MAP_AREA.inset(-0.4f), BLACK, BLACK, 1.f);
	for (int row = 0; row < LEVEL_HEIGHT; row++)
		for (int col = 0; col < LEVEL_WIDTH; col++)
			drawTileSwatch(grid.at(col, row).a, cellRect(col, row), 1.f);

	// Grid lines, every 5th a little stronger to help counting.
	for (int col = 0; col <= LEVEL_WIDTH; col++) {
		float x = MAP_X + static_cast<float>(col) * CELL;
		line(x, MAP_Y, x, MAP_AREA.y + MAP_AREA.h, BLACK, col % 5 == 0 ? 0.8f : 0.45f, 1.f);
	}
	for (int row = 0; row <= LEVEL_HEIGHT; row++) {
		float y = MAP_Y + static_cast<float>(row) * CELL;
		line(MAP_X, y, MAP_AREA.x + MAP_AREA.w, y, BLACK, row % 5 == 0 ? 0.8f : 0.45f, 1.f);
	}
}

void Editor::drawMapOverlay() {
	beginShapes();
	float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.005f);

	if (mode == Mode::Check) {
		// Cheapest path from the entrance to the goal.
		for (const CellPos& p : report.path) {
			Rect r = cellRect(p.col, p.row);
			diamond(r.cx(), r.cy(), 0.5f, GOLD_BRIGHT, 0.9f);
		}
		if (checked.col >= 0) {
			Rect r = cellRect(checked.col, checked.row);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			ring(r, 1.2f, GOLD, 0.5f + 0.4f * pulse, 0.f);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			strokeRect(r, GOLD, 1.f, 2.f);
		}
	}

	if (hovered.col >= 0) {
		Rect r = cellRect(hovered.col, hovered.row);
		if (mode == Mode::Paint)
			drawTileSwatch(tile, r, 0.55f + 0.3f * pulse); // preview of the brush
		strokeRect(r.inset(-0.15f), GOLD_BRIGHT, 1.f, 1.5f);
	}

	// Cursor readout above the map.
	beginText();
	char buf[96];
	if (hovered.col >= 0) {
		Tint t = grid.at(hovered.col, hovered.row);
		snprintf(buf, sizeof(buf), "Column %d, row %d:  %s  %d  %d", hovered.col, hovered.row, tileInfo(t.a).name, t.b,
				 t.c);
		text(small, MAP_PANEL.x + 1.f, 96.6f, buf, GOLD);
	}
	const char* modeText = mode == Mode::Paint ? "Paint: click to draw" : "Check: click to inspect";
	text(small, MAP_PANEL.x + MAP_PANEL.w - small.TextWidth(modeText) - 1.f, 96.6f, modeText, GOLD_DIM);
	beginShapes();
}

void Editor::drawButton(const Rect& area, const char* label, bool active, Target target) {
	bool hovered = hoveredTarget == target;
	Rect r = area;
	fillRect({r.x + 0.5f, r.y - 0.6f, r.w, r.h}, BLACK, BLACK, 0.35f);
	if (hovered) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		ring(r, 1.6f, GOLD, 0.35f, 0.f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	if (active) {
		fillRect(r, hovered ? Color{0.20f, 0.40f, 0.78f} : LAPIS, hovered ? Color{0.09f, 0.20f, 0.46f} : LAPIS_DARK,
				 1.f);
		fillRect({r.x, r.y + r.h - 0.8f, r.w, 0.8f}, {1, 1, 1}, {1, 1, 1}, 0.12f);
		strokeRect(r, hovered ? GOLD_BRIGHT : GOLD, 1.f, 2.5f);
		strokeRect(r.inset(0.7f), GOLD_DIM, 0.6f, 1.f);
	} else {
		fillRect(r, hovered ? Color{0.33f, 0.25f, 0.15f} : STONE_TOP,
				 hovered ? Color{0.17f, 0.13f, 0.08f} : STONE_BOTTOM, 1.f);
		strokeRect(r, hovered ? GOLD_BRIGHT : BRONZE, 1.f, 1.5f);
	}
	beginText();
	Font& font = r.h > 5.f ? heading : body;
	float textY = r.y + (r.h - (r.h > 5.f ? 3.6f : 2.7f)) / 2 + 0.2f;
	textCentered(font, r.cx(), textY, label, active ? (hovered ? GOLD_BRIGHT : GOLD) : GOLD_DIM);
	beginShapes();
}

void Editor::drawTextBox(const Rect& r, const std::string& value, Field which, Target target) {
	bool active = field == which;
	bool hovered = hoveredTarget == target;
	fillRect(r, {0.03f, 0.025f, 0.02f}, {0.06f, 0.045f, 0.03f}, 1.f);
	if (active) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		ring(r, 1.2f, GOLD, 0.4f, 0.f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	strokeRect(r, active ? GOLD : (hovered ? GOLD_BRIGHT : BRONZE), 1.f, active ? 2.f : 1.5f);

	beginText();
	// Empty fields show a placeholder: 0 for the numbers, what to type for the name.
	const char* placeholder = which == Field::Name ? "Level name" : "0";
	bool empty = value.empty() && !active;
	std::string shown = empty ? placeholder : value;
	float x = r.x + 1.f;
	float y = r.y + (r.h - 2.7f) / 2 + 0.2f;
	text(body, x, y, shown.c_str(), empty ? GOLD_DIM : GOLD, empty && which == Field::Name ? 0.6f : 1.f);
	beginShapes();
	if (active && (glutGet(GLUT_ELAPSED_TIME) / 500) % 2 == 0) { // blinking caret
		float cx = x + body.TextWidth(shown.c_str()) + 0.3f;
		line(cx, r.y + 0.8f, cx, r.y + r.h - 0.8f, GOLD, 1.f, 1.5f);
	}
}

void Editor::drawPalette() {
	float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.004f);
	for (int type = 0; type < TILE_COUNT; type++) {
		Rect r = slotRect(type);
		bool selected = type == tile;
		bool hover = type == hoveredTile;
		if (selected) {
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			ring(r, 1.8f, GOLD, 0.35f + 0.25f * pulse, 0.f);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		Color top = hover || selected ? Color{0.33f, 0.25f, 0.15f} : STONE_TOP;
		Color bottom = hover || selected ? Color{0.17f, 0.13f, 0.08f} : STONE_BOTTOM;
		fillRect(r, top, bottom, 1.f);

		Rect band = {r.x, r.y, r.w, NAME_BAND_H};
		fillRect(band, {0.09f, 0.07f, 0.05f}, {0.06f, 0.045f, 0.03f}, 1.f);
		line(r.x, r.y + NAME_BAND_H, r.x + r.w, r.y + NAME_BAND_H, selected ? GOLD : BRONZE, 1.f, 1.f);

		constexpr float ICON = 5.2f;
		Rect icon = {r.cx() - ICON / 2, r.y + NAME_BAND_H + (r.h - NAME_BAND_H - ICON) / 2, ICON, ICON};
		drawTileSwatch(type, icon, selected || hover ? 1.f : 0.8f);
		strokeRect(icon, BLACK, 0.6f, 1.f);

		if (selected)
			strokeRect(r, GOLD, 1.f, 2.5f);
		else
			strokeRect(r, hover ? GOLD_BRIGHT : BRONZE, 1.f, hover ? 2.f : 1.5f);

		beginText();
		textCentered(small, r.cx(), r.y + 0.45f, tileInfo(type).name,
					 selected || hover ? GOLD : Color{0.78f, 0.64f, 0.40f});
		beginShapes();
	}

	drawButton(PAINT_BUTTON, "Paint", mode == Mode::Paint, Target::PaintMode);
	drawButton(CHECK_BUTTON, "Check", mode == Mode::Check, Target::CheckMode);
}

void Editor::drawBrush() {
	drawTextBox(ATTRIBUTE_BOX, attributeText, Field::Attribute, Target::Attribute);
	drawTextBox(VALUE_BOX, valueText, Field::Value, Target::Value);
	beginText();
	float y = ATTRIBUTE_BOX.y + 1.1f;
	text(body, RIGHT_X + 3.f, y, "Attribute", GOLD_DIM);
	text(body, VALUE_BOX.x - body.TextWidth("Value") - 1.5f, y, "Value", GOLD_DIM);
	beginShapes();
}

void Editor::drawFieldHint(float& y, const char* name, const FieldHint& hint) {
	constexpr float LINE = 2.8f;
	float x = HINT_PANEL.x + 4.f;
	float width = HINT_PANEL.w - 8.f;

	std::string label = std::string(name) + ": " + hint.label;
	text(body, x, y, label.c_str(), INK_FADED);
	if (!hint.current.empty()) {
		float valueX = x + body.TextWidth(label.c_str()) + 1.5f;
		std::vector<std::string> lines = wrap(body, hint.current, x + width - valueX);
		for (size_t i = 0; i < lines.size(); i++) {
			text(body, i == 0 ? valueX : x + 3.f, y, lines[i].c_str(), hint.valid ? INK : INK_RED);
			if (i + 1 < lines.size())
				y -= LINE;
		}
	}
	y -= LINE;
	if (!hint.choices.empty())
		for (const std::string& l : wrap(small, hint.choices, width - 3.f)) {
			text(small, x + 3.f, y + 0.2f, l.c_str(), INK_FADED);
			y -= LINE - 0.4f;
		}
	y -= 0.6f;
}

void Editor::drawHint() {
	bool checking = mode == Mode::Check && checked.col >= 0;
	Tint cell = checking ? grid.at(checked.col, checked.row) : brush();
	CellHint hint = describeCell(cell);
	float cx = HINT_PANEL.cx();
	float x = HINT_PANEL.x + 4.f;
	float width = HINT_PANEL.w - 8.f;

	beginText();
	textCentered(heading, cx, HINT_PANEL.y + HINT_PANEL.h - 5.2f, hint.title.c_str(), INK);
	char sub[64];
	if (checking)
		snprintf(sub, sizeof(sub), "Cell at column %d, row %d", checked.col, checked.row);
	else
		snprintf(sub, sizeof(sub), "%s", mode == Mode::Check ? "Brush. Click a cell to inspect it" : "Brush");
	textCentered(small, cx, HINT_PANEL.y + HINT_PANEL.h - 8.2f, sub, INK_RED);

	float y = HINT_PANEL.y + HINT_PANEL.h - 12.6f;
	for (const std::string& l : wrap(body, hint.description, width)) {
		text(body, x, y, l.c_str(), INK);
		y -= 2.8f;
	}
	y -= 0.8f;
	drawFieldHint(y, "Attribute", hint.attribute);
	drawFieldHint(y, "Value", hint.value);

	beginShapes();
	float ruleY = HINT_PANEL.y + HINT_PANEL.h - 9.2f;
	line(HINT_PANEL.x + 8, ruleY, HINT_PANEL.x + HINT_PANEL.w - 8, ruleY, INK_FADED, 0.8f, 1.f);
	diamond(cx, ruleY, 0.6f, INK_RED, 1.f);
}

void Editor::drawLevelPanel() {
	drawTextBox(NAME_BOX, name, Field::Name, Target::Name);
	drawButton(SAVE_BUTTON, "Save", true, Target::Save);
	drawButton(LOAD_BUTTON, "Load", true, Target::Load);
}

void Editor::drawFooter() {
	beginText();
	int age = glutGet(GLUT_ELAPSED_TIME) - statusStartMs;
	if (!status.empty() && age < STATUS_MS) {
		float alpha = age > STATUS_MS - STATUS_FADE_MS
						  ? static_cast<float>(STATUS_MS - age) / static_cast<float>(STATUS_FADE_MS)
						  : 1.f;
		textCentered(body, RIGHT_CX, 3.5f, status.c_str(), statusError ? Color{1.f, 0.45f, 0.3f} : GOLD_BRIGHT, alpha);
	}

	// Level check summary, check mode only.
	if (mode == Mode::Check) {
		char buf[128];
		if (report.valid)
			snprintf(buf, sizeof(buf), "Level OK: path %d moves, difficulty %.1f%s", report.pathLength,
					 static_cast<double>(report.difficulty),
					 report.warnings.empty() ? "" : (", " + report.warnings.front()).c_str());
		else
			snprintf(buf, sizeof(buf), "Level not finishable: %s",
					 report.errors.empty() ? "unknown" : report.errors.front().c_str());
		textCentered(body, MAP_PANEL.cx(), 3.5f, buf,
					 report.valid ? Color{0.55f, 0.85f, 0.4f} : Color{1.f, 0.45f, 0.3f});
	}

	textCentered(
		small, CANVAS_W / 2, 0.8f,
		"Left click: paint / inspect    Right click: pick cell into brush    P / C: mode    Tab: next field    "
		"Ctrl+S / Ctrl+O: save / load",
		{0.55f, 0.45f, 0.30f});
	beginShapes();
}

std::unique_ptr<Editor> editor;

} // namespace

int main(int argc, char* argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(START_WIDTH, START_HEIGHT);
	glutCreateWindow("Dungeon Editor");

	editor = std::make_unique<Editor>();
	glutDisplayFunc([] { editor->draw(); });
	glutIdleFunc([] { glutPostRedisplay(); });
	glutReshapeFunc([](int w, int h) { editor->resize(w, h); });
	glutKeyboardFunc([](unsigned char key, int, int) { editor->keyPressed(key); });
	glutMouseFunc([](int button, int state, int x, int y) { editor->mouseButton(button, state, x, y); });
	glutMotionFunc([](int x, int y) { editor->mouseDrag(x, y); });
	glutPassiveMotionFunc([](int x, int y) { editor->mouseMove(x, y); });

	glutMainLoop();
	return 0;
}
