#ifndef RIDDLE_H
#define RIDDLE_H
#include "../graphics/font.h"
#include "../world/names.h"
#include <istream>
#include <string>
#include <vector>

// Riddles live in text files, one theme per file (content/riddles/*.txt). Format: docs/riddles.md.
struct RiddleEntry {
	std::string theme;
	std::vector<std::string> question; // one entry per Q: line, wrapped to the scroll when drawn
	std::vector<std::string> answers; // normalized, see NormalizeAnswer
	std::string hint;
	std::string source; // "file:line" of the first Q:, for log messages
};

// Appends the riddles of one file to `out`; problems go to `errors` as "file:line: message".
void ParseRiddles(
	std::istream& in,
	const std::string& source,
	std::vector<RiddleEntry>& out,
	std::vector<std::string>& errors
);

class Riddle {
  private:
	std::vector<RiddleEntry> riddles;
	std::vector<size_t> deck; // shuffled riddle indices still to be asked, so none repeats before all were seen
	size_t selected = 0; // last riddle dealt from the deck
	RiddleEntry shown; // the riddle on the scroll: from the deck, or a journal entry asked again
	size_t journalIndex = 0;
	bool late = false; // asked from the journal: Esc and a right answer go back to it
	bool nameGate = false; // a name gate's scroll (plan secret-name-gates): no reward, no hint, not in the journal
	int reward = 0;
	std::string answer;
	int misses = 0;
	int wrongAtMs = -100000;
	Font title, heading, body, small;

	[[nodiscard]] bool CheckAnswer() const;
	void DrawBackground();
	void DrawScroll();
	void DrawAnswer();

  public:
	Riddle();
	// Replaces the riddles with the ones from a file, or from every *.txt of a directory. Returns the count.
	size_t Load(const std::string& path);
	void Ask(); // the next riddle of the shuffled deck, written down in the journal
	void AskLate(size_t journalIndex); // a journal riddle not solved yet, for its late XP
	// A name gate asks for the level's name (plan secret-name-gates); the right one opens it (Dungeon::SpeakName).
	void AskName(const RoyalName& name);
	void Draw();
	void KeyboardF(unsigned char key, int x, int y);
};

#endif
