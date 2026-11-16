#include "item_commands.h"
#include "game_state.h"
#include "../core/timer.h"
#include "../sim/player_control.h"
#include <algorithm>

PlayerActor ItemCommands::Actor() {
	return {game.player->stats, game.bag, game.journal, game.events, bagUser(*game.player)};
}

ActionResult ItemCommands::Use(ItemKind kind) {
	PlayerActor actor = Actor();
	return useItem(actor, kind);
}

void ItemCommands::AddItem(ItemKind kind) {
	PlayerActor actor = Actor();
	addItem(actor, kind);
}

bool ItemCommands::Equip(ItemKind weapon) {
	if (!isWeapon(weapon))
		return false;
	Use(weapon);
	return game.bag.Equipped() == weapon;
}

bool ItemCommands::Wear(std::optional<ItemKind> amulet) {
	const ItemBag& bag = game.bag;
	if (!amulet) {
		if (std::optional<ItemKind> worn = bag.Worn())
			Use(*worn);
		return !bag.Worn();
	}
	if (!isAmulet(*amulet) || bag.Count(*amulet) <= 0)
		return false;
	if (bag.Worn() != amulet)
		Use(*amulet);
	return bag.Worn() == amulet;
}

bool ItemCommands::WearRing(ItemKind ring, Hand hand) {
	if (!isRing(ring) || game.bag.Count(ring) <= 0)
		return false;
	if (game.bag.RingOn(hand) != ring) {
		PlayerActor actor = Actor();
		wearRingOn(actor, ring, hand);
	}
	return game.bag.RingOn(hand) == ring;
}

void ItemCommands::EquipNext(bool ranged) {
	const ItemKind held = game.bag.Equipped();
	const int from = isRanged(held) == ranged ? itemIndex(held) : -1;
	for (int step = 1; step < WEAPON_KIND_COUNT + 1; step++) {
		const ItemKind kind = itemAt((from + step) % WEAPON_KIND_COUNT);
		if (isRanged(kind) != ranged)
			continue;
		if (kind == held) // round to it again: no other one of its class
			return;
		if (game.bag.Count(kind) > 0) {
			Use(kind);
			return;
		}
	}
}

std::optional<ItemKind> ItemCommands::QuickChoice(QuickKind kind) const {
	const PlayerStats& s = game.player->stats;
	const bool health = kind == QuickKind::Health;
	const int current = health ? s.CurrentHP() : s.Stamina();
	const int max = health ? s.CurrentMaxHP() : s.MaxStamina();
	// At full, still show what a hit would make it drink: the pick for one point missing.
	return game.bag.QuickChoice(kind, std::min(current, max - 1), max);
}

void ItemCommands::QuickDrink(QuickKind kind) {
	const int now = GameClock::now();
	if (quickDrinkMs && now - *quickDrinkMs < QUICK_DRINK_COOLDOWN_MS)
		return;
	const PlayerStats& s = game.player->stats;
	const bool health = kind == QuickKind::Health;
	const bool full = health ? s.CurrentHP() >= s.CurrentMaxHP() : s.Stamina() >= s.MaxStamina();
	const std::optional<ItemKind> potion = QuickChoice(kind);
	if (full) {
		game.ShowStatus(health ? "You are at full health" : "You are at full energy");
		return;
	}
	if (!potion) {
		game.ShowStatus(health ? "No healing potion left" : "No stamina potion left");
		return;
	}
	quickDrinkMs = now;
	quickDrinkKind = kind;
	PlayerActor actor = Actor();
	game.ShowStatus("%s", drinkPotion(actor, *potion).c_str());
}

void ItemCommands::QuickAntidote() {
	const UseBlock block = game.bag.Block(ItemKind::Antidote, bagUser(*game.player));
	if (block == UseBlock::Dead)
		return;
	if (block != UseBlock::None) {
		game.ShowStatus("%s", block == UseBlock::NotPoisoned ? "You are not poisoned" : "No antidote left");
		return;
	}
	antidoteMs = GameClock::now();
	PlayerActor actor = Actor();
	game.ShowStatus("%s", drinkPotion(actor, ItemKind::Antidote).c_str());
}

std::optional<int> ItemCommands::QuickDrinkMs(QuickKind kind) const {
	return quickDrinkKind == kind ? quickDrinkMs : std::nullopt;
}

void ItemCommands::FollowInsanity(bool insane) {
	if (insane && !saneWeapon) {
		saneWeapon = game.bag.Equipped();
		Equip(ItemKind::Club);
	} else if (!insane && saneWeapon.has_value()) {
		const ItemKind weapon = *saneWeapon;
		saneWeapon.reset();
		Equip(weapon);
	}
}

void ItemCommands::Reset() {
	game.bag.Reset();
	quickDrinkMs.reset();
	antidoteMs.reset();
	saneWeapon.reset();
}

void ItemCommands::BagLoaded() { saneWeapon.reset(); }
