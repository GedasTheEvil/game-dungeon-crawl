#ifndef SAVE_SLOTS_H
#define SAVE_SLOTS_H

// The six save game slots on disk: saves/save<N>.sav, plus the list of slot names in saves/gamelist.dat (one line
// per slot, "<level>_<month>-<day>_<hh:mm>"). The menu draws them; GameState::Save / LoadSave write and read a game.
// The autosave (plan autosave) is no slot: the game writes it (GameState::Autosave), the player only loads it.
// Nor is the quick save (plan quick-save): F5 writes it, F9 loads it.

#include <array>
#include <optional>
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
	static constexpr const char* QUICKSAVE = "saves/quicksave.sav"; // a scenario's goes to its output directory

	// The save file at `path` (the autosave, the quick save): its level is the save's first number.
	[[nodiscard]] static Info DescribeFile(const std::string& path);

	// The newest save (plan retry-latest-save): where it is and what it is, for the death screen's Retry and Continue.
	struct Latest {
		enum class Kind : unsigned char { Autosave, Slot, Quicksave };
		Kind kind = Kind::Autosave;
		int slot = 0; // from 0, for Kind::Slot
		std::string path;
		[[nodiscard]] std::string Label() const; // "Autosave", "Slot 3", "Quick Save"
	};
	// The newest of the autosave, the slots' files and the quick save by file time; a missing file does not count.
	// A tie goes to the later in that order. None when no file exists.
	[[nodiscard]] static std::optional<Latest>
	PickLatest(const std::string& autosave, const std::array<std::string, COUNT>& slots, const std::string& quicksave);
	void LoadNames(); // from saves/gamelist.dat; false-free: a missing list leaves the names empty
	void Record(int slot, int level); // a game was just saved into the slot on `level`: name it, write the list
	[[nodiscard]] Info Describe(int slot) const;

  private:
	std::array<std::string, COUNT> names;
	void writeNames() const;
};

#endif
