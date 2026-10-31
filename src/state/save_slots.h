#ifndef SAVE_SLOTS_H
#define SAVE_SLOTS_H

// The six save game slots on disk: saves/save<N>.sav. The menu draws them; GameState::Save / LoadSave write and read a
// game. The autosave (plan autosave) is no slot: the game writes it (GameState::Autosave), the player only loads it.
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
		int characterLevel = 0; // 0 if unknown (plan save-load-safety)
		int playMs = -1; // the run's play time, -1 for a save from before it was kept
	};

	// A save's first and last words (plan save-load-safety): a save without its end was cut. One from before them
	// starts with the level number.
	static constexpr const char* FILE_TAG = "SAVEGAME";
	static constexpr const char* END_TAG = "END";

	static constexpr const char* AUTOSAVE = "saves/autosave.sav"; // a scenario's goes to its output directory

	[[nodiscard]] static std::string FileName(int slot);
	static constexpr const char* QUICKSAVE = "saves/quicksave.sav"; // a scenario's goes to its output directory

	// The save file at `path`, from its head: the level is the save's first number, the character level the second,
	// the play time the run's (RunStats). Read again only when the file changes: the menu asks each frame.
	[[nodiscard]] static Info DescribeFile(const std::string& path);
	[[nodiscard]] static std::string PlayTime(int ms); // "1h 05m", "12m"

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
};

#endif
