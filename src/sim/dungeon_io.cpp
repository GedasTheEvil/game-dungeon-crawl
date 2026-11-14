#include "dungeon.h"
#include "../core/logger.h"
#include <fstream>
#include <string>
#include <string_view>
#include "../level/campaign.h"

namespace {
// The containers in a save: one character per cell after the tag. An opened container is saved as open, its clip
// played. A dropped one: the chest, or a dead mimic jar's jar (plan tomb-curse-containers).
constexpr std::string_view CONTAINERS_TAG = "containers:";
constexpr char CLOSED = '.', OPENED = 'o', EMPTIED = 'e', DROPPED = 'd', DROPPED_EMPTIED = 'x', DROPPED_JAR = 'j',
			   DROPPED_JAR_EMPTIED = 'k';
// The hurt monsters in a save (plan save-live-monsters): the tag, their count, then col row hp each; the boss fight's
// counters after BOSS_TAG, if one runs. Saves from before have neither.
constexpr std::string_view MONSTERS_TAG = "monsters:";
constexpr std::string_view BOSS_TAG = "boss:";
} // namespace

char Dungeon::containerChar(const ContainerCell& c) {
	if (!c.opened)
		return CLOSED;
	if (c.dropped && c.droppedKind == Container::Jar)
		return c.emptied ? DROPPED_JAR_EMPTIED : DROPPED_JAR;
	if (c.dropped)
		return c.emptied ? DROPPED_EMPTIED : DROPPED;
	return c.emptied ? EMPTIED : OPENED;
}
//======================================================================================
Dungeon::ContainerCell Dungeon::containerFromChar(char c) {
	ContainerCell cell;
	const bool jar = c == DROPPED_JAR || c == DROPPED_JAR_EMPTIED;
	cell.opened = c == OPENED || c == EMPTIED || c == DROPPED || c == DROPPED_EMPTIED || jar;
	cell.emptied = c == EMPTIED || c == DROPPED_EMPTIED || c == DROPPED_JAR_EMPTIED;
	cell.dropped = c == DROPPED || c == DROPPED_EMPTIED || jar;
	cell.droppedKind = jar ? Container::Jar : Container::Chest;
	cell.openedMs = GameClock::now() - CONTAINER_OPENED_LONG_AGO_MS;
	return cell;
}
//======================================================================================
bool Dungeon::Load(const char* filename, int depth) {
	LevelGrid grid;
	std::string error = loadLevelFile(filename, grid);
	if (!error.empty()) {
		LOG_ERRORF("world", "Cannot load level %s: %s", filename, error.c_str());
		return false;
	}
	LoadGrid(grid, filename, depth);
	return true;
}
//======================================================================================
void Dungeon::LoadGrid(const LevelGrid& grid, const char* levelName, int depth) {
	travel = {};
	dial = {};
	stargateRestDeg = 0.f;
	std::copy(std::begin(grid.cells), std::end(grid.cells), map);
	meta = grid.meta;
	findSecretRooms(); // plan heka-surplus

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
	std::fill(std::begin(containers), std::end(containers), ContainerCell{});
	exploreAroundPlayer();
	clearMonsters();
	resetPlayerMotion();
	resetMechanisms();
	scatterDecorations(levelName, depth);
}
//======================================================================================
bool Dungeon::LoadCampaignLevel(int number) {
	levelNumber = number;
	if (!Load(campaignLevelFile(number).c_str(), number))
		return false;
	if (isZone(meta.zone) && sim.events != nullptr) // plan campaign-texts: the hour's note, once
		sim.events->Note(hourNote(meta.zone));
	return true;
}
//======================================================================================
bool Dungeon::LoadDump(std::istream& f) {
	f >> mapX >> mapY;
	if (!f)
		return false;

	// Saves from before the level format v2 hold a v1 map: it is converted.
	std::string error = readLevel(f, map, nullptr, &meta);
	if (!error.empty()) {
		LOG_ERRORF("world", "Cannot read the saved map: %s", error.c_str());
		return false;
	}
	findSecretRooms(); // plan heka-surplus

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
	// Saves from before the treasure containers (plan treasure-containers) end here: every chest is closed.
	std::fill(std::begin(containers), std::end(containers), ContainerCell{});
	const std::streampos before = f.tellg();
	std::string opened;
	if (f >> opened && opened.size() == static_cast<size_t>(MAP_CELL_COUNT) + CONTAINERS_TAG.size() &&
		opened.compare(0, CONTAINERS_TAG.size(), CONTAINERS_TAG) == 0) {
		for (int l = 0; l < MAP_CELL_COUNT; l++)
			containers[l] = containerFromChar(opened[CONTAINERS_TAG.size() + static_cast<size_t>(l)]);
	} else {
		f.clear();
		f.seekg(before);
	}
	return loadMonsters(f);
}
//======================================================================================
// Only the living, unslain monsters below their max health: the others come out of their tiles as new, as before.
// A minion has no tile: it is counted for the boss fight.
void Dungeon::dumpMonsters(std::ostream& f) const {
	std::vector<SavedMonster> hurt;
	for (const Monster& mon : monsters)
		if (mon.Active() && mon.Alive() && !mon.Minion() && mon.Health() < mon.MaxHealth())
			hurt.push_back({mon.Col(), mon.SpawnRow(), mon.Health()});
	f << MONSTERS_TAG << ' ' << hurt.size();
	for (const SavedMonster& m : hurt)
		f << ' ' << m.col << ' ' << m.row << ' ' << m.hp;
	f << ' ';
	if (bossFight.slot >= 0 && monsters[bossFight.slot].Alive())
		f << BOSS_TAG << ' ' << bossFight.summoned << ' ' << bossFight.hatched << ' ' << bossFight.firstLeft << ' '
		  << LivingMinions() << ' ';
}
//======================================================================================
bool Dungeon::loadMonsters(std::istream& f) {
	savedMonsters.clear();
	savedBossFight.reset();
	std::streampos before = f.tellg();
	std::string tag;
	if (!(f >> tag) || tag != MONSTERS_TAG) {
		f.clear();
		f.seekg(before);
		return true;
	}
	int count = 0;
	if (!(f >> count) || count < 0 || count > MAX_MONSTERS)
		return false;
	for (int k = 0; k < count; k++) {
		SavedMonster m;
		if (!(f >> m.col >> m.row >> m.hp))
			return false;
		savedMonsters.push_back(m);
	}
	before = f.tellg();
	SavedBossFight boss;
	if (f >> tag && tag == BOSS_TAG && f >> boss.summoned >> boss.hatched >> boss.firstLeft >> boss.minions)
		savedBossFight = boss;
	else {
		f.clear();
		f.seekg(before);
	}
	return true;
}
//======================================================================================
void Dungeon::Dump(std::ostream& f) {
	f << mapX << " " << mapY << " ";

	writeLevel(f, map, meta);

	f << keysHeld << " ";

	for (bool cell : explored)
		f << (cell ? '1' : '0');
	f << " " << CONTAINERS_TAG;
	for (const ContainerCell& c : containers)
		f << containerChar(c);
	f << " ";
	dumpMonsters(f);
}
