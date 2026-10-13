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

void Dungeon::PickUp() {
	if (Map(mapX, mapY).type == Treasure) {
		std::optional<ItemKind> placed = itemFromFile(Map(mapX, mapY).attr, Map(mapX, mapY).value);
		clearObject(map[MapIndex(static_cast<int>(mapX), static_cast<int>(mapY))]);
		if (!placed) // an empty chest (type 0), or an item the game does not know
			return;

		// One line per item: "Found: Sword", then "+ Small Stamina" for each bonus.
		std::string found;
		std::vector<ItemKind> loot = RollChestLoot(*placed, sim.items->Owned(), levelNumber, sim.random->gameplay);
		for (size_t i = 0; i < loot.size(); i++) {
			sim.items->Find(loot[i], *sim.journal);
			found += std::string(i == 0 ? "Found: " : "\n+ ") + itemText(loot[i]).name;
		}
		sim.events->Status("%s", found.c_str());
	}
}
//======================================================================================
// Keys, treasure and levers are picked up or pulled elsewhere (PickUp, the mechanisms).
void Dungeon::Interact() {
	PickUp();
	if (PullLever() || readInscription())
		return;
	useGate();
}
//======================================================================================
// The status line only says what happened; the title and the comment go into the journal (plans
// hieroglyph-cartouches, inscription-messages).
bool Dungeon::readInscription() {
	const Tile here = Map(mapX, mapY);
	if (here.type != Inscription)
		return false;
	const InscriptionText* text = sim.inscriptions != nullptr ? sim.inscriptions->Find(here.attr) : nullptr;
	if (text == nullptr) {
		sim.events->Status("The signs are too worn to read.");
		return true;
	}
	const bool first = sim.journal->ReadInscription({text->id, text->title, text->text, text->note, levelNumber});
	sim.events->Status(first ? "Deciphered the hieroglyphs. Journal updated." : "Already seen. In the journal.");
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
