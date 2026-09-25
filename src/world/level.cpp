#include "level.h"
#include <fstream>

bool readLevelCells(std::istream& in, Tint* cells, int cellCount) {
	for (int i = 0; i < cellCount; i++) {
		in >> cells[i].a >> cells[i].b >> cells[i].c;
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
	for (const Tint& t : grid.cells)
		out << t.a << ' ' << t.b << ' ' << t.c << " \n";
	return static_cast<bool>(out);
}
