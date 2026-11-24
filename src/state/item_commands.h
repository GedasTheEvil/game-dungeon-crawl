#ifndef ITEM_COMMANDS_H
#define ITEM_COMMANDS_H

#include "../level/rules/player_actions.h"
#include "../level/rules/quick_potion.h"
#include <optional>

class GameState;

// The player's item commands outside the inventory screen (plan game-state-split): the quick drinks, the weapon keys,
// insanity's club, and what scenarios equip, wear and add. The screen's buttons go through Use and Actor too.
class ItemCommands {
  public:
	explicit ItemCommands(GameState& game) : game(game) {}

	PlayerActor Actor(); // the player, the bag, the journal and the events, for the actions (player_actions.h)
	ActionResult Use(ItemKind kind); // the Use button (useItem): drink, wear or take off, equip
	void AddItem(ItemKind kind); // found: its journal notes (ItemBag::Find)
	// Equips the weapon (scenario `equip`). False if it is not in hand afterwards (not held, a potion).
	bool Equip(ItemKind weapon);
	// Puts the amulet on, or takes the worn one off (nullopt) (scenario `wear`). False if it is not held or not worn
	// afterwards.
	bool Wear(std::optional<ItemKind> amulet);
	// Puts the ring on that hand (scenario `wear <ring> left|right`). False if it is not held or not on that hand
	// afterwards.
	bool WearRing(ItemKind ring, Hand hand);
	// The class keys (1 melee, 2 ranged): equips the next weapon of that class held, in ItemKind order and wrapping
	// round; the first one if the other class is in hand. None held: nothing.
	void EquipNext(bool ranged);
	// H drinks a healing and 0 a stamina potion, the best fit (quickPotion). Full health / stamina or none left:
	// nothing is drunk, the status box says why.
	void QuickDrink(QuickKind kind);
	// = drinks an antidote. Not poisoned or none left: nothing is drunk, the status box says why.
	void QuickAntidote();
	// The potion QuickDrink would drink now, ignoring whether anything is missing (the HUD slot); nullopt if none
	// left.
	[[nodiscard]] std::optional<ItemKind> QuickChoice(QuickKind kind) const;
	// Game clock time of the last quick drink of that kind, for the HUD flash.
	[[nodiscard]] std::optional<int> QuickDrinkMs(QuickKind kind) const;
	[[nodiscard]] std::optional<int> AntidoteMs() const { return antidoteMs; }
	// Once a tick: insanity's start puts the club in hand and remembers the weapon, its end gives the weapon back (plan
	// curses).
	void FollowInsanity(bool insane);
	void Reset(); // a new game: only the club (ItemBag::Reset), no quick drink, no weapon to give back
	void BagLoaded(); // a save game was read into the bag: insanity's weapon is forgotten

  private:
	GameState& game;
	// Last quick drink (H / 0 in game), for the cooldown and the HUD slot flash.
	std::optional<int> quickDrinkMs;
	QuickKind quickDrinkKind = QuickKind::Health;
	std::optional<int> antidoteMs; // the last antidote drunk with its key, for the HUD slot flash
	// The weapon in hand when insanity put the club there (FollowInsanity); a load forgets it.
	std::optional<ItemKind> saneWeapon;
};

#endif
