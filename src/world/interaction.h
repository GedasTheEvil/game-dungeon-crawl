#ifndef INTERACTION_H
#define INTERACTION_H

#include <cstdint>

// What the interact key does in the player's cell (plan interact-prompt): Dungeon::InteractVerb decides it,
// Dungeon::Interact does it, the HUD prompt shows it ("[E] Open").
enum class Interaction : std::uint8_t {
	None,
	Open, // a chest, or a mimic lying in ambush (the prompt never tells them apart)
	Pull, // a lever not pulled yet
	Read, // an inscription, with the hieroglyph skill (or too worn to read)
	Copy, // an inscription, without the skill: the glyphs go into the journal
	Light, // a brazier, with a bow: fire arrows
	Answer, // a riddle gate
	Enter, // the exit, a teleporter with a partner
	Take, // the ankh
};

// The prompt's verb: "Open", "Pull", ...; empty for None.
constexpr const char* interactionVerb(Interaction verb) {
	switch (verb) {
	case Interaction::None:
		return "";
	case Interaction::Open:
		return "Open";
	case Interaction::Pull:
		return "Pull";
	case Interaction::Read:
		return "Read";
	case Interaction::Copy:
		return "Copy";
	case Interaction::Light:
		return "Light arrows";
	case Interaction::Answer:
		return "Answer";
	case Interaction::Enter:
		return "Enter";
	case Interaction::Take:
		return "Take";
	}
	return "";
}

#endif
