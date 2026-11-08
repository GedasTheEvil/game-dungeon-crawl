#ifndef STATUS_LIST_H
#define STATUS_LIST_H

#include "hud_icon.h"
#include "curses.h"
#include "poison.h"
#include "rgb.h"
#include "shrines.h"
#include "terrain.h"
#include "vitals.h"
#include <array>

class PlayerStats;

// The status cards over the player HUD (plan curse-blessing-cards): every running effect is one card, a painted
// background by kind (media/textures/ui/status_cards.png, tools/textures/status_cards.py, one cell per kind in enum
// order) with the effect's icon on it.
//
// The kind follows what the effect does to the player (or the monster, plan monster-status-icons):
// * Blessing, yellow-gold: any good effect. Hapi, the gods' blessings, the resistance potion, a monster's native
// terrain.
// * Condition, neutral grey: a bad effect that is easy to end (an antidote, water, a short wait). Poison, burning,
// daze or stun, a hostile terrain.
// * Curse, purple: a bad effect that is almost incurable (only a prayer at a Shrine of Isis lifts it).
// A new effect: a cell in hud_icons.py, an Icon entry, one line in statusList() (a condition both the player and a
// monster can have: in addConditions).
enum class StatusKind : std::uint8_t { Blessing, Condition, Curse };
constexpr int STATUS_KIND_COUNT = 3;

struct Status {
	StatusKind kind = StatusKind::Condition;
	PlayerHud::Icon icon = PlayerHud::Icon::None;
	Rgb tint = {1, 1, 1}; // the icon's tint (a white drop takes the potion's colour)
	int pips = 0; // 0: none; the tier or the grade
	int leftMs = 0; // the seconds shown under the card
};

// Fixed size, no allocation per frame: all the blessings, poison tiers, burning, daze, terrain and curses at once.
constexpr int MAX_STATUSES = 2 + GOD_COUNT + POISON_TIER_COUNT + 3 + CURSE_COUNT;
struct StatusList {
	std::array<Status, MAX_STATUSES> items{};
	int count = 0;
	void Add(const Status& s) {
		if (count < MAX_STATUSES)
			items[static_cast<size_t>(count++)] = s;
	}
};

// The conditions of a body, the player's or a monster's (plan monster-status-icons): the poison tiers weak first,
// burning, out of action (the player's daze, a monster's stun), then the terrain's mood (native: a blessing card,
// hostile: a condition card; the player's own affinity, PlayerStats::TerrainHostile). One place for the next condition
// both can have.
void addConditions(StatusList& list, const Vitals& vitals, TerrainMood mood = TerrainMood::Neutral);

// The player's running effects in display order: blessings (resistance, Hapi, the gods in God order), then the
// conditions (addConditions; mood: the player's terrain, Dungeon::PlayerTerrainMood, plan same-hazards-for-all),
// then curses in Curse order, so the row reads by colour.
[[nodiscard]] StatusList statusList(const PlayerStats& stats, TerrainMood mood = TerrainMood::Neutral);

// A monster's (plan monster-status-icons): its conditions only, no blessings or curses (it prays at no shrine).
[[nodiscard]] StatusList monsterStatuses(const Vitals& vitals, TerrainMood mood);

#endif
