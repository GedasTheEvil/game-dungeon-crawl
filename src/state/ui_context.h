#ifndef UI_CONTEXT_H
#define UI_CONTEXT_H

#include "../ui/inventory.h"
#include "../ui/ui_screen.h"
#include "../ui/screen.h"
#include "../ui/riddle.h"
#include "../ui/menu.h"
#include "../ui/win_entry.h"
#include "../ui/death_screen.h"
#include "../ui/ending_screen.h"
#include "../ui/map_view.h"
#include "../ui/journal_view.h"
#include "../ui/zone_banner.h"
#include "../ui/skills_view.h"
#include "../ui/screen_tabs.h"
#include "../level/ui/ui_layout.h"
#include "../ui/screen_world.h"
#include <memory>

// The screens over the game and what shows on top of it (GameState::ui; plan game-state-split).
struct UIContext {
	explicit UIContext(const ScreenWorld& world)
		: world(world), menu(this->world, *this), winEntry(this->world, *this), map(this->world, *this),
		  journal(this->world, *this), skills(this->world, *this), death(this->world, *this),
		  ending(this->world, *this) {}

	ScreenWorld world; // the game the screens show and change
	Screen screen = Screen::Menu; // the game starts in the main menu
	std::unique_ptr<Inventory> inventory;
	std::unique_ptr<Riddle> riddle;
	MainMenu menu;
	WinEntry winEntry; // the won run: its name for the leaderboard, the epilogue, then the Hall of Records
	DraftMap map;
	JournalScreen journal;
	SkillsScreen skills;
	ScreenTabsState tabs;
	DeathScreen death; // after the death clip: Retry from Autosave or Main Menu (plan autosave)
	EndingScreen ending; // the ankh's choice: life without end or the gods renounced (plan ending-choice)
	ui::Toast autosaved; // "Autosaved" over the game
	int bannerLevel = 0; // the campaign level whose banner shows (plan zone-looks, ZoneBanner), since bannerStartMs
	int bannerStartMs = 0;
	float bannerTop = ZoneBanner::TOP; // under the status lines; it only moves down while it shows (plan message-log)
	// The open tab screen (inventory, map, journal, skills; plan screen-interface), nullptr on any other.
	[[nodiscard]] UiScreen* TabScreen();
};

#endif
