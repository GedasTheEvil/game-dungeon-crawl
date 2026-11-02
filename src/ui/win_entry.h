#ifndef WIN_ENTRY_H
#define WIN_ENTRY_H

#include "../graphics/font.h"
#include "confirm_box.h"
#include <cstdint>
#include <string>

/// @file win_entry.h
/// The won run's end (plans leaderboard, win-ending-flow). A campaign run (New Game on level 1) first gets the name
/// entry over the dimmed game: a run summary and a name field. Enter records the run; Esc asks before it leaves the
/// run out of the Hall of Records. Then the epilogue opens in the journal; closing it opens the Hall of Records with
/// the new row marked (a run off the campaign: the main menu). No saves and no way back into the game meanwhile.

class WinEntry {
  public:
	static constexpr size_t MAX_NAME = 20;

	// Each update: opens when the game is won (the name field holds the last name used), goes on once the epilogue's
	// journal is closed, resets when the game is no longer won (a new game, a load).
	void Follow();
	[[nodiscard]] bool Typing() const { return state == State::Typing; }
	void KeyPressed(unsigned char key);
	void SpecialKeyPressed(int key); // the confirm box's arrows
	void Draw(int resX, int resY);

  private:
	enum class State : std::uint8_t { Hidden, Typing, Epilogue, Done };
	State state = State::Hidden;
	std::string name;
	int marked = -1; // the recorded run's row in the Hall of Records (RecordsView::NONE: none)
	ConfirmBox confirm; // "Leave without entering the Hall of Records?"
	bool fontsLoaded = false;
	Font title, heading, body, small;

	void record();
	void openEpilogue(int row);
	void finish();
	void drawPanel(const ui::Rect& area); // the summary and the name field
};

#endif
