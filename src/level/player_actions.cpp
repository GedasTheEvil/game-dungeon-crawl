#include "player_actions.h"
#include "offering_gift.h"
#include <cstdio>

namespace {
// "Copper Garnet Ring put on the left hand"
std::string handLine(ItemKind ring, bool on, Hand hand) {
	return std::string(itemText(ring).name) + (on ? " put on the " : " taken off the ") +
		   (hand == Hand::Left ? "left" : "right") + " hand";
}

// Plan offering-gift: now and then the god gives a gift on top; its text (". Montu grants a gift: Might +1"), empty
// for none.
std::string gift(PlayerActor& actor, God god, int level, bool always) {
	PlayerStats& stats = actor.stats;
	const OfferingGift gift = RollOfferingGift(god, level, stats.run.nameSeed, stats.run.offerings, always);
	stats.run.offerings++;
	const std::string giver = std::string(". ") + godDef(god).name + " grants ";
	if (gift.boon.Any())
		return giver + "a gift: " + stats.Grant(gift.boon);
	if (gift.item) {
		addItem(actor, *gift.item);
		const std::string name = itemText(*gift.item).name;
		const bool vowel = std::string("AEIOU").contains(name.front());
		return giver + (vowel ? "an " : "a ") + name;
	}
	return "";
}

WorldSound prayerSound(God god) {
	return static_cast<WorldSound>(static_cast<int>(WorldSound::ShrinePray) + static_cast<int>(god));
}
} // namespace

void applyRings(PlayerActor& actor) { actor.stats.WearRings(ringBonus(actor.bag.Rings())); }

void addItem(PlayerActor& actor, ItemKind kind) { actor.bag.Find(kind, actor.journal); }

ActionResult offerItem(PlayerActor& actor, ItemKind kind, God god, int level, bool alwaysGift) {
	if (!actor.bag.Offer(kind, actor.user.alive, AtShrine::Yes))
		return {};
	const int xp = offerXP(kind, actor.stats.CurrentXP());
	actor.stats.Offered(god);
	actor.events.Play(prayerSound(god));
	actor.stats.AddXP(xp, actor.events);
	char buf[64];
	snprintf(buf, sizeof(buf), "%s accepts the offering. +%d XP", godDef(god).name, xp);
	return {true, buf + gift(actor, god, level, alwaysGift), std::nullopt};
}

ActionResult mergeRings(PlayerActor& actor, ItemKind ring, God god) {
	const std::optional<ItemKind> next = nextMetal(ring);
	if (!next || !actor.bag.Merge(ring, actor.user.alive, AtShrine::Yes))
		return {};
	noteItem(*next, actor.journal); // a sky iron ring's note, as one found
	applyRings(actor);
	actor.stats.Offered(god);
	actor.events.Play(prayerSound(god));
	return {true, std::string(godDef(god).name) + " joins the rings: " + itemText(*next).name, next};
}

ActionResult wearRingOn(PlayerActor& actor, ItemKind ring, Hand hand) {
	if (!actor.bag.WearOn(ring, hand, actor.user.alive))
		return {};
	applyRings(actor);
	actor.events.Play(WorldSound::ItemOn);
	return {true, handLine(ring, actor.bag.RingOn(hand) == ring, hand), std::nullopt};
}

ActionResult upgradeItem(PlayerActor& actor, ItemKind kind, AtShrine shrine) {
	if (!actor.bag.Upgrade(kind, actor.user.alive, shrine))
		return {};
	// An amulet: one of the next tier now; worn, it keeps the HP share.
	if (const std::optional<ItemKind> upgraded = isAmulet(kind) ? nextTier(kind) : std::nullopt) {
		if (actor.bag.Worn() == upgraded)
			actor.stats.Wear(amuletBonus(upgraded), true);
		actor.events.Play(WorldSound::ItemOn);
		return {true, std::string("Upgraded: ") + itemText(*upgraded).name, upgraded};
	}
	char buf[64];
	snprintf(buf, sizeof(buf), "%s reaches level %d", itemText(kind).name, actor.bag.Level(kind));
	return {true, buf, std::nullopt};
}

ActionResult useItem(PlayerActor& actor, ItemKind kind) {
	if (actor.bag.Block(kind, actor.user) != UseBlock::None)
		return {};
	if (isPotion(kind))
		return {true, drinkPotion(actor, kind), std::nullopt};
	if (isRing(kind)) { // the quick way: off if worn, else on the free hand (or in place of the older one)
		const WornRings before = actor.bag.Rings();
		actor.bag.Use(kind, actor.user);
		applyRings(actor);
		actor.events.Play(WorldSound::ItemOn);
		for (int hand = 0; hand < HAND_COUNT; hand++)
			if (before[static_cast<size_t>(hand)] != actor.bag.Rings()[static_cast<size_t>(hand)]) {
				const bool on = actor.bag.Rings()[static_cast<size_t>(hand)] == kind;
				return {true, handLine(kind, on, static_cast<Hand>(hand)), std::nullopt};
			}
		return {true, "", std::nullopt};
	}
	if (isAmulet(kind)) {
		actor.bag.Use(kind, actor.user);
		actor.stats.Wear(amuletBonus(actor.bag.Worn()), true);
		actor.events.Play(WorldSound::ItemOn);
		return {
			true,
			std::string(itemText(kind).name) + (actor.bag.Worn() == kind ? " put on" : " taken off"),
			std::nullopt
		};
	}
	actor.bag.Use(kind, actor.user);
	return {true, std::string(itemText(kind).name) + " equipped", std::nullopt};
}

std::string drinkPotion(PlayerActor& actor, ItemKind potion) {
	actor.events.Play(WorldSound::Drink);
	actor.bag.Use(potion, actor.user);
	actor.stats.run.Drink(potion);
	return actor.stats.Drink(potionGain(potion));
}

void solveRiddle(
	PlayerStats& stats,
	Journal& journal,
	WorldEvents& events,
	size_t journalIndex,
	bool late,
	const std::string& answer,
	int reward
) {
	journal.Solve(journalIndex, late, answer);
	stats.run.riddles++;
	stats.AddXP(reward, events);
}

std::optional<std::string>
learnSkill(PlayerStats& stats, Journal& journal, WorldEvents& events, SkillId id, bool force) {
	LearnCure cure = LearnCure::None;
	if (!stats.Learn(id, force, &cure))
		return std::nullopt;
	if (const std::optional<FieldNote> note = skillNote(id))
		journal.LearnNote(*note);
	std::string learnt = std::string("Learnt: ") + skillDef(id).name;
	if (cure == LearnCure::Poison)
		learnt += ". The poison is gone";
	else if (cure == LearnCure::Curses)
		learnt += ". The curses lift";
	if (id == SkillId::Hieroglyphs) {
		const std::vector<Boon> boons = journal.TranslateInscriptions();
		const int copies = static_cast<int>(boons.size());
		if (copies > 0) {
			// Every copy gives the full XP at the level now, no penalty for learning late (plan inscription-xp).
			const int xp = copies * stats.InscriptionXP();
			learnt += ". " + std::to_string(copies) + (copies == 1 ? " inscription" : " inscriptions") +
					  " translated, +" + std::to_string(xp) + " XP";
			for (const Boon& b : boons) // plan inscription-boons
				if (b.Any())
					learnt += ", " + stats.Grant(b);
			stats.AddXP(xp, events);
		}
	}
	events.Play(WorldSound::ItemOn);
	return learnt;
}
