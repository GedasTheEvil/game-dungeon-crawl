#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "assets.h"
#include "../entities/player.h"
#include "../entities/player_view.h"
#include "../graphics/texture_registry.h"
#include "../graphics/animated_model.h"
#include "../world/dungeon.h"
#include "../graphics/font.h"
#include "../entities/item.h"
#include "../ui/inventory.h"
#include "../ui/screen.h"
#include "../world/rng.h"
#include "../core/sound.h"
#include "../entities/player_stats.h"
#include "../core/timer.h"
#include "../entities/trap.h"
#include "../ui/riddle.h"
#include "../ui/menu.h"
#include "../ui/end_screens.h"
#include "../ui/map_view.h"
#include "../ui/journal_view.h"
#include "../ui/screen_tabs.h"
#include "../world/journal.h"
#include "../world/world_events.h"
#include "save_slots.h"
#include "settings_ini.h"
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
	int resX = 800;		   // the window's size now; Settings::display holds the one it opens with

	int resY = 500;
};

struct GameTimers {
	Timer idleModel{300}; // back to the idle clip after walking
};

struct UIContext {
	Screen screen = Screen::Menu; // the game starts in the main menu
	std::unique_ptr<Inventory> inventory;
	std::unique_ptr<Riddle> riddle;
	MainMenu menu;
	std::unique_ptr<EndScreens> endScreens;
	DraftMap map;
	JournalScreen journal;
	ScreenTabsState tabs;
};

class GameState {
  public:
	Assets assets;
	Camera camera;
	RenderSettings render;
	Settings settings; // the Options choices (saves/settings.ini)
	bool cacheLoaded = false;
	std::string status; // the gameplay status message, shown for STATUS_MS after ShowStatus
	Timer statusTimer{STATUS_MS};
	std::unique_ptr<Player> player;
	PlayerView playerView; // the player's model, drawn
	GameTimers timers;
	GameRandom random;
	UIContext ui;
	Dungeon dungeon;
	Journal journal;	// what the archaeologist wrote down, kept per save game
	WorldEvents events; // what the world and the player told the app since the last ApplyWorldEvents
	SaveSlots saves;

	GameState();
	~GameState();
	void Load();
	void DrawLoad(float xxx, const char text[]);
	void Save(const char filename[]);
	void LoadSave(const char filename[]);
	void NewGame();
	// Hands the graphics and sound choices to the renderer and the mixer; with `save`, writes the file too.
	void ApplySettings(bool save);
	[[gnu::format(printf, 2, 3)]] void ShowStatus(const char* format, ...);
	void ApplyWorldEvents(); // plays, shows and writes down what is in events; runs after input and each tick
	static constexpr int STATUS_MS = 3000;
};

// The one game state, created in main() before the window and destroyed after the main loop.
void CreateGame();
void DestroyGame();
GameState& Game();

#endif
