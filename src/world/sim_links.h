#ifndef SIM_LINKS_H
#define SIM_LINKS_H

#include "../entities/monster.h"

struct Assets;
class Player;
class Journal;
class ItemBag;
struct GameRandom;
class WorldEvents;
class InscriptionBook;
class NameList;

// What the Dungeon is given, instead of reaching into the app: GameState links them once it has loaded (plan
// world-without-game). The Dungeon passes the player, the journal and the events on to its monsters (MonsterLinks).
// What it tells the app goes into events.
struct SimLinks {
	Player* player = nullptr;
	Journal* journal = nullptr;
	ItemBag* items = nullptr; // the inventory's contents
	GameRandom* random = nullptr;
	Assets* assets = nullptr; // models and textures: only the drawing (dungeon_render*.cpp) reads them
	WorldEvents* events = nullptr;
	const MonsterTypes* monsterTypes = nullptr; // the kinds and their ModelInfo, by MonsterTypeId
	const InscriptionBook* inscriptions = nullptr; // the texts of the wall inscriptions; null: none can be read
	const NameList* names = nullptr; // the royal names of the name gates; null: none (a name cartouche is worn)
};

#endif
