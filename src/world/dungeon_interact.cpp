// What the player does on a cell (Interact): pick up the treasure, pull a lever, read an inscription, use a gate (the
// exit, a teleport), and the travel through a gate (the fade). Loading and saving the dungeon: dungeon_io.cpp.
#include "dungeon.h"
#include "../entities/player.h"
#include "inscriptions.h"
#include "journal.h"
#include "loot.h"
#include "names.h"
#include "dungeon_rules.h"
#include "world_events.h"
#include <cctype>
#include <string>
#include <vector>

bool Dungeon::PickUp() {
	if (!isTreasure(Map(mapX, mapY)))
		return false;
	sim.player->stats.run.treasures++;
	const bool trapped = Map(mapX, mapY).type == TrappedTreasure; // one bonus more (plan trapped-chests)
	std::optional<ItemKind> placed = itemFromFile(Map(mapX, mapY).attr, Map(mapX, mapY).value);
	const int cell = MapIndex(static_cast<int>(mapX), static_cast<int>(mapY));
	clearObject(map[cell]);
	containers[cell].opened = true; // the open container stays, empty
	containers[cell].emptied = true;
	if (!placed) // an empty chest (type 0), or an item the game does not know
		return true;

	// One line per item: "Found: Sword", then "+ Small Stamina" for each bonus.
	std::string found;
	std::vector<ItemKind> loot =
		trapped ? RollTrappedChestLoot(*placed, sim.items->Owned(), levelNumber, sim.random->gameplay)
				: RollChestLoot(*placed, sim.items->Owned(), levelNumber, sim.random->gameplay);
	for (size_t i = 0; i < loot.size(); i++) {
		sim.items->Find(loot[i], *sim.journal);
		found += std::string(i == 0 ? "Found: " : "\n+ ") + itemText(loot[i]).name;
	}
	sim.events->Status("%s", found.c_str());
	return true;
}
//======================================================================================
// One thing a press, the one the prompt shows (plan interact-prompt).
void Dungeon::Interact() {
	switch (InteractVerb()) {
	case Interaction::None:
		break;
	case Interaction::Open:
		if (const int slot = ambushHere(); slot >= 0)
			monsters[slot].Spring(); // the Chest of Set: opened like a chest, it bites
		else
			openContainer();
		break;
	case Interaction::Take:
		if (isTreasure(Map(mapX, mapY)))
			PickUp();
		else
			useGate(); // the ankh
		break;
	case Interaction::Pull:
		PullLever();
		break;
	case Interaction::Read:
	case Interaction::Copy:
		readInscription();
		break;
	case Interaction::Light:
		lightArrows();
		break;
	case Interaction::Turn:
		TurnMirror();
		break;
	case Interaction::Answer:
	case Interaction::Enter:
		useGate();
		break;
	}
}
//======================================================================================
Interaction Dungeon::InteractVerb() const {
	const auto col = static_cast<int>(mapX), row = static_cast<int>(mapY);
	if (!IsInBounds(col, row))
		return Interaction::None;
	const Tile here = MapAt(col, row);
	if (ambushHere() >= 0)
		return Interaction::Open;
	if (isTreasure(here)) {
		const ContainerCell& c = containers[MapIndex(col, row)];
		if (!c.opened)
			return Interaction::Open;
		return containerShows(MapIndex(col, row)) ? Interaction::Take : Interaction::None;
	}
	if (here.fixture.type == Lever && isLockColour(here.fixture.attr) && !leverPulled(here.fixture))
		return Interaction::Pull;
	if (here.fixture.type == Inscription) {
		const bool worn = !inscriptionPage(here.fixture);
		return worn || sim.player->stats.heka.Has(SkillId::Hieroglyphs) ? Interaction::Read : Interaction::Copy;
	}
	if (here.type == Mirror)
		return Interaction::Turn;
	const OwnedWeapons& owned = sim.items->Owned();
	if (decoration.decor[MapIndex(col, row)].type == DECOR_BRAZIER &&
		(owned[static_cast<size_t>(itemIndex(ItemKind::SelfBow))] ||
		 owned[static_cast<size_t>(itemIndex(ItemKind::CompositeBow))]))
		return Interaction::Light;
	if (here.type == Ankh)
		return Interaction::Take;
	if (here.type != Door)
		return Interaction::None;
	switch (here.attr) {
	case GateRiddle:
		return Interaction::Answer;
	case GateExit:
		return Interaction::Enter;
	case GateTeleport:
		if (isNameGate(here))
			return Interaction::Answer;
		return !sim.player->jump.jumping && !sim.player->jump.falling && teleportPartner(map, MapIndex(col, row)) >= 0
				   ? Interaction::Enter
				   : Interaction::None;
	default: // the entrance, an answered riddle gate
		return Interaction::None;
	}
}
//======================================================================================
// The clip plays, the item shows at its end (containerShows). A trapped chest's trap goes off as it starts.
void Dungeon::openContainer() {
	const int cell = MapIndex(static_cast<int>(mapX), static_cast<int>(mapY));
	if (!isTreasure(map[cell]) || containers[cell].opened)
		return;
	containers[cell].opened = true;
	containers[cell].openedMs = GameClock::now();
	const Container kind = treasureLook(cell).kind;
	switch (kind) {
	case Container::Chest:
	case Container::Coffer:
		sim.events->Play(WorldSound::LidWood);
		break;
	case Container::Basket:
	case Container::Table: // the linen cover
		sim.events->Play(WorldSound::LidReed);
		break;
	case Container::Canopic:
		sim.events->Play(WorldSound::LidStone);
		sim.events->Note(FieldNote::CanopicJars);
		break;
	case Container::Jar:
		sim.events->Play(WorldSound::JarSmash);
		break;
	}
	if (map[cell].type == TrappedTreasure)
		springChestTrap(cell);
}
//======================================================================================
void Dungeon::placeDroppedChest(int cell, ItemFileId item) {
	setObject(map[cell], Tile{Treasure, item.type, item.id});
	containers[cell] = {true, false, true, GameClock::now() - containerDef(DROPPED_LOOK.kind).openMs};
}
//======================================================================================
float Dungeon::containerOpening(int cell) const {
	const ContainerCell& c = containers[cell];
	if (!c.opened)
		return 0.f;
	return motionProgress(c.openedMs, containerDef(treasureLook(cell).kind).openMs);
}
//======================================================================================
bool Dungeon::containerShows(int cell) const { return containerOpening(cell) >= 1.f; }
//======================================================================================
TreasureLook Dungeon::treasureLook(int cell) const {
	return containers[cell].dropped ? DROPPED_LOOK : decoration.treasure[cell];
}
//======================================================================================
bool Dungeon::PlaceContainer(int col, int row, int kind, int stance) {
	if (!IsInBounds(col, row) || kind < 0 || kind >= CONTAINER_COUNT || stance < 0 || stance >= STANCE_COUNT)
		return false;
	const StanceDef& st = STANCES[stance];
	decoration.treasure[MapIndex(col, row)] = {static_cast<Container>(kind), st.yaw, st.x, st.z};
	return true;
}
//======================================================================================
int Dungeon::ambushHere() const {
	const auto col = static_cast<int>(mapX), row = static_cast<int>(mapY);
	for (int slot = 0; slot < MAX_MONSTERS; slot++) {
		const Monster& mon = monsters[slot];
		if (mon.Active() && mon.Alive() && mon.lurking() && ambushes(mon.Type()->locomotion) && mon.Col() == col &&
			mon.Row() == row)
			return slot;
	}
	return -1;
}
//======================================================================================
float Dungeon::InscriptionGlint(int col, int row) const {
	constexpr float COPY_GLINT = 0.4f; // a copy keeps a dimmer glint until the skill is learnt (plan skill-hieroglyphs)
	if (!IsInBounds(col, row) || MapAt(col, row).fixture.type != Inscription)
		return 0.f;
	const std::optional<JournalInscription> page = inscriptionPage(MapAt(col, row).fixture);
	if (!page) // too worn to read: nothing to learn there
		return 0.f;
	if (const JournalInscription* read = sim.journal->FindInscription(*page))
		return read->translated ? 0.f : COPY_GLINT;
	return 1.f;
}
//======================================================================================
void Dungeon::lightArrows() {
	sim.player->fireArrows = FIRE_ARROW_SHOTS;
	sim.events->Status("Dipped %d arrows in the brazier's fire", FIRE_ARROW_SHOTS);
}
//======================================================================================
const RoyalName* Dungeon::LevelName() const {
	return sim.names != nullptr ? sim.names->ForLevel(sim.player->stats.run.nameSeed, levelNumber) : nullptr;
}
//======================================================================================
int Dungeon::NameCartouchePicture() const { return nameCartouchePicture(sim.player->stats.run.nameSeed, levelNumber); }
//======================================================================================
std::optional<JournalInscription> Dungeon::inscriptionPage(const Fixture& f) const {
	const bool reads = sim.player->stats.heka.Has(SkillId::Hieroglyphs);
	if (f.attr == NAME_CARTOUCHE_ID) { // plan secret-name-gates
		const RoyalName* name = LevelName();
		if (name == nullptr)
			return std::nullopt;
		std::string upper = name->name;
		for (char& c : upper)
			c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
		const std::string asked = "Something on this level wants to hear it.";
		JournalInscription page{
			NAME_CARTOUCHE_ID,
			"A royal name",
			{"The name of the " + name->title + ": " + upper},
			name->note.empty() ? asked : name->note + " " + asked,
			levelNumber,
			reads
		};
		page.picture = NameCartouchePicture();
		return page;
	}
	const InscriptionText* text = sim.inscriptions != nullptr ? sim.inscriptions->Find(f.attr) : nullptr;
	if (text == nullptr)
		return std::nullopt;
	return JournalInscription{text->id, text->title, text->text, text->note, levelNumber, reads, text->boon};
}
//======================================================================================
// The status line only says what happened; the title and the comment go into the journal (plans
// hieroglyph-cartouches, inscription-messages, skill-hieroglyphs).
bool Dungeon::readInscription() {
	const Fixture here = Map(mapX, mapY).fixture;
	if (here.type != Inscription)
		return false;
	const std::optional<JournalInscription> page = inscriptionPage(here);
	if (!page) {
		sim.events->Status("The signs are too worn to read.");
		return true;
	}
	// Without the skill the glyphs are copied into the journal, the text stays hidden there (plan skill-hieroglyphs).
	const bool reads = page->translated;
	const bool first = sim.journal->ReadInscription(*page);
	if (first)
		sim.player->stats.run.inscriptions++;
	if (!first)
		sim.events->Status("Already seen. In the journal.");
	else if (reads) {
		const int xp = sim.player->stats.InscriptionXP(); // plan inscription-xp
		const std::string boon = sim.player->stats.Grant(page->boon); // plan inscription-boons
		sim.events->Status(
			"Deciphered the hieroglyphs. Journal updated. +%d XP%s%s",
			xp,
			boon.empty() ? "" : ", ",
			boon.c_str()
		);
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
		if (isNameGate(here))
			sim.events->AskName(); // plan secret-name-gates
		else if (!sim.player->jump.jumping && !sim.player->jump.falling &&
				 teleportPartner(map, MapIndex(static_cast<int>(mapX), static_cast<int>(mapY))) >= 0)
			startTravel(false);
		break;
	default: // the entrance, an answered riddle gate
		break;
	}
}
//======================================================================================
bool Dungeon::SpeakName() {
	const int cell = MapIndex(static_cast<int>(mapX), static_cast<int>(mapY));
	if (!isNameGate(map[cell]))
		return false;
	unsealNameGate(map[cell]);
	namedCell = cell;
	namedAtMs = GameClock::now();
	sim.events->Play(WorldSound::Teleport);
	sim.events->Status("The name is spoken. The way opens.");
	return true;
}
//======================================================================================
void Dungeon::startTravel(bool exitGate) {
	travel = {GateTravel::Phase::Out, GameClock::now(), exitGate};
	if (!exitGate)
		sim.events->Play(WorldSound::Teleport);
}
//======================================================================================
void Dungeon::updateTravel() {
	updateAnimations();
	if (GameClock::now() - travel.startMs < GATE_TRAVEL_FADE_MS)
		return;
	if (travel.phase == GateTravel::Phase::Out) {
		if (travel.exit) {
			if (LoadCampaignLevel(levelNumber + 1))
				sim.events->Autosave(AutosaveMoment::LevelStart); // plan autosave
		} else
			Teleport();
		travel.phase = GateTravel::Phase::In;
		travel.startMs = GameClock::now();
	} else
		travel.phase = GateTravel::Phase::None;
}
//======================================================================================
float Dungeon::TravelFade() const {
	if (travel.phase == GateTravel::Phase::None)
		return 0.f;
	const float t = motionProgress(travel.startMs, GATE_TRAVEL_FADE_MS);
	return travel.phase == GateTravel::Phase::Out ? t : 1.f - t;
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
