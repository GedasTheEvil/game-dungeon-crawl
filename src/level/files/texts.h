#ifndef TEXTS_H
#define TEXTS_H

// The player-facing prose (plan texts-to-content): the field notes, the diary and expedition pages, the creature
// moves, the monster notes, the skill and item lore, in content/texts/*.txt (docs/texts.md), keyed by section and
// field ("note.poison.title"). Read on the first lookup, once: the game, the unit tests and the tools see the same
// table. No GL here.

#include <istream>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// "Serket's favour" -> "serkets-favour": lower case letters and digits, a dash for every run of others, an
// apostrophe dropped. The keys of the rows whose names live in code (monsters, items, skills).
[[nodiscard]] std::string textSlug(std::string_view name);

// Appends one file's "key, text" pairs to out; problems go to errors as "source:line: message".
void ParseTexts(
	std::istream& in,
	const std::string& source,
	std::vector<std::pair<std::string, std::string>>& out,
	std::vector<std::string>& errors
);

class TextBook {
  public:
	// Every *.txt in a directory, or one file. Returns the problems found (a duplicate key keeps the first one).
	std::vector<std::string> Load(const std::string& path);
	// Null for a key with no text.
	[[nodiscard]] const std::string* Find(std::string_view key) const;
	[[nodiscard]] size_t Size() const { return entries.size(); }

  private:
	std::map<std::string, std::string, std::less<>> entries; // a node map: the texts stay put, c_str() stays valid
};

// The table of content/texts/, loaded on the first call.
[[nodiscard]] const TextBook& gameTexts();
// The problems its load found; the game logs them.
[[nodiscard]] const std::vector<std::string>& textProblems();
// The text of a key. A missing key gives "[key]" and goes into missingTexts(): the unit tests ask for every key the
// code uses and fail on any missing one.
[[nodiscard]] const std::string& gameText(std::string_view key);
[[nodiscard]] std::vector<std::string> missingTexts();

#endif
