#ifndef INSCRIPTIONS_H
#define INSCRIPTIONS_H

// Wall inscriptions the player can read (plan hieroglyph-cartouches): an Inscription tile's attribute is a text id
// into content/inscriptions/*.txt (docs/inscriptions.md). Read once, the translation goes into the journal. Shared by
// the game and levelcheck, so no GL here.

#include <istream>
#include <string>
#include <vector>

// A name cartouche (plan secret-name-gates): an Inscription whose attribute is NAME_CARTOUCHE_ID. It holds the
// level's name for its name gate (names.h), not a text of content/inscriptions/; its signs spell the name.
constexpr int NAME_CARTOUCHE_ID = 100;
// Ids run from 1 to INSCRIPTION_MAX; the name cartouche's id is no text id.
constexpr int INSCRIPTION_MAX = 256;
[[nodiscard]] constexpr bool isInscriptionId(int id) {
	return id >= 1 && id <= INSCRIPTION_MAX && id != NAME_CARTOUCHE_ID;
}

// A permanent stat a few inscriptions give the first time they are translated (plan inscription-boons). Life and
// Stamina are percents of the max.
enum class BoonKind : unsigned char { None, Might, Armor, Life, Stamina };
constexpr int BOON_KIND_COUNT = 5;
struct Boon {
	BoonKind kind = BoonKind::None;
	int amount = 0;
	[[nodiscard]] bool Any() const { return kind != BoonKind::None; }
	bool operator==(const Boon&) const = default;
};
// "Might +1", "Max health +10%"; empty for none.
[[nodiscard]] std::string boonText(const Boon& boon);

// The frame on the wall (style: line), a plate each (inscription_layout.h). The order of the plates.
enum class InscriptionStyle : unsigned char { Cartouche, Column, Row, Stela };
constexpr int INSCRIPTION_STYLE_COUNT = 4;

struct InscriptionText {
	int id = 0;
	std::string title; // the journal page's heading
	std::vector<std::string> text; // the translation, one entry per T: line
	std::string note; // the archaeologist's comment: the status line, and under the translation
	Boon boon; // the boon: line, none for most
	std::string source; // file:line, for messages
	InscriptionStyle style = InscriptionStyle::Cartouche;
	std::vector<std::string> glyphs; // the signs by Gardiner code, the G: line (plan composed-inscriptions)
	std::string glyphSource; // file:line of the G: line
};

// Reads one file's entries into out; problems go to errors as "source:line: message". Whether the G: codes are in the
// hieroglyph sheet is checkSigns' part (inscription_layout.h).
void ParseInscriptions(
	std::istream& in,
	const std::string& source,
	std::vector<InscriptionText>& out,
	std::vector<std::string>& errors
);

class InscriptionBook {
  public:
	// Every *.txt in a directory, or one file. Returns the problems found (a duplicate id keeps the first one).
	std::vector<std::string> Load(const std::string& path);
	void Add(const InscriptionText& inscription) { entries.push_back(inscription); }
	// Null for an id with no text.
	[[nodiscard]] const InscriptionText* Find(int id) const;
	[[nodiscard]] size_t Size() const { return entries.size(); }
	[[nodiscard]] const std::vector<InscriptionText>& All() const { return entries; }

  private:
	std::vector<InscriptionText> entries;
};

#endif
