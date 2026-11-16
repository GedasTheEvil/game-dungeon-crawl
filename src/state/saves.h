#ifndef SAVES_H
#define SAVES_H

#include "../level/save_slots.h"
#include "../level/world_events.h"
#include <string>

class GameState;

// Saving and loading the game (plan game-state-split): the files (SaveFolder), the autosave, the quick save, the
// latest save and the death count in the autosave. The file's parts: writeSaveGame / loadSaveGame (sim/dungeon_save.h).
class SaveService {
  public:
	explicit SaveService(GameState& game) : game(game) {}

	SaveFolder folder;

	// False when the file cannot be written (the old one stays) or read; a failed read leaves the game as it was
	// (plan save-load-safety).
	[[nodiscard]] bool Write(const std::string& path);
	[[nodiscard]] bool Read(const std::string& path);
	// What loading a save file shows: the gameplay, no menu page. False if it cannot be loaded: nothing changes.
	bool LoadIntoGame(const std::string& path);
	bool LoadLatest(); // into the game; false without a save or when it cannot be loaded
	// Play time since the last save or load (plan save-load-safety): leaving the run then asks first.
	[[nodiscard]] bool UnsavedProgress() const;
	// A safe moment (plan autosave): writes the autosave and shows "Autosaved". Skipped (the old one stays) while the
	// player is dead or in danger (Dungeon::PlayerInDanger); a level start is always written.
	void Autosave(AutosaveMoment moment);
	void Quicksave(); // F5 (plan quick-save): writes it and shows "Quick saved"; nothing while the player is dead
	void LoadQuicksave(); // F9: into the game with "Quick loaded", or the toast "No quick save"
	void CountDeath(); // the player just died: one more death, in the autosave's file too

  private:
	GameState& game;
	int savedPlayMs = 0; // the run's play time at the last save or load (UnsavedProgress)
};

#endif
