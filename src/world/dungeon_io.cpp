#include "dungeon.h"
#include "../state/game_state.h"
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

	bool entranceFound = false;
	for (int j = 0; j < MAP_HEIGHT; j++)
		for (int i = 0; i < MAP_WIDTH; i++)
			if (map[MapIndex(i, j)].type == Door && map[MapIndex(i, j)].attr == GateEntrance) {
				mapX = static_cast<float>(i);
				mapY = static_cast<float>(j);
				entranceFound = true;
			}
	if (!entranceFound)
		LOG_WARNINGF("world", "Level %s has no entrance (Door with attribute 1), player position not set", levelName);

	std::fill(std::begin(explored), std::end(explored), false);
	exploreAroundPlayer();
	clearMonsters();
	resetPlayerMotion();
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
	if (header != MAP_CELL_COUNT) {
		LOG_ERRORF("world", "Wrong dump header. Expected '%d', got %d", MAP_CELL_COUNT, header);
		return false;
	}

	if (!readLevelCells(f, map, MAP_CELL_COUNT))
		return false;

	clearMonsters();
	resetPlayerMotion();
	resetMechanisms();
	// Saves from before the keys end here.
	int keys = 0;
	if (f >> keys)
		keysHeld = keys;
	levelKeys |= keysHeld; // their tiles are gone from the saved map
	// Saves from before the draft map end here: the map starts over from the player's position.
	std::fill(std::begin(explored), std::end(explored), false);
	std::string exploredBits;
	if (f >> exploredBits && exploredBits.size() == static_cast<size_t>(MAP_CELL_COUNT))
		for (int l = 0; l < MAP_CELL_COUNT; l++)
			explored[l] = exploredBits[static_cast<size_t>(l)] == '1';
	exploreAroundPlayer();
	return true;
}
//======================================================================================
void Dungeon::Dump(std::ofstream& f) {
	f << mapX << " " << mapY << " ";

	f << MAP_CELL_COUNT << " ";

	for (int l = 0; l < MAP_CELL_COUNT; l++)
		f << map[l].type << " " << map[l].attr << " " << map[l].value << " ";

	f << keysHeld << " ";

	for (bool cell : explored)
		f << (cell ? '1' : '0');
	f << " ";
}
//======================================================================================
void Dungeon::PickUp() {
	if (Map(mapX, mapY).type == Treasure) {
		int type = Map(mapX, mapY).attr;
		int id = Map(mapX, mapY).value;
		map[MapIndex(static_cast<int>(mapX), static_cast<int>(mapY))].type = Empty;
		if (type == 0) // empty chest
			return;

		// One line per item: "Found: Sword", then "+ Small Stamina" for each bonus.
		std::string found;
		std::vector<LootItem> loot = RollChestLoot(type, id);
		for (size_t i = 0; i < loot.size(); i++) {
			Game().ui.inventory->AddItem(loot[i].type, loot[i].id);
			found += std::string(i == 0 ? "Found: " : "\n+ ") + Inventory::ItemName(loot[i].type, loot[i].id);
		}
		Game().ShowStatus("%s", found.c_str());
	}
}
//======================================================================================
void Dungeon::Interact() {
	if (Map(mapX, mapY).type == Ankh) {
		Game().hasWon = true;
		return;
	}

	if (Map(mapX, mapY).type == Door && Map(mapX, mapY).attr == GateRiddle) {
		Game().ui.riddle->Ask();
		Game().ui.riddle->show = true;
		SetMapBAtPlayer(GateEmpty);
	} else if (Map(mapX, mapY).type == Door && Map(mapX, mapY).attr == GateExit) {
		Game().curMap++;
		LoadCampaignLevel(Game().curMap);
	} else if (isTeleporter(Map(mapX, mapY)))
		Teleport();
}
//======================================================================================
// The player steps out in the middle of the partner gate, in the plasma.
void Dungeon::Teleport() {
	const JumpState& jump = Game().player->jump;
	if (jump.jumping || jump.falling)
		return;
	int to = teleportPartner(map, MapIndex(static_cast<int>(mapX), static_cast<int>(mapY)));
	if (to < 0)
		return;
	Game().assets.sounds.teleport.Play();
	const int row = to / MAP_WIDTH;
	mapX = static_cast<float>(to % MAP_WIDTH) + TELEPORT_ARRIVAL_X;
	mapY = static_cast<float>(row);
	resetPlayerMotion();
	exploreAroundPlayer();
}
