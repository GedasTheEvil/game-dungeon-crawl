#ifndef SAVE_SLOTS_H
#define SAVE_SLOTS_H

// The six save game slots on disk: saves/save<N>.sav, plus the list of slot names in saves/gamelist.dat (one line
// per slot, "<level>_<month>-<day>_<hh:mm>"). The menu draws them; GameState::Save / LoadSave write and read a game.
// The autosave (plan autosave) is no slot: the game writes it (GameState::Autosave), the player only loads it.

#include <array>
#include <string>

class SaveSlots {
  public:
	static constexpr int COUNT = 6;

	struct Info {
		bool used = false;
		int level = 0; // campaign level the save was made on, 0 if unknown
		std::string when; // date and time of the save file
	};

	static constexpr const char* AUTOSAVE = "saves/autosave.sav"; // a scenario's goes to its output directory

	[[nodiscard]] static std::string FileName(int slot);
	// The autosave at `path`: its level is the save's first number.
	[[nodiscard]] static Info DescribeAutosave(const std::string& path);
	void LoadNames(); // from saves/gamelist.dat; false-free: a missing list leaves the names empty
	void Record(int slot, int level); // a game was just saved into the slot on `level`: name it, write the list
	[[nodiscard]] Info Describe(int slot) const;

  private:
	std::array<std::string, COUNT> names;
	void writeNames() const;
};

#endif
