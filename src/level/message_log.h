#ifndef MESSAGE_LOG_H
#define MESSAGE_LOG_H

#include "../world/world_events.h"
#include <deque>
#include <string>
#include <vector>

// A gameplay status line (plan message-log): what it says, its tone, when it came and how long it shows.
struct StatusMessage {
	std::string text; // without the repeat count
	StatusTone tone = StatusTone::Plain;
	int bornMs = 0; // GameClock time it first showed: it fades in from here
	int shownMs = 0; // ...and it was last told: it shows LifeMs() from here
	int repeats = 1;

	// Plain lines show 3 s, good news and dangers 6 s.
	[[nodiscard]] int LifeMs() const;
	[[nodiscard]] int EndMs() const { return shownMs + LifeMs(); }
	// The text with the repeats: "Your feet stick in the web (x3)".
	[[nodiscard]] std::string Shown() const;
};

// The status lines over the running game and this level's log of them (plan message-log). Up to VISIBLE show at once,
// stacked, oldest at the top, each on its own timer. A line told again while it still shows merges into it and comes
// to the bottom: an XP gain adds up ("Gained 30 XP" twice is "Gained 60 XP"), any other line counts its repeats. The
// log keeps the last LOG_SIZE lines of the level the player is on; it is not saved.
class MessageLog {
  public:
	static constexpr int VISIBLE = 3;
	static constexpr size_t LOG_SIZE = 50;
	static constexpr int PLAIN_MS = 3000;
	static constexpr int TONED_MS = 6000;

	// At GameClock time now, on campaign level `level`: a line from another level starts the log afresh.
	void Push(const std::string& text, StatusTone tone, int now, int level);
	// What shows at now, oldest first: the newest VISIBLE lines whose time has not run out.
	[[nodiscard]] std::vector<const StatusMessage*> Visible(int now) const;
	// Oldest first.
	[[nodiscard]] const std::deque<StatusMessage>& Log() const { return log; }
	[[nodiscard]] int Level() const { return level; }
	void Clear() { log.clear(); }

  private:
	std::deque<StatusMessage> log;
	int level = 0;
};

#endif
