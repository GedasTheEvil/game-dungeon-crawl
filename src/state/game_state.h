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
#include "../ui/win_entry.h"
#include "../ui/death_screen.h"
#include "../ui/ui_layout.h"
#include "../ui/map_view.h"
#include "../ui/journal_view.h"
#include "../ui/skills_view.h"
#include "../ui/screen_tabs.h"
#include "../world/inscriptions.h"
#include "../world/names.h"
#include "../world/journal.h"
#include "../world/leaderboard.h"
#include "../world/world_events.h"
#include "save_slots.h"
#include <optional>
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
	int resX = 800; // the window's size now; Settings::display holds the one it opens with

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
	WinEntry winEntry; // the won run's name for the leaderboard
	DraftMap map;
	JournalScreen journal;
	SkillsScreen skills;
	ScreenTabsState tabs;
	DeathScreen death; // after the death clip: Retry from Autosave or Main Menu (plan autosave)
	ui::Toast autosaved; // "Autosaved" over the game
	int bannerLevel = 0; // the campaign level whose banner shows (plan zone-looks, ZoneBanner), since bannerStartMs
	int bannerStartMs = 0;
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
	Journal journal; // what the archaeologist wrote down, kept per save game
	InscriptionBook inscriptions; // the wall inscriptions' texts, content/inscriptions/*.txt
	NameList names; // the royal names of the name gates, content/names/*.txt (plan secret-name-gates)
	WorldEvents events; // what the world and the player told the app since the last ApplyWorldEvents
	SaveSlots saves;
	Leaderboard leaderboard; // the won runs, saves/leaderboard.txt (plan leaderboard)

	GameState();
	~GameState();
	void Load();
	void DrawLoad(float xxx, const char step[], const char detail[]);
	void Save(const char filename[]);
	void LoadSave(const char filename[]);
	void NewGame();
	// The autosave (plan autosave): SaveSlots::AUTOSAVE, a scenario's in its output directory.
	[[nodiscard]] std::string AutosavePath() const;
	[[nodiscard]] bool HasAutosave() const;
	// A safe moment: writes the autosave and shows "Autosaved". Skipped (the old one stays) while the player is dead or
	// in danger (Dungeon::PlayerInDanger); a level start is always written.
	void Autosave(AutosaveMoment moment);
	void LoadAutosave(); // into the game: the screen is the gameplay
	void LoadSaveIntoGame(const std::string& path); // what loading a save file shows: the gameplay, no menu page
	// The quick save (plan quick-save): SaveSlots::QUICKSAVE, a scenario's in its output directory.
	[[nodiscard]] std::string QuicksavePath() const;
	[[nodiscard]] bool HasQuicksave() const;
	void Quicksave(); // F5: writes it and shows "Quick saved"; nothing while the player is dead
	void LoadQuicksave(); // F9: into the game with "Quick loaded", or the toast "No quick save"
	// The slot's file (plan retry-latest-save): SaveSlots::FileName, a scenario's in its output directory.
	[[nodiscard]] std::string SlotPath(int slot) const;
	// The newest of the autosave, the slots and the quick save (plan retry-latest-save): Retry and Continue load it.
	[[nodiscard]] std::optional<SaveSlots::Latest> LatestSave() const;
	void LoadLatestSave(); // into the game, nothing without a save
	void CountDeath(); // the player just died: one more death, in the autosave's file too
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
