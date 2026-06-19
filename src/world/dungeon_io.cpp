#include "dungeon.h"
#include "../state/game_state.h"
#include "../core/service_locator.h"
#include "../core/logger.h"
#include <fstream>

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
		LOG_ERRORF("world", "Wrong map header. Expected '%d', got %d", kMapCellCount, header);
		return false;
	}

	if (!readMapCells(f, map, kMapCellCount))
		return false;
	f.close();

	for (int j = 0; j < kMapHeight; j++)
		for (int i = 0; i < kMapWidth; i++)
			if (map[MapIndex(i, j)].a == Door && map[MapIndex(i, j)].b == GateEntrance) {
				mapX = static_cast<float>(i);
				mapY = static_cast<float>(j);
			}

	return true;
}
//======================================================================================
bool Dungeon::LoadDump(std::ifstream& f) {
	f >> mapX >> mapY;
	if (!f)
		return false;

	int header;
	f >> header;
	if (header != kMapCellCount) {
		LOG_ERRORF("world", "Wrong dump header. Expected '%d', got %d", kMapCellCount, header);
		return false;
	}

	if (!readMapCells(f, map, kMapCellCount))
		return false;

	return true;
}
//======================================================================================
void Dungeon::Dump(std::ofstream& f) {
	f << mapX << " " << mapY << " ";

	f << kMapCellCount << " ";

	for (int l = 0; l < kMapCellCount; l++)
		f << map[l].a << " " << map[l].b << " " << map[l].c << " ";
}
//======================================================================================
void Dungeon::GetPickUp() {
	if (Map(mapX, mapY).a == Treasure) {
		GAME_STATE.ui.invent->GetItem(Map(mapX, mapY).b, Map(mapX, mapY).c);
		map[MapIndex(static_cast<int>(mapX), static_cast<int>(mapY))].a = Empty;
		if (Map(mapX, mapY).b != 0) {
			sprintf(GAME_STATE.status, "Picked up an item  \n");
			GAME_STATE.status_timer->Reset();
		}
	}
}
//======================================================================================
void Dungeon::GetRiddle() {
	if (Map(mapX, mapY).a == Ankh) {
		GAME_STATE.IHaveWon = true;
		return;
	}

	if (Map(mapX, mapY).a == Door && Map(mapX, mapY).b == GateRiddle) {
		GAME_STATE.ui.rid->GetRiddle();
		GAME_STATE.ui.rid->show = true;
		SetMapBAtPlayer(GateEmpty);
	} else if (Map(mapX, mapY).a == Door && Map(mapX, mapY).b == GateExit) {
		GAME_STATE.curMap++;

		char mapName[40];

		sprintf(mapName, "Levels/lvl%d", GAME_STATE.curMap);

		Load(mapName);
	}
}
