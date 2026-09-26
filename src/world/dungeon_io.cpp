#include "dungeon.h"
#include "../state/game_state.h"
#include "../core/service_locator.h"
#include "../core/logger.h"
#include "loot.h"
#include "campaign.h"
#include <algorithm>
#include <iterator>
#include <fstream>
#include <string>
#include <vector>

bool Dungeon::Load(const char* filename) {
	LevelGrid grid;
	std::string error = loadLevelFile(filename, grid);
	if (!error.empty()) {
		LOG_ERRORF("world", "Cannot load level %s: %s", filename, error.c_str());
		return false;
	}
	LoadGrid(grid, filename);
	return true;
}
//======================================================================================
void Dungeon::LoadGrid(const LevelGrid& grid, const char* levelName) {
	std::copy(std::begin(grid.cells), std::end(grid.cells), map);

	for (int j = 0; j < kMapHeight; j++)
		for (int i = 0; i < kMapWidth; i++)
			if (map[MapIndex(i, j)].a == Door && map[MapIndex(i, j)].b == GateEntrance) {
				mapX = static_cast<float>(i);
				mapY = static_cast<float>(j);
			}

	resetMechanisms();
	scatterDecorations(levelName);
}
//======================================================================================
bool Dungeon::LoadCampaignLevel(int number) { return Load(campaignLevelFile(number).c_str()); }
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

	if (!readLevelCells(f, map, kMapCellCount))
		return false;

	resetMechanisms();
	// Saves from before the keys end here.
	int keys = 0;
	if (f >> keys)
		keysHeld = keys;
	return true;
}
//======================================================================================
void Dungeon::Dump(std::ofstream& f) {
	f << mapX << " " << mapY << " ";

	f << kMapCellCount << " ";

	for (int l = 0; l < kMapCellCount; l++)
		f << map[l].a << " " << map[l].b << " " << map[l].c << " ";

	f << keysHeld << " ";
}
//======================================================================================
void Dungeon::GetPickUp() {
	if (Map(mapX, mapY).a == Treasure) {
		int type = Map(mapX, mapY).b;
		int id = Map(mapX, mapY).c;
		map[MapIndex(static_cast<int>(mapX), static_cast<int>(mapY))].a = Empty;
		if (type == 0) // empty chest
			return;

		// One line per item: "Found: Sword", then "+ Small Stamina" for each bonus.
		std::string found;
		std::vector<LootItem> loot = RollChestLoot(type, id);
		for (size_t i = 0; i < loot.size(); i++) {
			GAME_STATE.ui.invent->GetItem(loot[i].type, loot[i].id);
			found += std::string(i == 0 ? "Found: " : "\n+ ") + inventory::ItemName(loot[i].type, loot[i].id);
		}
		snprintf(GAME_STATE.status, sizeof(GAME_STATE.status), "%s", found.c_str());
		GAME_STATE.status_timer->Reset();
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
		LoadCampaignLevel(GAME_STATE.curMap);
	}
}
