#include "dungeon.h"
#include "../state/cashe.h"
#include "../core/service_locator.h"
#include <fstream>
#include <cstdio>

namespace {
bool readMapCells(std::istream& in, Tint* map, int cellCount) {
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
		return false;

	int header;
	f >> header;
	if (header != kMapCellCount) {
		printf("Wrong map header. Expected '%d', got %d\n", kMapCellCount, header);
		return false;
	}

	if (!readMapCells(f, map, kMapCellCount))
		return false;
	f.close();

	for (int j = 0; j < kMapHeight; j++)
		for (int i = 0; i < kMapWidth; i++)
			if (map[MapIndex(i, j)].a == Door && map[MapIndex(i, j)].b == GateEntrance) {
				x = static_cast<float>(i);
				y = static_cast<float>(j);
			}

	return true;
}
//======================================================================================
bool Dungeon::LoadDump(std::ifstream& f) {
	f >> x >> y;
	if (!f)
		return false;

	int header;
	f >> header;
	if (header != kMapCellCount) {
		printf("Wrong header. Expected '%d', got %d\n", kMapCellCount, header);
		return false;
	}

	if (!readMapCells(f, map, kMapCellCount))
		return false;

	return true;
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
		GAME_STATE.ui.invent->GetItem(Map(x, y).b, Map(x, y).c);
		map[MapIndex((int)x, (int)y)].a = Empty;
		if (Map(x, y).b != 0) {
			sprintf(GAME_STATE.status, "Picked up an item  \n");
			GAME_STATE.status_timer->Reset();
		}
	}
}
//======================================================================================
void Dungeon::GetRiddle() {
	if (Map(x, y).a == Ankh) {
		GAME_STATE.IHaveWon = true;
		return;
	}

	if (Map(x, y).a == Door && Map(x, y).b == GateRiddle) {
		GAME_STATE.ui.rid->GetRiddle();
		GAME_STATE.ui.rid->show = true;
		SetMapBAtPlayer(GateEmpty);
	} else if (Map(x, y).a == Door && Map(x, y).b == GateExit) {
		GAME_STATE.curMap++;

		char mapName[40];

		sprintf(mapName, "Levels/lvl%d", GAME_STATE.curMap);

		Load(mapName);
	}
}
