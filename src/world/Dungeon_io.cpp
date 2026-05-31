#include "Dungeon.h"
#include "../state/cashe.h"
#include <fstream>
#include <cstdio>

extern Cashe c;

namespace {
bool ReadMapCells(std::istream& in, Tint* map, int cellCount) {
	for (int jj = 0; jj < cellCount; jj++) {
		in >> map[jj].a;
		in >> map[jj].b;
		in >> map[jj].c;
		if (!in)
			return false;
	}
	return true;
}
} // namespace

bool Dungeon::Load(const char* filename) {
	std::ifstream f(filename);
	if (!f)
		return 0;

	int header;
	f >> header;
	if (header != kMapCellCount) {
		printf("Wrong map header. Expected '%d', got %d\n", kMapCellCount, header);
		return 0;
	}

	if (!ReadMapCells(f, map, kMapCellCount))
		return 0;
	f.close();

	for (int j = 0; j < kMapHeight; j++)
		for (int i = 0; i < kMapWidth; i++)
			if (map[MapIndex(i, j)].a == Door && map[MapIndex(i, j)].b == GateEntrance) {
				x = i;
				y = j;
			}

	return 1;
}
//======================================================================================
bool Dungeon::LoadDump(std::ifstream& f) {
	f >> x >> y;
	if (!f)
		return 0;

	int header;
	f >> header;
	if (header != kMapCellCount) {
		printf("Wrong header. Expected '%d', got %d\n", kMapCellCount, header);
		return 0;
	}

	if (!ReadMapCells(f, map, kMapCellCount))
		return 0;

	return 1;
}
//======================================================================================
void Dungeon::Dump(std::ofstream& f) {
	f << x << " " << y << " ";

	f << kMapCellCount << " ";

	for (int l = 0; l < kMapCellCount; l++)
		f << map[l].a << " " << map[l].b << " " << map[l].c << " ";
}
//======================================================================================
void Dungeon::GetPickUp() {
	if (Map(x, y).a == Treasure) {
		c.invent->GetItem(Map(x, y).b, Map(x, y).c);
		map[MapIndex((int)x, (int)y)].a = Empty;
		if (Map(x, y).b) {
			sprintf(c.status, "Picked up an item  \n");
			c.status_timer->Reset();
		}
	}
}
//======================================================================================
void Dungeon::GetRiddle() {
	if (Map(x, y).a == Ankh) {
		c.IHaveWon = 1;
		return;
	}

	if (Map(x, y).a == Door && Map(x, y).b == GateRiddle) {
		c.rid->GetRiddle();
		c.rid->show = 1;
		SetMapBAtPlayer(GateEmpty);
	} else if (Map(x, y).a == Door && Map(x, y).b == GateExit) {
		c.curMap++;

		char mapName[40];

		sprintf(mapName, "Levels/lvl%d", c.curMap);

		Load(mapName);
	}
}
