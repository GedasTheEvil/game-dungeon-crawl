#ifndef PLAYER_ACTIONS_H
#define PLAYER_ACTIONS_H

// The player's item, riddle and skill actions (plan item-actions-out-of-screens): what an action does to the game (the
// bag, the stats, the journal, the run's counters, the XP), GL-free, so the screens, the scenario commands and the
// unit tests share one copy. Each returns what the screen shows; the sounds go out as WorldEvents.
#include "item_bag.h"
#include "items.h"
#include "journal.h"
#include "player_stats.h"
#include "shrines.h"
#include "skills.h"
#include "world_events.h"
#include <optional>
#include <string>

// What a player action touches.
struct PlayerActor {
	PlayerStats& stats;
	ItemBag& bag;
	Journal& journal;
	WorldEvents& events;
	BagUser user; // the player's state for the bag's rules (bagUser), taken before the action
};

// An action's outcome: done or refused, the line to show (empty: none), the item to select after it.
struct ActionResult {
	bool done = false;
	std::string text;
	std::optional<ItemKind> select;
};

// The worn rings' bonus to the player's stats.
void applyRings(PlayerActor& actor);
// Finds the item (its journal note) as from a chest.
void addItem(PlayerActor& actor, ItemKind kind);

// A spare offered at the god's shrine (plan shrine-offerings): offerXP, the prayer's beam and sound, and now and then
// a gift (plan offering-gift; `level` and the run's seed and offering count roll it, `alwaysGift` forces one).
ActionResult offerItem(PlayerActor& actor, ItemKind kind, God god, int level, bool alwaysGift);
// Two rings of one kind joined at the god's shrine into one of the next metal (plan ring-accessories); its notes.
ActionResult mergeRings(PlayerActor& actor, ItemKind ring, God god);
// The ring card's hand button: the ring on that hand, or off it.
ActionResult wearRingOn(PlayerActor& actor, ItemKind ring, Hand hand);
// A weapon's level or an amulet's tier up; a worn amulet keeps its HP share.
ActionResult upgradeItem(PlayerActor& actor, ItemKind kind, AtShrine shrine);
// The Use button: drink a potion, a ring on or off (the free hand first), an amulet on or off, a weapon in hand.
ActionResult useItem(PlayerActor& actor, ItemKind kind);
// The potion drunk: its gain to the player and the run's count; returns the feedback line.
std::string drinkPotion(PlayerActor& actor, ItemKind potion);

// A riddle answered (the journal's entry `journalIndex`, `late`: from the journal): solved in the journal, counted,
// its reward in XP.
void solveRiddle(
	PlayerStats& stats,
	Journal& journal,
	WorldEvents& events,
	size_t journalIndex,
	bool late,
	const std::string& answer,
	int reward
);
// A heka spent on the skill (`force`: also a sealed one, as the scenario's `learn`): its field note, a cure, and for
// Hieroglyphs every copied inscription translated (InscriptionXP each, their boons). Returns the toast ("Learnt:
// ..."), nothing if it could not be learnt.
std::optional<std::string>
learnSkill(PlayerStats& stats, Journal& journal, WorldEvents& events, SkillId id, bool force = false);
// A heka spent on the skill's rank II (plan skill-upgrades). Returns the toast ("Rank II: ..."), nothing if it could
// not be upgraded.
std::optional<std::string> upgradeSkill(PlayerStats& stats, WorldEvents& events, SkillId id);

#endif
