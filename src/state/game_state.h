#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "../graphics/assets.h"
#include "../sim/player.h"
#include "../graphics/player_view.h"
#include "../graphics/texture_registry.h"
#include "../render/animated_model.h"
#include "../sim/dungeon/dungeon.h"
#include "../sim/dungeon/dungeon_save.h"
#include "../graphics/dungeon/dungeon_view.h"
#include "../render/font.h"
#include "../graphics/item.h"
#include "../level/rules/item_bag.h"
#include "../core/rng.h"
#include "../audio/sound.h"
#include "../level/rules/player_stats.h"
#include "../core/timer.h"
#include "../graphics/trap.h"
#include "../level/ui/ui_layout.h"
#include "../level/ui/message_log.h"
#include "../level/inscriptions.h"
#include "../level/files/names.h"
#include "../level/rules/journal.h"
#include "../level/files/leaderboard.h"
#include "../level/rules/world_events.h"
#include "saves.h"
#include "item_commands.h"
#include "../level/files/settings_ini.h"
#include <array>
#include <memory>
#include <string>

struct Camera {
	float rotW = -110.f; // the player's yaw: 70 walking right, -110 left
	float rotM = 0.f;
	float rotN = 0.f;
	[[nodiscard]] int Facing() const { return rotW > 0 ? 1 : -1; } // +1 right, -1 left
};

struct RenderSettings {
	bool Hitboxes = false; // debug outlines of the monster and player hitboxes (F3, scenario `hitboxes on`)
	int resX = 800; // the window's size now; Settings::display holds the one it opens with

	int resY = 500;
};

struct GameTimers {
	Timer idleModel{300}; // back to the idle clip after walking
};

struct UIContext; // state/ui_context.h: the screens

class GameState {
  public:
	Assets assets;
	Camera camera;
	RenderSettings render;
	Settings settings; // the Options choices (saves/settings.ini)
	bool cacheLoaded = false;
	MessageLog messages; // the gameplay status lines and this level's log of them (plan message-log)
	std::unique_ptr<Player> player = std::make_unique<Player>(); // set up by Load
	// The player's items: weapons, potions, amulets, rings (plan bag-out-of-inventory). The inventory screen shows
	// and changes it, the world adds to it (SimLinks), the save holds it.
	ItemBag bag;
	PlayerView playerView; // the player's model, drawn
	GameTimers timers;
	GameRandom random;
	Dungeon dungeon;
	DungeonView dungeonView{dungeon, assets}; // the dungeon drawn (plan dungeon-view-split)
	Journal journal; // what the archaeologist wrote down, kept per save game
	InscriptionBook inscriptions; // the wall inscriptions' texts, content/inscriptions/*.txt
	NameList names; // the royal names of the name gates, content/names/*.txt (plan secret-name-gates)
	WorldEvents events; // what the world and the player told the app since the last ApplyWorldEvents
	Leaderboard leaderboard; // the won runs, saves/leaderboard.txt (plan leaderboard)
	SaveService saves{*this}; // the save files, the autosave and the quick save (plan game-state-split)
	// The quick drinks, the weapon keys, insanity's club (plan game-state-split).
	ItemCommands itemCommands{*this};
	// The first hints show in a campaign run (plan controls-help); scenario `hints on` shows them in any run.
	bool hintsAnyRun = false;
	// Every offering brings a gift (plan offering-gift), for the play scripts: scenario `gifts on`.
	bool alwaysGift = false;
	bool debug = false; // --debug: F3 shows the hitboxes (plan controls-help)
	// The screens, apart so this header includes no ui/ (plan game-state-split): bound to uiOwned. Last: they hold
	// references to the parts above (ScreenWorld).
	std::unique_ptr<UIContext> uiOwned;
	UIContext& ui;

	GameState();
	~GameState();
	void Load();
	[[nodiscard]] UiViewState ViewState() const; // what a save keeps of the screens and the look (UiViewState)
	void ApplyViewState(const UiViewState& view);
	void NewGame();
	// Hands the graphics and sound choices to the renderer and the mixer; with `save`, writes the file too.
	void ApplySettings(bool save);
	[[gnu::format(printf, 2, 3)]] void ShowStatus(const char* format, ...);
	void PushStatus(const std::string& text, StatusTone tone); // into the messages, on the level the player is on
	void ApplyWorldEvents(); // plays, shows and writes down what is in events; runs after input and each tick
};

// The one game state, created in main() before the window and destroyed after the main loop.
void CreateGame();
void DestroyGame();
GameState& Game();

#endif
