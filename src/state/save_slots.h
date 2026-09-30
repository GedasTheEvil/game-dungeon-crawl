#ifndef SAVE_SLOTS_H
#define SAVE_SLOTS_H

// The six save game slots on disk: saves/save<N>.sav, plus the list of slot names in saves/gamelist.dat (one line
// per slot, "<level>_<month>-<day>_<hh:mm>"). The menu draws them; GameState::Save / LoadSave write and read a game.

#include <array>
#include <string>

class SaveSlots {
  public:
	static constexpr int COUNT = 6;

	struct Info {
		bool used = false;
		int level = 0;	  // campaign level the save was made on, 0 if unknown
		std::string when; // date and time of the save file
	};

	[[nodiscard]] static std::string FileName(int slot);
	void LoadNames();				  // from saves/gamelist.dat; false-free: a missing list leaves the names empty
	void Record(int slot, int level); // a game was just saved into the slot on `level`: name it, write the list
	[[nodiscard]] Info Describe(int slot) const;

  private:
	std::array<std::string, COUNT> names;
	void writeNames() const;
};

#endif
