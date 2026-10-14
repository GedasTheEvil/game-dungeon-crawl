#include "level.h"
#include <cstring>
#include <fstream>

int teleportPartner(const Tile* cells, int index) {
	for (int i = 0; i < LEVEL_WIDTH * LEVEL_HEIGHT; i++)
		if (i != index && isTeleporter(cells[i]) && teleportPair(cells[i]) == teleportPair(cells[index]))
			return i;
	return -1;
}

TeleportPairs::TeleportPairs(const Tile* cells) {
	for (int i = 0; i < LEVEL_WIDTH * LEVEL_HEIGHT; i++)
		partners[i] = isTeleporter(cells[i]) ? teleportPartner(cells, i) : -1;
}

LevelGrid::LevelGrid() {
	for (Tile& t : cells)
		t = wallTile();
}

Tile convertV1Tile(int type, int attr, int value) {
	constexpr int V1_WALL = 0;
	constexpr int V1_AREA3D = 7;
	if (type == V1_WALL)
		return wallTile();
	Tile cell;
	if (type != V1_AREA3D)
		setThing(cell, Tile{type, attr, value});
	return cell;
}

namespace {
constexpr int CELLS = LEVEL_WIDTH * LEVEL_HEIGHT;

std::string readV1(std::istream& in, Tile* cells) {
	int type = 0, attr = 0, value = 0;
	for (int i = 0; i < LEVEL_CELL_COUNT; i++) {
		if (!(in >> type >> attr >> value))
			return "file ends early";
		cells[i] = convertV1Tile(type, attr, value);
	}
	return "";
}

// A layer's list: "NAME COUNT", then COUNT lines "COL ROW TYPE ATTR VALUE". Each thing goes in its own layer (a v2
// file's wall types are in its object list); the wall list holds only wall types.
std::string readThings(std::istream& in, Tile* cells, const char* name, bool wall) {
	std::string word;
	int count = 0;
	if (!(in >> word >> count) || word != name || count < 0)
		return std::string("no ") + (wall ? "wall" : "object") + " layer";
	for (int k = 0; k < count; k++) {
		int col = 0, row = 0;
		Tile thing;
		if (!(in >> col >> row >> thing.type >> thing.attr >> thing.value))
			return "file ends early";
		if (!LevelGrid::inBounds(col, row))
			return std::string(wall ? "fixture" : "object") + " outside the level at " + std::to_string(col) + " " +
				   std::to_string(row);
		if (wall && !isWallType(thing.type))
			return "type " + std::to_string(thing.type) + " in the wall layer at " + std::to_string(col) + " " +
				   std::to_string(row);
		setThing(cells[row * LEVEL_WIDTH + col], thing);
	}
	return "";
}

std::string readDcLevel(std::istream& in, Tile* cells, int& version) {
	int width = 0, height = 0;
	if (!(in >> version >> width >> height))
		return "file ends early";
	if (version != 2 && version != LEVEL_VERSION)
		return "unknown version " + std::to_string(version);
	if (width != LEVEL_WIDTH || height != LEVEL_HEIGHT)
		return "wrong size " + std::to_string(width) + " x " + std::to_string(height);

	std::string word;
	if (!(in >> word) || word != "structure")
		return "no structure layer";
	for (int row = LEVEL_HEIGHT - 1; row >= 0; row--) {
		std::string line;
		if (!(in >> line))
			return "file ends early";
		if (line.size() != static_cast<size_t>(LEVEL_WIDTH))
			return "structure row " + std::to_string(row) + " is not " + std::to_string(LEVEL_WIDTH) + " wide";
		for (int col = 0; col < LEVEL_WIDTH; col++) {
			const char* glyph = std::strchr(STRUCTURE_GLYPHS, line[static_cast<size_t>(col)]);
			if (glyph == nullptr || *glyph == '\0')
				return "unknown structure '" + line.substr(static_cast<size_t>(col), 1) + "' in row " +
					   std::to_string(row);
			cells[row * LEVEL_WIDTH + col] = Tile{};
			cells[row * LEVEL_WIDTH + col].structure = static_cast<Structure>(glyph - STRUCTURE_GLYPHS);
		}
	}

	if (version >= 3) {
		std::string error = readThings(in, cells, "wall", true);
		if (!error.empty())
			return error;
	}
	return readThings(in, cells, "objects", false);
}
} // namespace

std::string readLevel(std::istream& in, Tile* cells, int* version) {
	std::string head;
	if (!(in >> head))
		return "empty file";
	int read = 0;
	if (version == nullptr)
		version = &read;
	if (head == LEVEL_MAGIC)
		return readDcLevel(in, cells, *version);
	if (head != std::to_string(LEVEL_CELL_COUNT))
		return "wrong header " + head + ", expected " + LEVEL_MAGIC + " or " + std::to_string(LEVEL_CELL_COUNT);
	*version = 1;
	return readV1(in, cells);
}

void writeLevel(std::ostream& out, const Tile* cells) {
	out << LEVEL_MAGIC << ' ' << LEVEL_VERSION << ' ' << LEVEL_WIDTH << ' ' << LEVEL_HEIGHT << "\nstructure\n";
	for (int row = LEVEL_HEIGHT - 1; row >= 0; row--) {
		for (int col = 0; col < LEVEL_WIDTH; col++)
			out << structureGlyph(cells[row * LEVEL_WIDTH + col].structure);
		out << '\n';
	}
	int count = 0;
	for (int i = 0; i < CELLS; i++)
		count += hasFixture(cells[i]) ? 1 : 0;
	out << "wall " << count << '\n';
	for (int i = 0; i < CELLS; i++) {
		const Fixture& f = cells[i].fixture;
		if (hasFixture(cells[i]))
			out << i % LEVEL_WIDTH << ' ' << i / LEVEL_WIDTH << ' ' << f.type << ' ' << f.attr << ' ' << f.value
				<< '\n';
	}
	count = 0;
	for (int i = 0; i < CELLS; i++)
		count += hasObject(cells[i]) ? 1 : 0;
	out << "objects " << count << '\n';
	for (int i = 0; i < CELLS; i++) {
		const Tile& t = cells[i];
		if (hasObject(t))
			out << i % LEVEL_WIDTH << ' ' << i / LEVEL_WIDTH << ' ' << t.type << ' ' << t.attr << ' ' << t.value
				<< '\n';
	}
}

std::string loadLevelFile(const char* path, LevelGrid& grid, int* version) {
	std::ifstream in(path);
	if (!in)
		return "cannot open file";
	return readLevel(in, grid.cells, version);
}

bool saveLevelFile(const char* path, const LevelGrid& grid) {
	std::ofstream out(path);
	if (!out)
		return false;
	writeLevel(out, grid.cells);
	return static_cast<bool>(out);
}
