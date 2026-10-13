#ifndef WIN_ENTRY_H
#define WIN_ENTRY_H

#include "../graphics/font.h"
#include <cstdint>
#include <string>

/// @file win_entry.h
/// The win's leaderboard entry (plan leaderboard): over the won game, a run summary and a name field. Enter records
/// the run and opens the Hall of Records with it marked; Esc skips it (no record). Only a campaign run (New Game on
/// level 1) gets it.

class WinEntry {
  public:
	static constexpr size_t MAX_NAME = 20;

	// Each update: opens when a campaign run is won (the name field holds the last name used), resets when the game
	// is no longer won (a new game, a load).
	void Follow();
	[[nodiscard]] bool Typing() const { return state == State::Typing; }
	void KeyPressed(unsigned char key);
	void Draw(int resX, int resY);

  private:
	enum class State : std::uint8_t { Hidden, Typing, Done };
	State state = State::Hidden;
	std::string name;
	bool fontsLoaded = false;
	Font title, heading, body, small;

	void record();
};

#endif
