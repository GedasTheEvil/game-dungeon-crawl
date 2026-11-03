#ifndef STATUS_LIST_H
#define STATUS_LIST_H

#include "hud_icon.h"
#include "../world/curses.h"
#include "../world/poison.h"
#include "../world/rgb.h"
#include <array>

class PlayerStats;

// The status cards over the player HUD (plan curse-blessing-cards): every running effect is one card, a painted
// background by kind (media/textures/ui/status_cards.png, tools/textures/status_cards.py, one cell per kind in enum
// order) with the effect's icon on it.
//
// The kind follows what the effect does to the player:
// * Blessing, yellow-gold: any good effect. Hapi, the Protection of Isis, the resistance potion.
// * Condition, neutral grey: a bad effect that is easy to end (an antidote, water, a short wait). Poison, burning,
// daze.
// * Curse, purple: a bad effect that is almost incurable (only a prayer at a Shrine of Isis lifts it).
// A new effect: a cell in hud_icons.py, an Icon entry, one line in statusList().
enum class StatusKind : std::uint8_t { Blessing, Condition, Curse };
constexpr int STATUS_KIND_COUNT = 3;

struct Status {
	StatusKind kind = StatusKind::Condition;
	PlayerHud::Icon icon = PlayerHud::Icon::None;
	Rgb tint = {1, 1, 1}; // the icon's tint (a white drop takes the potion's colour)
	int pips = 0; // 0: none; the tier or the grade
	int leftMs = 0; // the seconds shown under the card
};

// Fixed size, no allocation per frame: all the blessings, poison tiers, burning, daze and curses at once.
constexpr int MAX_STATUSES = 3 + POISON_TIER_COUNT + 2 + CURSE_COUNT;
struct StatusList {
	std::array<Status, MAX_STATUSES> items{};
	int count = 0;
	void Add(const Status& s) {
		if (count < MAX_STATUSES)
			items[static_cast<size_t>(count++)] = s;
	}
};

// The running effects in display order: blessings (resistance, Hapi, Isis), then conditions (poison weak first,
// burning, daze), then curses in Curse order, so the row reads by colour. burningLeftMs: Player::burning, dazeMs:
// Player::dazeMs.
[[nodiscard]] StatusList statusList(const PlayerStats& stats, int burningLeftMs, int dazeMs);

#endif
