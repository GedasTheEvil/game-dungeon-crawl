#ifndef NAMES_H
#define NAMES_H

// The royal names that seal the name gates (plan secret-name-gates): content/names/*.txt (docs/names.md), loaded at
// start-up like the riddles. Each run gives every campaign level its own name and the picture of its name cartouche.
// Shared by the game and the tests, so no GL here.

#include <cstdint>
#include <istream>
#include <string>
#include <vector>

// Lower case letters and digits only, a leading "a " / "an " / "the " dropped: "The Force!" -> "force". The riddle
// answers are compared the same way.
[[nodiscard]] std::string NormalizeAnswer(const std::string& text);
constexpr size_t MAX_ANSWER_CHARS = 32; // what the scroll lets the player type

struct RoyalName {
	std::string name; // as the journal writes it: "Ramesses"
	std::string title = "king"; // "The name of the <title>: RAMESSES"
	std::vector<std::string> answers; // normalized, see NormalizeAnswer
	std::string note; // the archaeologist's comment, optional
	std::string source; // "file:line" of the name: line, for messages

	// Typed at the gate: compared like a riddle's answer.
	[[nodiscard]] bool Accepts(const std::string& typed) const;
};

// Appends the names of one file to out; problems go to errors as "file:line: message".
void ParseNames(
	std::istream& in,
	const std::string& source,
	std::vector<RoyalName>& out,
	std::vector<std::string>& errors
);

class NameList {
  public:
	// Every *.txt in a directory, or one file. Returns the problems found.
	std::vector<std::string> Load(const std::string& path);
	void Add(const RoyalName& name) { names.push_back(name); }
	[[nodiscard]] size_t Size() const { return names.size(); }
	[[nodiscard]] const RoyalName& At(size_t i) const { return names.at(i); }
	// The name of a campaign level in the run of this seed: the list shuffled by the seed, level 1 the first. No two
	// levels up to Size() share a name. Null for an empty list.
	[[nodiscard]] const RoyalName* ForLevel(uint32_t seed, int level) const;

  private:
	std::vector<RoyalName> names;
};

// The pictures of the name cartouches, media/textures/decorations/name_cartouches.png (tools/textures/inscriptions.py):
// NAME_CARTOUCHE_GRID x NAME_CARTOUCHE_GRID royal-looking sign groups, any picture for any name (they do not spell it).
constexpr int NAME_CARTOUCHE_GRID = 4;
constexpr int NAME_CARTOUCHE_PICTURES = NAME_CARTOUCHE_GRID * NAME_CARTOUCHE_GRID;
// The picture of a level's name cartouche in the run of this seed, 0 .. NAME_CARTOUCHE_PICTURES - 1.
[[nodiscard]] int nameCartouchePicture(uint32_t seed, int level);

#endif
