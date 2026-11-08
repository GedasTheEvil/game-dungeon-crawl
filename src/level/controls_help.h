#ifndef CONTROLS_HELP_H
#define CONTROLS_HELP_H

#include "bindings.h"
#include <cstdint>
#include <string>
#include <vector>

// The How to Play page and the first hints (plan controls-help). GL-free: the menu draws the page, the game loop
// shows the hints, the unit tests check both against the bindings.

// "A / Left", "Space / RMB": an action's bound keys and mouse button as key caps; "-" when nothing is bound.
[[nodiscard]] std::string bindingCaps(const Binding& binding);

// One line of the How to Play page: what to do and the keys that do it.
struct HelpRow {
	std::string label;
	std::string keys;
};

struct HelpGroup {
	const char* title;
	std::vector<HelpRow> rows;
};

// Move, Fight, Items, Screens, Saves: every BindAction once, read from the bindings, with the fixed keys and the
// power hit's charge.
[[nodiscard]] std::vector<HelpGroup> controlsHelp(const Bindings& bindings);

// A first hint (plan controls-help): a status line the first time a thing is possible in a run, naming the bound key.
// In the order the bit sits in RunStats::hints.
enum class FirstHint : std::uint8_t {
	Jump, // a gap to jump across or a ledge to jump onto ahead
	Climb, // at a ladder
	Attack, // a monster comes for the player
	Sprint, // a few seconds of walking
	PowerHit, // the power hit learnt
	QuickHeal, // the first potion of each quick key in the bag
	QuickStamina,
	QuickAntidote,
};
constexpr int FIRST_HINT_COUNT = static_cast<int>(FirstHint::QuickAntidote) + 1;

// "Jump: Space or RMB". The key part follows the bindings; an action with nothing bound names the Options.
[[nodiscard]] std::string hintText(FirstHint hint, const Bindings& bindings);

// The run's shown hints, a bit per FirstHint (RunStats::hints).
[[nodiscard]] constexpr std::uint32_t hintBit(FirstHint hint) { return 1u << static_cast<unsigned>(hint); }

// The first hints' pace: each once a run (shown: RunStats::hints), one at a time, the next HINT_GAP_MS after the last.
struct HintPacer {
	static constexpr int HINT_GAP_MS = 7000; // after a hint, the next waits this long: each read
	int lastHintMs = -HINT_GAP_MS;

	// True while the hint may still show: not shown and not done yet. done: the player already did it, so it is
	// marked shown without a word.
	[[nodiscard]] static bool Pending(std::uint32_t& shown, FirstHint hint, bool done);
	// True when it shows now: possible and no other hint showed lately; marks it shown.
	[[nodiscard]] bool Offer(std::uint32_t& shown, FirstHint hint, bool possible, int now);
};

#endif
