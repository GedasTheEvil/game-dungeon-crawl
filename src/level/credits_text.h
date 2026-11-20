#ifndef CREDITS_TEXT_H
#define CREDITS_TEXT_H

// The credits that are not on the sheet (plan credits-second-section): content/credits (docs/ui.md#credits), read at
// the first frame of the credits screen. Shared by the game and the tests, so no GL here.

#include <istream>
#include <string>
#include <vector>

struct CreditsLine {
	std::vector<std::string> fields; // "Track | Author | Licence": the cells, trimmed
};

struct CreditsGroup {
	std::string heading; // "Music"
	std::vector<CreditsLine> lines;
};

// `# Heading` starts a group, every other non-empty line is a row whose cells are split at `|`; `//` starts a comment
// line. A row before the first heading gets an empty heading.
[[nodiscard]] std::vector<CreditsGroup> ParseCredits(std::istream& in);
// A missing file is no groups.
[[nodiscard]] std::vector<CreditsGroup> LoadCredits(const std::string& path);

#endif
