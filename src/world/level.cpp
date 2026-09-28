#include "level.h"
#include <fstream>

bool readLevelCells(std::istream& in, Tile* cells, int cellCount) {
	for (int i = 0; i < cellCount; i++) {
		in >> cells[i].type >> cells[i].attr >> cells[i].value;
		if (!in)
			return false;
	}
	return true;
}

std::string loadLevelFile(const char* path, LevelGrid& grid) {
	std::ifstream in(path);
	if (!in)
		return "cannot open file";

	int header = 0;
	in >> header;
	if (header != LEVEL_CELL_COUNT)
		return "wrong header " + std::to_string(header) + ", expected " + std::to_string(LEVEL_CELL_COUNT);
	if (!readLevelCells(in, grid.cells, LEVEL_CELL_COUNT))
		return "file ends early";
	return "";
}

bool saveLevelFile(const char* path, const LevelGrid& grid) {
	std::ofstream out(path);
	if (!out)
		return false;

	out << LEVEL_CELL_COUNT << '\n';
	for (const Tile& t : grid.cells)
		out << t.type << ' ' << t.attr << ' ' << t.value << " \n";
	return static_cast<bool>(out);
}
