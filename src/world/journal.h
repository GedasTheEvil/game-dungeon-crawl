#ifndef JOURNAL_H
#define JOURNAL_H

#include <istream>
#include <ostream>
#include <string>
#include <vector>

// The archaeologist's notebook (docs/plan/journal-sections.draft.md): what the player went through, kept per save
// game. The riddles section so far.

// A riddle met at a gate. A copy, not an index into riddles/*.txt: those files can change between saves.
struct JournalRiddle {
	std::string theme;
	std::vector<std::string> question; // one entry per Q: line
	std::vector<std::string> answers;  // normalized (NormalizeAnswer)
	std::string hint;
	int level = 0;			// campaign level of the gate
	int lateXP = 0;			// a late answer from the journal; fixed when the gate is met
	bool hintShown = false; // the hint is written down only once it was shown
	bool solved = false;
	bool solvedLate = false; // answered from the journal, not at the gate
	std::string written;	 // the answer as the player typed it

	[[nodiscard]] bool SameRiddle(const JournalRiddle& other) const {
		return theme == other.theme && question == other.question;
	}
};

// One tenth of the gate's reward (riddleXP), at least 50.
[[nodiscard]] int lateRiddleXP(int gateXP);

class Journal {
  public:
	// A riddle asked at a gate. A riddle met again keeps its entry (the late XP and the level of the first time).
	// Returns its index in Riddles().
	size_t MeetRiddle(const JournalRiddle& riddle);
	void ShowHint(size_t index) { riddles.at(index).hintShown = true; }
	void Solve(size_t index, bool late, const std::string& typed);
	[[nodiscard]] const std::vector<JournalRiddle>& Riddles() const { return riddles; }
	void Clear() { riddles.clear(); }

	// Saved after the dungeon. Saves from before the journal end before the tag: the journal stays empty.
	void Dump(std::ostream& out) const;
	void Load(std::istream& in);

  private:
	std::vector<JournalRiddle> riddles;
};

#endif
