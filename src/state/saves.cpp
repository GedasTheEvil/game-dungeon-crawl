#include "saves.h"
#include "game_state.h"
#include "ui_context.h"
#include "../core/logger.h"
#include "../level/campaign.h"
#include "../sim/dungeon_save.h"
#include "../ui/player_hud.h"
#include <sstream>

bool SaveService::Write(const std::string& path) {
	if (!game.cacheLoaded) {
		LOG_ERROR("game", "can't save without loading cashe");
		return false;
	}
	std::ostringstream text;
	UiViewState view = game.ViewState();
	writeSaveGame(text, SaveParts{game.dungeon, *game.player, game.bag, game.journal, view});
	if (!writeFileSafely(path, text.str())) {
		LOG_ERRORF("game", "can't write save file %s", path.c_str());
		return false;
	}
	savedPlayMs = game.player->stats.run.PlayMs();
	return true;
}

// Read and checked before the game changes: a missing or broken file leaves the game as it was (plan
// save-load-safety).
bool SaveService::Read(const std::string& path) {
	if (!game.cacheLoaded) {
		LOG_ERROR("game", "can't load without loading cashe");
		return false;
	}
	LOG_INFOF("game", "Loading save %s", path.c_str());
	std::string text;
	if (!readWholeFile(path, text)) {
		LOG_ERRORF("game", "can't open save file %s", path.c_str());
		return false;
	}
	UiViewState view = game.ViewState(); // a broken save puts these back too
	const bool loaded = loadSaveGame(text, SaveParts{game.dungeon, *game.player, game.bag, game.journal, view});
	game.ui.inventory->BagLoaded();
	game.ApplyViewState(view);
	if (!loaded) {
		LOG_ERRORF("game", "broken save file %s", path.c_str());
		return false;
	}
	game.messages.Clear(); // the log is not saved
	game.dungeon.ClearWin(); // a save is never of a won game
	const int levelNumber = game.dungeon.LevelNumber();
	game.dungeon.scatterDecorations(campaignLevelFile(levelNumber).c_str(), levelNumber);
	LOG_INFOF("game", "Loaded level %d", levelNumber);
	PlayerHud::reset();
	savedPlayMs = game.player->stats.run.PlayMs();
	return true;
}

bool SaveService::LoadIntoGame(const std::string& path) {
	if (!Read(path))
		return false;
	game.ui.screen = Screen::Gameplay;
	game.ui.menu.ResetSubScreens();
	game.ui.menu.inGame = true;
	return true;
}

bool SaveService::LoadLatest() {
	const auto latest = folder.Latest();
	return latest && LoadIntoGame(latest->path);
}

bool SaveService::UnsavedProgress() const {
	constexpr int UNSAVED_PLAY_MS = 3000; // a look around after a load is no progress
	return game.player->stats.run.PlayMs() - savedPlayMs >= UNSAVED_PLAY_MS;
}

void SaveService::Autosave(AutosaveMoment moment) {
	if (!game.player->Alive() || game.dungeon.Won() ||
		(moment != AutosaveMoment::LevelStart && game.dungeon.PlayerInDanger()))
		return;
	game.ui.autosaved.Show(Write(folder.Autosave()) ? "Autosaved" : "Autosave failed", GameClock::now());
}

void SaveService::Quicksave() {
	if (!game.player->Alive() || game.dungeon.Won()) // a save is never of a won game (plan win-ending-flow)
		return;
	game.ui.autosaved.Show(Write(folder.Quicksave()) ? "Quick saved" : "Quick save failed", GameClock::now());
}

void SaveService::LoadQuicksave() {
	if (!folder.HasQuicksave()) {
		game.ui.autosaved.Show("No quick save", GameClock::now());
		return;
	}
	game.ui.autosaved.Show(
		LoadIntoGame(folder.Quicksave()) ? "Quick loaded" : "Could not load the quick save",
		GameClock::now()
	);
}

// A load puts the saved count back, so the file learns the death now: a retry, or Continue after a restart, keeps it.
void SaveService::CountDeath() { SaveSlots::SetFileDeaths(folder.Autosave(), ++game.player->stats.run.deaths); }
