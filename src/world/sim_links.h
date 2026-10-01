#ifndef SIM_LINKS_H
#define SIM_LINKS_H

struct Assets;
class Player;
class Journal;
class ItemBag;
struct GameRandom;
class WorldEvents;

// What the Dungeon is given, instead of reaching into the app: GameState links them once it has loaded
// (docs/plan/solved/world-without-game.md). The Dungeon passes the player, the journal and the events on to its
// monsters (MonsterLinks). What it tells the app goes into events.
struct SimLinks {
	Player* player = nullptr;
	Journal* journal = nullptr;
	ItemBag* items = nullptr; // the inventory's contents
	GameRandom* random = nullptr;
	Assets* assets = nullptr; // models, textures and the monster types
	WorldEvents* events = nullptr;
};

#endif
