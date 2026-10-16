// Level editor. Runs from the repo root, levels go to tools/editor/saved/<name>. See readme.md.

#include "map_view.h"
#include "tile_info.h"
#include "../../src/graphics/font.h"
#include "../../src/graphics/textures.h"
#include "../../src/ui/ui_draw.h"
#include "../../src/world/level.h"
#include "../../src/world/level_check.h"
#include <GL/gl.h>
#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include "../../src/core/paths.h"

using namespace ui;

namespace {

// Layout works on a 160 x 100 canvas (y up), like the inventory: kept at its aspect ratio, centred on the window.
constexpr float CANVAS_W = 160.f;
constexpr float CANVAS_H = 100.f;
constexpr int START_WIDTH = 1280;
constexpr int START_HEIGHT = 800;

constexpr Rect MAP_AREA = EDITOR_MAP_AREA; // the window on the map; at 1x the whole map, 1.8 per cell
constexpr float MINIMAP_CELL = 0.42f; // the minimap (zoomed only): the whole map, in a corner of the map area
constexpr float MINIMAP_MARGIN = 0.8f;
constexpr Rect MINIMAP = {MAP_AREA.x + MAP_AREA.w - MINIMAP_MARGIN - LEVEL_WIDTH * MINIMAP_CELL,
						  MAP_AREA.y + MAP_AREA.h - MINIMAP_MARGIN - LEVEL_HEIGHT* MINIMAP_CELL,
						  LEVEL_WIDTH* MINIMAP_CELL, LEVEL_HEIGHT* MINIMAP_CELL};
constexpr float WHEEL_PAN_CELLS = 2.f; // a wheel notch or an arrow key moves the window this far
constexpr Rect MAP_PANEL = {4.f, 6.5f, 76.f, 88.8f};

constexpr float RIGHT_X = 84.f;
constexpr float RIGHT_W = 72.f;
constexpr float RIGHT_CX = RIGHT_X + RIGHT_W / 2;
constexpr Rect PAINT_BUTTON = {86.f, 82.f, 12.f, 5.5f};
constexpr Rect CHECK_BUTTON = {98.8f, 82.f, 12.f, 5.5f};
constexpr Rect STRUCTURE_BUTTON = {112.4f, 82.f, 16.f, 5.5f};
constexpr Rect WALL_BUTTON = {129.2f, 82.f, 11.f, 5.5f};
constexpr Rect OBJECTS_BUTTON = {141.f, 82.f, 14.f, 5.5f};
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
constexpr Color WATER_TOP = {0.12f, 0.30f, 0.42f};
constexpr Color WATER_BOTTOM = {0.07f, 0.18f, 0.28f};
constexpr Color DEEP_TOP = {0.05f, 0.12f, 0.24f};
constexpr Color DEEP_BOTTOM = {0.03f, 0.07f, 0.15f};
constexpr float OTHER_LAYER_ALPHA = 0.4f; // the layers not being painted, dimmed
constexpr float HALF_WATER_OVER_OBJECT = 0.5f; // the water over the lower half of an object in it

enum class Mode : unsigned char { Paint, Check };
// Which one the brush paints and the palette shows: the structure, the fixtures on the back wall (plan wall-layer) or
// the objects.
enum class Layer : unsigned char { Structure, Wall, Objects };
enum class Field : unsigned char { None, Attribute, Value, Name };
// Clickable things besides the map.
enum class Target : unsigned char {
	None,
	Tile,
	PaintMode,
	CheckMode,
	StructureLayer,
	WallLayer,
	ObjectsLayer,
	Attribute,
	Value,
	Name,
	Save,
	Load
};

// The palette of a layer: structure numbers or tile types (the wall types in the wall layer, the others in the object
// layer, None in both), in slot order.
std::vector<int> paletteOf(Layer layer) {
	std::vector<int> ids;
	if (layer == Layer::Structure) {
		for (int s = 0; s < STRUCTURE_COUNT; s++)
			ids.push_back(s);
		return ids;
	}
	for (int type = 0; type < TILE_TYPE_COUNT; type++)
		if (isTileType(type) && (type == NoObject || isWallType(type) == (layer == Layer::Wall)))
			ids.push_back(type);
	return ids;
}

Rect slotRect(int slot) {
	float x0 = RIGHT_CX - (PALETTE_COLUMNS * SLOT_W + (PALETTE_COLUMNS - 1) * SLOT_GAP) / 2;
	int column = slot % PALETTE_COLUMNS;
	int row = slot / PALETTE_COLUMNS;
	return {x0 + static_cast<float>(column) * (SLOT_W + SLOT_GAP),
			TILES_PANEL.y + TILES_PANEL.h - 2.f - SLOT_H - static_cast<float>(row) * (SLOT_H + SLOT_GAP), SLOT_W,
			SLOT_H};
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
	void specialKeyPressed(int key);

  private:
	LevelGrid grid; // all wall, the rock the level is carved from
	int tile = NoObject;
	Structure structure = Structure::Empty;
	std::string attributeText, valueText, name;
	Mode mode = Mode::Paint;
	Layer layer = Layer::Objects;
	Field field = Field::None;

	CellPos hovered;
	CellPos checked;
	bool painting = false;
	MapView view; // zoom and pan (plan editor-zoom)
	float pointerX = 0.f; // the mouse, canvas
	float pointerY = 0.f;
	bool panning = false; // middle button drag
	bool draggingMinimap = false;
	Target hoveredTarget = Target::None;
	int hoveredSlot = -1;

	LevelReport report;
	bool reportStale = true;

	std::string status;
	bool statusError = false;
	int statusStartMs = -STATUS_MS;

	int winW = START_WIDTH;
	int winH = START_HEIGHT;

	Texture icons[TILE_TYPE_COUNT];
	bool hasIcon[TILE_TYPE_COUNT] = {};
	Texture wallTexture, papyrus;
	Font title, heading, body, small;

	[[nodiscard]] Tile brush() const { return {tile, parseNumber(attributeText), parseNumber(valueText)}; }
	// The palette's id in the current layer: a tile type or a structure number.
	[[nodiscard]] int selectedId() const { return layer == Layer::Structure ? static_cast<int>(structure) : tile; }
	[[nodiscard]] Rect visibleArea() const;
	void toCanvas(int mouseX, int mouseY, float& x, float& y) const;
	[[nodiscard]] bool minimapShown() const { return view.zoom() > 1; }
	[[nodiscard]] Rect cellRect(int col, int row) const { return view.cellRect(col, row); }
	void updateHover(float x, float y);
	void track(int mouseX, int mouseY);
	void wheel(int button);
	void zoomTo(int zoom);
	void centerOnMinimap(float x, float y);
	void beginMapClip() const;
	static void endMapClip();
	void drawMinimap();

	void setMode(Mode m);
	void setLayer(Layer l);
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
	void drawStructureSwatch(Structure s, const Rect& r, float alpha);
	void drawObjectIcon(int type, const Rect& r, float alpha);
	void drawCell(const Tile& t, const Rect& r, float structureAlpha, float wallAlpha, float objectAlpha);
};

Editor::Editor() {
	title.Load(MEDIA_DIR "textures/fonts/papyrus.png", 6.f, 0.26f, true);
	heading.Load(MEDIA_DIR "textures/fonts/papyrus.png", 3.6f, 0.12f, true);
	body.Load(MEDIA_DIR "textures/fonts/papyrus.png", 2.7f, 0.08f, true);
	small.Load(MEDIA_DIR "textures/fonts/papyrus.png", 2.3f, 0.06f, true);
	wallTexture.LoadPNG(MEDIA_DIR "textures/ui/scarab_slate.png", TexFilter::Flat);
	papyrus.LoadPNG(MEDIA_DIR "textures/ui/papyrus_sheet.png", TexFilter::Flat);
	for (int type = 0; type < TILE_TYPE_COUNT; type++) {
		const char* icon = tileInfo(type).icon;
		if (icon == nullptr)
			continue;
		std::string path = std::string("tools/editor/icons/") + icon;
		hasIcon[type] = icons[type].LoadPNG(path.c_str(), TexFilter::Flat) != 0;
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

// Remembers the mouse and what is under it.
void Editor::track(int mouseX, int mouseY) {
	toCanvas(mouseX, mouseY, pointerX, pointerY);
	updateHover(pointerX, pointerY);
}

void Editor::updateHover(float x, float y) {
	hovered = minimapShown() && MINIMAP.contains(x, y) ? CellPos{} : view.cellAt(x, y);
	hoveredSlot = -1;
	hoveredTarget = Target::None;
	const size_t slots = paletteOf(layer).size();
	for (size_t slot = 0; slot < slots; slot++)
		if (slotRect(static_cast<int>(slot)).contains(x, y)) {
			hoveredSlot = static_cast<int>(slot);
			hoveredTarget = Target::Tile;
		}
	static const std::pair<Rect, Target> TARGETS[] = {
		{PAINT_BUTTON, Target::PaintMode},
		{CHECK_BUTTON, Target::CheckMode},
		{STRUCTURE_BUTTON, Target::StructureLayer},
		{WALL_BUTTON, Target::WallLayer},
		{OBJECTS_BUTTON, Target::ObjectsLayer},
		{ATTRIBUTE_BOX, Target::Attribute},
		{VALUE_BOX, Target::Value},
		{NAME_BOX, Target::Name},
		{SAVE_BUTTON, Target::Save},
		{LOAD_BUTTON, Target::Load},
	};
	for (const auto& [rect, target] : TARGETS)
		if (rect.contains(x, y))
			hoveredTarget = target;
}

// Zooms to `zoom` around the cell under the mouse (the centre of the window when the mouse is not on the map).
void Editor::zoomTo(int zoom) {
	float fracX = 0.5f;
	float fracY = 0.5f;
	if (MAP_AREA.contains(pointerX, pointerY)) {
		fracX = (pointerX - MAP_AREA.x) / MAP_AREA.w;
		fracY = (pointerY - MAP_AREA.y) / MAP_AREA.h;
	}
	view.setZoom(zoom, view.originCol() + fracX * view.visibleCols(), view.originRow() + fracY * view.visibleRows(),
				 fracX, fracY);
	updateHover(pointerX, pointerY);
}

void Editor::centerOnMinimap(float x, float y) {
	view.centerOn((x - MINIMAP.x) / MINIMAP_CELL, (y - MINIMAP.y) / MINIMAP_CELL);
	updateHover(pointerX, pointerY);
}

// The mouse wheel, freeglut's buttons 3 (up), 4 (down), 5 (left), 6 (right): Ctrl zooms, Shift pans sideways.
void Editor::wheel(int button) {
	const bool forward = button == 3 || button == 5; // up, left
	const int modifiers = glutGetModifiers();
	if ((modifiers & GLUT_ACTIVE_CTRL) != 0) {
		zoomTo(view.zoom() + (button == 3 ? 1 : (button == 4 ? -1 : 0)));
		return;
	}
	const float step = forward ? WHEEL_PAN_CELLS : -WHEEL_PAN_CELLS;
	if (button >= 5 || (modifiers & GLUT_ACTIVE_SHIFT) != 0)
		view.pan(-step, 0.f); // wheel up: left
	else
		view.pan(0.f, step);
	updateHover(pointerX, pointerY);
}

void Editor::mouseButton(int button, int state, int x, int y) {
	track(x, y);

	if (button >= 3) {
		if (state == GLUT_DOWN)
			wheel(button);
		return;
	}
	if (button == GLUT_MIDDLE_BUTTON) {
		panning = state == GLUT_DOWN && MAP_AREA.contains(pointerX, pointerY);
		return;
	}
	if (state == GLUT_UP) {
		painting = false;
		draggingMinimap = false;
		return;
	}

	if (button == GLUT_LEFT_BUTTON && minimapShown() && MINIMAP.contains(pointerX, pointerY)) {
		field = Field::None;
		draggingMinimap = true;
		centerOnMinimap(pointerX, pointerY);
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
	case Target::Tile: {
		int id = paletteOf(layer)[static_cast<size_t>(hoveredSlot)];
		if (layer == Layer::Structure)
			structure = static_cast<Structure>(id);
		else
			tile = id;
		break;
	}
	case Target::PaintMode:
		setMode(Mode::Paint);
		break;
	case Target::CheckMode:
		setMode(Mode::Check);
		break;
	case Target::StructureLayer:
		setLayer(Layer::Structure);
		break;
	case Target::WallLayer:
		setLayer(Layer::Wall);
		break;
	case Target::ObjectsLayer:
		setLayer(Layer::Objects);
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
	const float lastX = pointerX;
	const float lastY = pointerY;
	track(x, y);
	if (draggingMinimap) {
		centerOnMinimap(pointerX, pointerY);
		return;
	}
	if (panning) { // the map follows the mouse
		view.pan(-(pointerX - lastX) / view.cellSize(), -(pointerY - lastY) / view.cellSize());
		updateHover(pointerX, pointerY);
		return;
	}
	if (painting && hovered.col >= 0)
		paint(hovered);
}

void Editor::mouseMove(int x, int y) { track(x, y); }

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
		else if (key == 'l' || key == 'L')
			setLayer(layer == Layer::Structure ? Layer::Wall
					 : layer == Layer::Wall	   ? Layer::Objects
											   : Layer::Structure);
		else if (key == KEY_TAB)
			field = Field::Attribute;
		else if (key == '+' || key == '=')
			zoomTo(view.zoom() + 1);
		else if (key == '-')
			zoomTo(view.zoom() - 1);
		else if (key >= '1' && key <= '3')
			zoomTo(key - '0');
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

void Editor::specialKeyPressed(int key) {
	if (field != Field::None)
		return;
	if (key == GLUT_KEY_LEFT)
		view.pan(-WHEEL_PAN_CELLS, 0.f);
	else if (key == GLUT_KEY_RIGHT)
		view.pan(WHEEL_PAN_CELLS, 0.f);
	else if (key == GLUT_KEY_UP)
		view.pan(0.f, WHEEL_PAN_CELLS);
	else if (key == GLUT_KEY_DOWN)
		view.pan(0.f, -WHEEL_PAN_CELLS);
	updateHover(pointerX, pointerY);
}

// ---- actions ---------------------------------------------------------------

void Editor::setMode(Mode m) {
	mode = m;
	painting = false;
}

void Editor::setLayer(Layer l) {
	layer = l;
	painting = false;
	hoveredSlot = -1;
	if (l != Layer::Structure && isWallType(tile) != (l == Layer::Wall))
		tile = NoObject; // the brush's type is not in this layer's palette
}

// Keeps the level valid: an object or fixture painted into rock carves the cell open, rock painted over them removes
// them.
void Editor::paint(CellPos cell) {
	Tile t = grid.at(cell.col, cell.row);
	if (layer == Layer::Structure) {
		t.structure = structure;
		if (isSolidStructure(structure)) {
			clearObject(t);
			clearFixture(t);
		}
	} else {
		if (layer == Layer::Wall)
			setFixture(t, Fixture{tile, parseNumber(attributeText), parseNumber(valueText)});
		else
			setObject(t, brush());
		if ((hasObject(t) || hasFixture(t)) && isSolidStructure(t.structure))
			t.structure = Structure::Empty;
	}
	grid.set(cell.col, cell.row, t);
	reportStale = true;
}

// Takes the cell's structure, or its fixture's or object's type, attribute and value, into the brush: the layer being
// painted.
void Editor::pick(CellPos cell) {
	Tile t = grid.at(cell.col, cell.row);
	if (layer == Layer::Wall)
		t = fixtureAsTile(t.fixture);
	if (layer == Layer::Structure) {
		structure = t.structure;
		showStatus("Picked " + std::string(structureDef(t.structure).name), false);
		return;
	}
	if (isTileType(t.type))
		tile = t.type;
	attributeText = t.attr != 0 ? std::to_string(t.attr) : "";
	valueText = t.value != 0 ? std::to_string(t.value) : "";
	showStatus("Picked " + std::string(tileInfo(t.type).name) + " " + std::to_string(t.attr) + " " +
				   std::to_string(t.value),
			   false);
}

void Editor::save() {
	if (name.empty()) {
		showStatus("Enter a level name first", true);
		return;
	}
	std::string path = "tools/editor/saved/" + name;
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
	std::string path = "tools/editor/saved/" + name;
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
	if (minimapShown())
		drawMinimap();
	drawPalette();
	drawBrush();
	drawHint();
	drawLevelPanel();
	drawFooter();

	glutSwapBuffers();
}

void Editor::drawBackground() {
	backdrop(visibleArea(), wallTexture.ID());

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

void Editor::drawStructureSwatch(Structure s, const Rect& r, float alpha) {
	switch (s) {
	case Structure::Wall:
		fillRect(r, WALL_TOP, WALL_BOTTOM, alpha);
		break;
	case Structure::Empty:
		fillRect(r, OPEN_COLOR, OPEN_COLOR, alpha);
		break;
	case Structure::HalfWater: // water up to half the cell
		fillRect(r, OPEN_COLOR, OPEN_COLOR, alpha);
		fillRect({r.x, r.y, r.w, r.h / 2}, WATER_TOP, WATER_BOTTOM, alpha);
		break;
	case Structure::DeepWater:
		fillRect(r, DEEP_TOP, DEEP_BOTTOM, alpha);
		break;
	}
}

void Editor::drawObjectIcon(int type, const Rect& r, float alpha) {
	if (isTileType(type) && hasIcon[type]) {
		texturedRect(r, icons[type].ID(), {alpha, alpha, alpha});
		beginShapes();
	}
}

// The fixture and the object share the cell: the brighter one (the layer being painted) is drawn on top.
void Editor::drawCell(const Tile& t, const Rect& r, float structureAlpha, float wallAlpha, float objectAlpha) {
	drawStructureSwatch(t.structure, r, structureAlpha);
	if (!hasObject(t) && !hasFixture(t))
		return;
	const bool wallOnTop = wallAlpha > objectAlpha;
	if (!wallOnTop && hasFixture(t))
		drawObjectIcon(t.fixture.type, r, wallAlpha);
	if (hasObject(t))
		drawObjectIcon(t.type, r, objectAlpha);
	if (wallOnTop && hasFixture(t))
		drawObjectIcon(t.fixture.type, r, wallAlpha);
	if (t.structure == Structure::HalfWater) // the thing stands in the water: its lower half shows through it
		fillRect({r.x, r.y, r.w, r.h / 2}, WATER_TOP, WATER_BOTTOM, HALF_WATER_OVER_OBJECT * structureAlpha);
}

// The map is drawn clipped to the map area: zoomed, the cells reach past it.
void Editor::beginMapClip() const {
	Rect area = visibleArea();
	auto px = [&](float x) { return static_cast<float>(winW) * (x - area.x) / area.w; };
	auto py = [&](float y) { return static_cast<float>(winH) * (y - area.y) / area.h; };
	glEnable(GL_SCISSOR_TEST);
	glScissor(static_cast<int>(std::floor(px(MAP_AREA.x))), static_cast<int>(std::floor(py(MAP_AREA.y))),
			  static_cast<int>(std::ceil(px(MAP_AREA.x + MAP_AREA.w) - px(MAP_AREA.x))),
			  static_cast<int>(std::ceil(py(MAP_AREA.y + MAP_AREA.h) - py(MAP_AREA.y))));
}

void Editor::endMapClip() { glDisable(GL_SCISSOR_TEST); }

void Editor::drawMap() {
	beginShapes();
	fillRect(MAP_AREA.inset(-0.4f), BLACK, BLACK, 1.f);
	beginMapClip();
	const bool structures = layer == Layer::Structure;
	const int firstCol = std::max(0, static_cast<int>(std::floor(view.originCol())));
	const int lastCol = std::min(LEVEL_WIDTH - 1, static_cast<int>(std::ceil(view.originCol() + view.visibleCols())));
	const int firstRow = std::max(0, static_cast<int>(std::floor(view.originRow())));
	const int lastRow = std::min(LEVEL_HEIGHT - 1, static_cast<int>(std::ceil(view.originRow() + view.visibleRows())));
	for (int row = firstRow; row <= lastRow; row++)
		for (int col = firstCol; col <= lastCol; col++)
			drawCell(grid.at(col, row), cellRect(col, row), structures ? 1.f : 0.7f,
					 layer == Layer::Wall ? 1.f : OTHER_LAYER_ALPHA, layer == Layer::Objects ? 1.f : OTHER_LAYER_ALPHA);

	// Grid lines, every 5th a little stronger to help counting.
	for (int col = firstCol; col <= lastCol + 1; col++) {
		float x = view.cellRect(col, 0).x;
		line(x, MAP_AREA.y, x, MAP_AREA.y + MAP_AREA.h, BLACK, col % 5 == 0 ? 0.8f : 0.45f, 1.f);
	}
	for (int row = firstRow; row <= lastRow + 1; row++) {
		float y = view.cellRect(0, row).y;
		line(MAP_AREA.x, y, MAP_AREA.x + MAP_AREA.w, y, BLACK, row % 5 == 0 ? 0.8f : 0.45f, 1.f);
	}
	endMapClip();
}

// Zoomed only: the whole map's structure, small, with a frame for the window. Click or drag to move the window.
void Editor::drawMinimap() {
	beginShapes();
	fillRect(MINIMAP.inset(-0.3f), BLACK, BLACK, 0.85f);
	for (int row = 0; row < LEVEL_HEIGHT; row++)
		for (int col = 0; col < LEVEL_WIDTH; col++) {
			Rect r = {MINIMAP.x + static_cast<float>(col) * MINIMAP_CELL,
					  MINIMAP.y + static_cast<float>(row) * MINIMAP_CELL, MINIMAP_CELL, MINIMAP_CELL};
			drawStructureSwatch(grid.at(col, row).structure, r, 1.f);
		}
	Rect frame = {MINIMAP.x + view.originCol() * MINIMAP_CELL, MINIMAP.y + view.originRow() * MINIMAP_CELL,
				  view.visibleCols() * MINIMAP_CELL, view.visibleRows() * MINIMAP_CELL};
	strokeRect(frame, GOLD_BRIGHT, 1.f, 1.5f);
	strokeRect(MINIMAP.inset(-0.3f), BRONZE, 1.f, 1.5f);
}

void Editor::drawMapOverlay() {
	beginShapes();
	beginMapClip();
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
		if (mode == Mode::Paint) { // preview of the brush
			const float alpha = 0.55f + 0.3f * pulse;
			if (layer == Layer::Structure)
				drawStructureSwatch(structure, r, alpha);
			else
				drawCell(brush(), r, alpha, alpha, alpha); // the brush as an object: it has no fixture
		}
		strokeRect(r.inset(-0.15f), GOLD_BRIGHT, 1.f, 1.5f);
	}

	endMapClip();

	// Cursor readout above the map.
	beginText();
	char buf[128];
	if (hovered.col >= 0) {
		Tile t = grid.at(hovered.col, hovered.row);
		const Fixture& f = t.fixture;
		if (hasFixture(t))
			snprintf(buf, sizeof(buf), "Column %d, row %d:  %s,  %s  %d  %d,  %s  %d  %d", hovered.col, hovered.row,
					 structureDef(t.structure).name, tileInfo(f.type).name, f.attr, f.value, tileInfo(t.type).name,
					 t.attr, t.value);
		else
			snprintf(buf, sizeof(buf), "Column %d, row %d:  %s,  %s  %d  %d", hovered.col, hovered.row,
					 structureDef(t.structure).name, tileInfo(t.type).name, t.attr, t.value);
		text(small, MAP_PANEL.x + 1.f, 96.6f, buf, GOLD);
	}
	std::string modeText = mode == Mode::Paint ? "Paint: click to draw" : "Check: click to inspect";
	if (view.zoom() > 1)
		modeText += "   Zoom " + std::to_string(view.zoom()) + "x";
	text(small, MAP_PANEL.x + MAP_PANEL.w - small.TextWidth(modeText.c_str()) - 1.f, 96.6f, modeText.c_str(), GOLD_DIM);
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
	const std::vector<int> palette = paletteOf(layer);
	for (size_t slot = 0; slot < palette.size(); slot++) {
		const int id = palette[slot];
		Rect r = slotRect(static_cast<int>(slot));
		bool selected = id == selectedId();
		bool hover = static_cast<int>(slot) == hoveredSlot;
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
		const float alpha = selected || hover ? 1.f : 0.8f;
		if (layer == Layer::Structure) {
			drawStructureSwatch(static_cast<Structure>(id), icon, alpha);
		} else {
			fillRect(icon, OPEN_COLOR, OPEN_COLOR, alpha);
			drawObjectIcon(id, icon, alpha);
		}
		strokeRect(icon, BLACK, 0.6f, 1.f);

		if (selected)
			strokeRect(r, GOLD, 1.f, 2.5f);
		else
			strokeRect(r, hover ? GOLD_BRIGHT : BRONZE, 1.f, hover ? 2.f : 1.5f);

		beginText();
		const char* label =
			layer == Layer::Structure ? structureDef(static_cast<Structure>(id)).name : tileInfo(id).name;
		textCentered(small, r.cx(), r.y + 0.45f, label, selected || hover ? GOLD : Color{0.78f, 0.64f, 0.40f});
		beginShapes();
	}

	drawButton(PAINT_BUTTON, "Paint", mode == Mode::Paint, Target::PaintMode);
	drawButton(CHECK_BUTTON, "Check", mode == Mode::Check, Target::CheckMode);
	drawButton(STRUCTURE_BUTTON, "Structure", layer == Layer::Structure, Target::StructureLayer);
	drawButton(WALL_BUTTON, "Wall", layer == Layer::Wall, Target::WallLayer);
	drawButton(OBJECTS_BUTTON, "Objects", layer == Layer::Objects, Target::ObjectsLayer);
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
	Tile cell = checking ? grid.at(checked.col, checked.row) : brush();
	if (checking && layer == Layer::Wall) // the fixture, in the cell's structure
		cell = Tile{cell.fixture.type, cell.fixture.attr, cell.fixture.value, cell.structure};
	CellHint hint = !checking && layer == Layer::Structure ? describeStructure(structure) : describeCell(cell);
	float cx = HINT_PANEL.cx();
	float x = HINT_PANEL.x + 4.f;
	float width = HINT_PANEL.w - 8.f;

	beginText();
	textCentered(heading, cx, HINT_PANEL.y + HINT_PANEL.h - 5.2f, hint.title.c_str(), INK);
	char sub[64];
	if (checking)
		snprintf(sub, sizeof(sub), "Cell at column %d, row %d, %s", checked.col, checked.row,
				 structureDef(cell.structure).name);
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

	textCentered(small, CANVAS_W / 2, 0.8f,
				 "Left click: paint / inspect    Right click: pick cell into brush    P / C: mode    L: layer    "
				 "+ / -: zoom    Wheel, arrows, middle drag: pan    Tab: next field    Ctrl+S / Ctrl+O: save / load",
				 {0.55f, 0.45f, 0.30f});
	beginShapes();
}

std::unique_ptr<Editor> editor;

} // namespace

int main(int argc, char* argv[]) {
	Paths::enterRootDir();
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(START_WIDTH, START_HEIGHT);
	glutCreateWindow("Dungeon Editor");

	editor = std::make_unique<Editor>();
	glutDisplayFunc([] { editor->draw(); });
	glutIdleFunc([] { glutPostRedisplay(); });
	glutReshapeFunc([](int w, int h) { editor->resize(w, h); });
	glutKeyboardFunc([](unsigned char key, int, int) { editor->keyPressed(key); });
	glutSpecialFunc([](int key, int, int) { editor->specialKeyPressed(key); });
	glutMouseFunc([](int button, int state, int x, int y) { editor->mouseButton(button, state, x, y); });
	glutMotionFunc([](int x, int y) { editor->mouseDrag(x, y); });
	glutPassiveMotionFunc([](int x, int y) { editor->mouseMove(x, y); });

	glutMainLoop();
	return 0;
}
