// What the player does on a cell (Interact): pick up the treasure, pull a lever, read an inscription, use a gate (the
// exit, a teleport), and the travel through a gate (the fade). Loading and saving the dungeon: dungeon_io.cpp.
#include "dungeon.h"
#include "../entities/player.h"
#include "inscriptions.h"
#include "journal.h"
#include "loot.h"
#include "dungeon_rules.h"
#include <string>
#include <vector>

bool Dungeon::PickUp() {
	if (Map(mapX, mapY).type != Treasure)
		return false;
	sim.player->stats.run.treasures++;
	std::optional<ItemKind> placed = itemFromFile(Map(mapX, mapY).attr, Map(mapX, mapY).value);
	clearObject(map[MapIndex(static_cast<int>(mapX), static_cast<int>(mapY))]);
	if (!placed) // an empty chest (type 0), or an item the game does not know
		return true;

	// One line per item: "Found: Sword", then "+ Small Stamina" for each bonus.
	std::string found;
	std::vector<ItemKind> loot = RollChestLoot(*placed, sim.items->Owned(), levelNumber, sim.random->gameplay);
	for (size_t i = 0; i < loot.size(); i++) {
		sim.items->Find(loot[i], *sim.journal);
		found += std::string(i == 0 ? "Found: " : "\n+ ") + itemText(loot[i]).name;
	}
	sim.events->Status("%s", found.c_str());
	return true;
}
//======================================================================================
// One thing a press (plan wall-layer): the chest in the cell first, then the fixture on its back wall (a lever,
// an inscription), the brazier, and the gate last, as it may take the player away. Keys are picked up on touch.
void Dungeon::Interact() {
	if (PickUp() || PullLever() || readInscription() || lightArrows())
		return;
	useGate();
}
//======================================================================================
bool Dungeon::lightArrows() {
	const auto col = static_cast<int>(mapX), row = static_cast<int>(mapY);
	const OwnedWeapons& owned = sim.items->Owned();
	if (!IsInBounds(col, row) || decoration.decor[MapIndex(col, row)].type != DECOR_BRAZIER ||
		(!owned[static_cast<size_t>(itemIndex(ItemKind::SelfBow))] &&
		 !owned[static_cast<size_t>(itemIndex(ItemKind::CompositeBow))]))
		return false;
	sim.player->fireArrows = FIRE_ARROW_SHOTS;
	sim.events->Status("Dipped %d arrows in the brazier's fire", FIRE_ARROW_SHOTS);
	return true;
}
//======================================================================================
// The status line only says what happened; the title and the comment go into the journal (plans
// hieroglyph-cartouches, inscription-messages, skill-hieroglyphs).
bool Dungeon::readInscription() {
	const Fixture here = Map(mapX, mapY).fixture;
	if (here.type != Inscription)
		return false;
	const InscriptionText* text = sim.inscriptions != nullptr ? sim.inscriptions->Find(here.attr) : nullptr;
	if (text == nullptr) {
		sim.events->Status("The signs are too worn to read.");
		return true;
	}
	// Without the skill the glyphs are copied into the journal, the text stays hidden there (plan skill-hieroglyphs).
	const bool reads = sim.player->stats.heka.Has(SkillId::Hieroglyphs);
	const bool first =
		sim.journal->ReadInscription({text->id, text->title, text->text, text->note, levelNumber, reads, text->boon});
	if (first)
		sim.player->stats.run.inscriptions++;
	if (!first)
		sim.events->Status("Already seen. In the journal.");
	else if (reads) {
		const int xp = sim.player->stats.InscriptionXP(); // plan inscription-xp
		const std::string boon = sim.player->stats.Grant(text->boon); // plan inscription-boons
		sim.events->Status("Deciphered the hieroglyphs. Journal updated. +%d XP%s%s", xp, boon.empty() ? "" : ", ",
						   boon.c_str());
		sim.player->stats.AddXP(xp, *sim.events);
	} else
		sim.events->Status("Cannot understand this. Copied the hieroglyphs to the journal.");
	return true;
}
//======================================================================================
void Dungeon::useGate() {
	const Tile here = Map(mapX, mapY);
	if (here.type == Ankh) {
		won = true;
		return;
	}
	if (here.type != Door)
		return;
	switch (here.attr) {
	case GateRiddle:
		sim.events->AskRiddle();
		SetMapBAtPlayer(GateEmpty);
		break;
	case GateExit:
		startTravel(true);
		break;
	case GateTeleport:
		if (!sim.player->jump.jumping && !sim.player->jump.falling &&
			teleportPartner(map, MapIndex(static_cast<int>(mapX), static_cast<int>(mapY))) >= 0)
			startTravel(false);
		break;
	default: // the entrance, an answered riddle gate
		break;
	}
}
//======================================================================================
void Dungeon::startTravel(bool exitGate) {
	travel = Travel::Out;
	travelStartMs = GameClock::now();
	travelExit = exitGate;
	if (!exitGate)
		sim.events->Play(WorldSound::Teleport);
}
//======================================================================================
void Dungeon::updateTravel() {
	updateAnimations();
	if (GameClock::now() - travelStartMs < GATE_TRAVEL_FADE_MS)
		return;
	if (travel == Travel::Out) {
		if (travelExit)
			LoadCampaignLevel(levelNumber + 1);
		else
			Teleport();
		travel = Travel::In;
		travelStartMs = GameClock::now();
	} else
		travel = Travel::None;
}
//======================================================================================
float Dungeon::TravelFade() const {
	if (travel == Travel::None)
		return 0.f;
	const float t = motionProgress(travelStartMs, GATE_TRAVEL_FADE_MS);
	return travel == Travel::Out ? t : 1.f - t;
}
//======================================================================================
// The player steps out in the middle of the partner gate, in the plasma.
void Dungeon::Teleport() {
	const JumpState& jump = sim.player->jump;
	if (jump.jumping || jump.falling)
		return;
	int to = teleportPartner(map, MapIndex(static_cast<int>(mapX), static_cast<int>(mapY)));
	if (to < 0)
		return;
	const int row = to / MAP_WIDTH;
	mapX = static_cast<float>(to % MAP_WIDTH) + TELEPORT_ARRIVAL_X;
	mapY = static_cast<float>(row);
	resetPlayerMotion();
	exploreAroundPlayer();
}
