#ifndef RIDDLE_H
#define RIDDLE_H
#include "../render/font.h"
#include "../level/names.h"
#include "../level/riddle_file.h"
#include "../level/rng.h"
#include <string>
#include <vector>

class Riddle {
  private:
	std::vector<RiddleEntry> riddles;
	// Shuffled indices of the common riddles (no zone) still to be asked, so none repeats before all were seen.
	std::vector<size_t> deck;
	size_t selected = 0; // last riddle dealt from the deck
	RiddleEntry shown; // the riddle on the scroll: from the deck, or a journal entry asked again
	size_t journalIndex = 0;
	bool late = false; // asked from the journal: Esc and a right answer go back to it
	bool nameGate = false; // a name gate's scroll (plan secret-name-gates): no reward, no hint, not in the journal
	std::string knownName; // the name gate's name in capitals, shown with the Wisdom of Seshat (plan heka-surplus)
	int reward = 0;
	std::string answer;
	int misses = 0;
	int wrongAtMs = -100000;
	Font title, heading, body, small;

	[[nodiscard]] bool CheckAnswer() const;
	void shuffleDeck(Rng& rng); // refills `deck` with the common riddles
	void DrawBackground();
	void DrawScroll();
	void DrawAnswer();

  public:
	Riddle();
	// Replaces the riddles with the ones from a file, or from every *.txt of a directory. Returns the count.
	size_t Load(const std::string& path);
	// The next riddle, written down in the journal: on a zone level one of the zone's not met yet, else the next of
	// the shuffled common deck.
	void Ask();
	void AskLate(size_t journalIndex); // a journal riddle not solved yet, for its late XP
	// A name gate asks for the level's name (plan secret-name-gates); the right one opens it (Dungeon::SpeakName).
	void AskName(const RoyalName& name);
	void Draw();
	void KeyboardF(unsigned char key, int x, int y);
};

#endif
