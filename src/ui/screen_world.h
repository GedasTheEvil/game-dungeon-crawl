#ifndef SCREEN_WORLD_H
#define SCREEN_WORLD_H

#include "../level/input/bindings.h"
#include <functional>
#include <string>

struct Assets;
struct RenderSettings;
struct Settings;
class Player;
class ItemBag;
class ItemCommands;
class Journal;
class Dungeon;
class WorldEvents;
struct GameRandom;
class Leaderboard;
class SaveService;
class MessageLog;
class InscriptionBook;

// What the screens read and change of the game (plan game-state-split): handed to each screen instead of Game().
// GameState builds it once; the screens over each other come with UIContext.
struct ScreenWorld {
	Assets& assets;
	const RenderSettings& render; // the window's size
	Settings& settings;
	Player& player;
	ItemBag& bag;
	ItemCommands& items;
	Journal& journal;
	Dungeon& dungeon;
	WorldEvents& events;
	GameRandom& random;
	Leaderboard& leaderboard;
	SaveService& saves;
	MessageLog& messages;
	InscriptionBook& inscriptions;
	std::function<void(bool save)> applySettings; // GameState::ApplySettings: the graphics and sound choices
	std::function<void()> newGame; // GameState::NewGame
	const bool& alwaysGift; // every offering brings a gift (GameState::alwaysGift, scenario `gifts on`)
	const Caps& caps; // the key caps' names: the keyboard's or the last used gamepad's (GameState::caps)
	const Caps& padFamily; // the last used gamepad's family (GameState::padFamily)

	// A plain status line on the level the player is on (GameState::ShowStatus).
	void Status(const std::string& text) const;
};

#endif
