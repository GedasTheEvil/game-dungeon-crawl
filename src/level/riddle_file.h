#ifndef RIDDLE_FILE_H
#define RIDDLE_FILE_H
#include "journal.h"
#include <cstddef>
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
	// The file's "zone:" line (plan weighing-of-the-heart): a ZoneInfo slug, dealt only on that zone's levels; empty:
	// the common deck, dealt everywhere.
	std::string zone;
};

// Appends the riddles of one file to `out`; problems go to `errors` as "file:line: message".
void ParseRiddles(
	std::istream& in,
	const std::string& source,
	std::vector<RiddleEntry>& out,
	std::vector<std::string>& errors
);

// The riddles of `zone` (a ZoneInfo slug) not met yet (`met`: the journal's), by index into `riddles`; empty for no
// zone (plan weighing-of-the-heart). A zone level deals one of these while any is left, then from the common deck.
[[nodiscard]] std::vector<size_t> zoneRiddlesLeft(
	const std::vector<RiddleEntry>& riddles,
	const std::string& zone,
	const std::vector<JournalRiddle>& met
);

#endif
