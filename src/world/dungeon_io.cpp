#include "dungeon.h"
#include "../state/assets.h"
#include "../entities/player.h"
#include "item_bag.h"
#include "journal.h"
#include "rng.h"
#include "world_events.h"
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
bool Dungeon::LoadCampaignLevel(int number) {
	levelNumber = number;
	return Load(campaignLevelFile(number).c_str());
}
//======================================================================================
bool Dungeon::LoadDump(std::ifstream& f) {
	f >> mapX >> mapY;
	if (!f)
		return false;

	// Saves from before the level format v2 hold a v1 map: it is converted.
	std::string error = readLevel(f, map);
	if (!error.empty()) {
		LOG_ERRORF("world", "Cannot read the saved map: %s", error.c_str());
		return false;
	}

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

	writeLevel(f, map);

	f << keysHeld << " ";

	for (bool cell : explored)
		f << (cell ? '1' : '0');
	f << " ";
}
//======================================================================================
void Dungeon::PickUp() {
	if (Map(mapX, mapY).type == Treasure) {
		std::optional<ItemKind> placed = itemFromFile(Map(mapX, mapY).attr, Map(mapX, mapY).value);
		clearObject(map[MapIndex(static_cast<int>(mapX), static_cast<int>(mapY))]);
		if (!placed) // an empty chest (type 0), or an item the game does not know
			return;

		// One line per item: "Found: Sword", then "+ Small Stamina" for each bonus.
		std::string found;
		std::vector<ItemKind> loot = RollChestLoot(*placed, sim.random->gameplay);
		for (size_t i = 0; i < loot.size(); i++) {
			sim.items->Find(loot[i], *sim.journal);
			found += std::string(i == 0 ? "Found: " : "\n+ ") + itemText(loot[i]).name;
		}
		sim.events->Status("%s", found.c_str());
	}
}
//======================================================================================
// Keys, treasure and levers are picked up or pulled elsewhere (PickUp, the mechanisms).
void Dungeon::Interact() {
	const Tile here = Map(mapX, mapY);
	if (here.type == Ankh) {
		won = true;
		return;
	}
	if (here.type != Door)
		return;
	switch (here.attr) {
	case GateRiddle:
		sim.events->AskRiddle();
		SetMapBAtPlayer(GateEmpty);
		break;
	case GateExit:
		LoadCampaignLevel(levelNumber + 1);
		break;
	case GateTeleport:
		Teleport();
		break;
	default: // the entrance, an answered riddle gate
		break;
	}
}
//======================================================================================
// The player steps out in the middle of the partner gate, in the plasma.
void Dungeon::Teleport() {
	const JumpState& jump = sim.player->jump;
	if (jump.jumping || jump.falling)
		return;
	int to = teleportPartner(map, MapIndex(static_cast<int>(mapX), static_cast<int>(mapY)));
	if (to < 0)
		return;
	sim.events->Play(WorldSound::Teleport);
	const int row = to / MAP_WIDTH;
	mapX = static_cast<float>(to % MAP_WIDTH) + TELEPORT_ARRIVAL_X;
	mapY = static_cast<float>(row);
	resetPlayerMotion();
	exploreAroundPlayer();
}
