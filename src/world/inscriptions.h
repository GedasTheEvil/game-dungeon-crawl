#ifndef INSCRIPTIONS_H
#define INSCRIPTIONS_H

// Wall inscriptions the player can read (plan hieroglyph-cartouches): an Inscription tile's attribute is a text id
// into inscriptions/*.txt (docs/inscriptions.md). Read once, the translation goes into the journal. Shared by the game
// and levelcheck, so no GL here.

#include <istream>
#include <string>
#include <vector>

// Ids run from 1 to INSCRIPTION_MAX: the atlas textures/decorations/inscriptions.png (tools/textures/inscriptions.py)
// has a cell per id, INSCRIPTION_ATLAS_GRID x INSCRIPTION_ATLAS_GRID, id 1 top left.
constexpr int INSCRIPTION_ATLAS_GRID = 8;
constexpr int INSCRIPTION_MAX = INSCRIPTION_ATLAS_GRID * INSCRIPTION_ATLAS_GRID;
[[nodiscard]] constexpr bool isInscriptionId(int id) { return id >= 1 && id <= INSCRIPTION_MAX; }
// The first campaign level with inscriptions: the hieroglyph skill (plan skill-hieroglyphs) comes with the first
// boss's heka, at the end of level 5. levelcheck warns about one before it.
constexpr int INSCRIPTIONS_FROM_LEVEL = 6;

struct InscriptionText {
	int id = 0;
	std::string title; // the journal page's heading
	std::vector<std::string> text; // the translation, one entry per T: line
	std::string note; // the archaeologist's comment: the status line, and under the translation
	std::string source; // file:line, for messages
};

// Reads one file's entries into out; problems go to errors as "source:line: message". The game skips the G: and
// style: lines (the atlas tool reads them).
void ParseInscriptions(std::istream& in, const std::string& source, std::vector<InscriptionText>& out,
					   std::vector<std::string>& errors);

class InscriptionBook {
  public:
	// Every *.txt in a directory, or one file. Returns the problems found (a duplicate id keeps the first one).
	std::vector<std::string> Load(const std::string& path);
	void Add(const InscriptionText& inscription) { entries.push_back(inscription); }
	// Null for an id with no text.
	[[nodiscard]] const InscriptionText* Find(int id) const;
	[[nodiscard]] size_t Size() const { return entries.size(); }

  private:
	std::vector<InscriptionText> entries;
};

#endif
