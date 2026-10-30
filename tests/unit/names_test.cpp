// Secret name gates (plan secret-name-gates): the name files, the pick per run, the name gate and its cartouche in the
// level, the checker's rules, the campaign's rooms.
#include "../../external/doctest/doctest.h"
#include "../../src/world/campaign.h"
#include "../../src/world/level_check.h"
#include "../../src/world/names.h"
#include "../../src/world/run_stats.h"
#include "sim_world.h"
#include <set>
#include <sstream>
#include "../../src/core/paths.h"

namespace {
// Row 1: the entrance (col 1), the name cartouche (5), the name gate (9, pair 3), the exit (14). Above, a pocket on
// row 5: the partner teleporter (3) and a chest (5).
LevelGrid nameGateLevel() {
	LevelGrid grid;
	for (int col = 1; col <= 15; col++)
		grid.set(col, 1, Tile{});
	for (int col = 2; col <= 6; col++)
		grid.set(col, 5, Tile{});
	grid.set(1, 1, cellWith({Door, GateEntrance, 0}));
	grid.set(5, 1, cellWith({Inscription, NAME_CARTOUCHE_ID, 0}));
	grid.set(9, 1, cellWith({Door, GateTeleport, 3 | NAME_SEAL}));
	grid.set(14, 1, cellWith({Door, GateExit, 0}));
	grid.set(3, 5, cellWith({Door, GateTeleport, 3}));
	grid.set(5, 5, cellWith({Treasure, 3, 0}));
	return grid;
}

bool warns(const LevelReport& r, const std::string& text) {
	return std::any_of(r.warnings.begin(), r.warnings.end(), [&](const std::string& w) {
		return w.find(text) != std::string::npos;
	});
}

NameList shippedNames() {
	NameList names;
	REQUIRE(names.Load(CONTENT_DIR "names").empty());
	return names;
}
} // namespace

TEST_CASE("a name file: entries by name:, the spellings normalized, problems told by line") {
	std::istringstream in(
		"# comment\n"
		"name: Hor-Aha\n"
		"A: Hor-Aha\n"
		"A: aha\n"
		"N: Killed by a hippo.\n"
		"G: G5 D34\n"
		"\n"
		"name: Nobody\n"
		"name: Nefertiti\n"
		"title: queen\n"
		"A: nefertiti\n"
		"X: what\n"
	);
	std::vector<RoyalName> out;
	std::vector<std::string> errors;
	ParseNames(in, "f.txt", out, errors);
	REQUIRE(out.size() == 2);
	CHECK(out[0].name == "Hor-Aha");
	CHECK(out[0].title == "king");
	CHECK(out[0].answers == std::vector<std::string>{"horaha", "aha"});
	CHECK(out[0].note == "Killed by a hippo.");
	CHECK(out[0].glyphs == std::vector<std::string>{"G5", "D34"});
	CHECK(out[0].glyphSource == "f.txt:6");
	CHECK(out[1].glyphs.empty());
	CHECK(out[1].title == "queen");
	CHECK(out[0].Accepts("HOR AHA"));
	CHECK(out[0].Accepts("the Aha!"));
	CHECK_FALSE(out[0].Accepts("horus"));
	REQUIRE(errors.size() == 2);
	CHECK(errors[0] == "f.txt:8: name Nobody has no A: line, skipped");
	CHECK(errors[1].find("f.txt:12:") == 0);
}

TEST_CASE("the shipped names load with no problems, each with a spelling, enough for every campaign level") {
	const NameList names = shippedNames();
	CHECK(names.Size() >= static_cast<size_t>(CAMPAIGN_LEVELS));
	for (size_t i = 0; i < names.Size(); i++) {
		CAPTURE(names.At(i).name);
		CHECK_FALSE(names.At(i).answers.empty());
		CHECK(names.At(i).Accepts(names.At(i).name)); // the name as the journal writes it is taken
		// With "Something on this level wants to hear it." the comment keeps to the journal page's two lines.
		CHECK(names.At(i).note.size() <= 70);
	}
	// Spelling variants (the plan's).
	auto find = [&](const std::string& name) {
		for (size_t i = 0; i < names.Size(); i++)
			if (names.At(i).name == name)
				return &names.At(i);
		return static_cast<const RoyalName*>(nullptr);
	};
	REQUIRE(find("Ramesses") != nullptr);
	CHECK(find("Ramesses")->Accepts("Ramses"));
	REQUIRE(find("Amenhotep") != nullptr);
	CHECK(find("Amenhotep")->Accepts("amenophis"));
}

TEST_CASE("the name pick: stable for a seed, other seeds other names, no name twice in a run") {
	const NameList names = shippedNames();
	CHECK(names.ForLevel(7, 6) == names.ForLevel(7, 6));
	bool differs = false;
	for (uint32_t seed = 1; seed < 20; seed++)
		differs = differs || names.ForLevel(seed, 6) != names.ForLevel(0, 6);
	CHECK(differs);
	for (uint32_t seed : {0U, 1U, 12345U, 0xffffffffU}) {
		std::set<const RoyalName*> run;
		for (int level = 1; level <= CAMPAIGN_LEVELS; level++)
			run.insert(names.ForLevel(seed, level));
		CHECK(run.size() == static_cast<size_t>(CAMPAIGN_LEVELS));
	}
	CHECK(NameList{}.ForLevel(1, 6) == nullptr);
}

TEST_CASE("the run's name seed is kept by a save, an old save has seed 0") {
	RunStats run;
	run.nameSeed = 987654321U;
	std::stringstream file;
	run.Dump(file);
	file << "NEXT\n";
	RunStats loaded;
	loaded.LoadDump(file);
	CHECK(loaded.nameSeed == 987654321U);
	std::string next;
	file >> next;
	CHECK(next == "NEXT");

	std::stringstream old("RUN 1 0 0 0 0 0 0 0 0 0 0 0\nNEXT\n");
	RunStats before;
	before.nameSeed = 5;
	before.LoadDump(old);
	CHECK(before.nameSeed == 0U);
	old >> next;
	CHECK(next == "NEXT");
}

TEST_CASE("the name gate: asks for the name, stays sealed until it is spoken, then teleports; a save keeps it open") {
	SimWorld world;
	world.player.god = true;
	world.names = shippedNames();
	world.player.stats.run.nameSeed = 42;
	world.dungeon.LoadGrid(nameGateLevel(), "name-gate");
	world.player.Reanimate();
	world.WaitTicks(10);
	const RoyalName* name = world.dungeon.LevelName();
	REQUIRE(name != nullptr);

	// The cartouche without the skill: a copy, the name already in it for later.
	REQUIRE(world.WalkTo(5.5f));
	CHECK(world.dungeon.InteractVerb() == Interaction::Copy);
	world.Interact();
	world.WaitTicks(1);
	REQUIRE(world.journal.Inscriptions().size() == 1);
	const JournalInscription& copy = world.journal.Inscriptions()[0];
	CHECK(copy.id == NAME_CARTOUCHE_ID);
	CHECK_FALSE(copy.translated);
	CHECK(copy.glyphs == name->glyphs); // the cartouche spells the name
	REQUIRE(copy.text.size() == 1);
	std::string upper = name->name;
	for (char& c : upper)
		c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
	CHECK(copy.text[0] == "The name of the " + name->title + ": " + upper);
	std::stringstream page;
	world.journal.Dump(page);
	Journal loaded;
	loaded.Load(page);
	REQUIRE(loaded.Inscriptions().size() == 1);
	CHECK(loaded.Inscriptions()[0].glyphs == copy.glyphs); // journal version 13
	CHECK(loaded.Inscriptions()[0].SameInscription(copy));
	JournalInscription otherLevel = copy;
	otherLevel.level = copy.level + 1;
	CHECK_FALSE(copy.SameInscription(otherLevel)); // one name cartouche per level, told apart by it

	REQUIRE(world.WalkTo(9.5f));
	CHECK(world.dungeon.InteractVerb() == Interaction::Answer);
	world.Interact();
	world.WaitTicks(world.GATE_TRAVEL_TICKS);
	CHECK(std::any_of(world.said.begin(), world.said.end(), [](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::AskName;
	}));
	CHECK(static_cast<int>(world.Y()) == 1); // no teleport while sealed
	CHECK(isNameGate(world.dungeon.Cell(9, 1)));

	REQUIRE(world.dungeon.SpeakName());
	CHECK_FALSE(isNameGate(world.dungeon.Cell(9, 1)));
	CHECK(isTeleporter(world.dungeon.Cell(9, 1)));
	CHECK_FALSE(world.dungeon.SpeakName()); // once
	CHECK(world.dungeon.InteractVerb() == Interaction::Enter);
	world.Interact();
	world.WaitTicks(world.GATE_TRAVEL_TICKS);
	CHECK(static_cast<int>(world.Y()) == 5);
	CHECK(static_cast<int>(world.X()) == 3);

	// The dungeon's part of a save keeps it open (as for every gate's state).
	const std::string path = "build/name_gate.sav";
	{
		std::ofstream out(path);
		world.dungeon.Dump(out);
	}
	world.dungeon.LoadGrid(nameGateLevel(), "name-gate");
	CHECK(isNameGate(world.dungeon.Cell(9, 1)));
	std::ifstream in(path);
	REQUIRE(world.dungeon.LoadDump(in));
	CHECK_FALSE(isNameGate(world.dungeon.Cell(9, 1)));
	CHECK(isTeleporter(world.dungeon.Cell(9, 1)));
}

TEST_CASE("the checker: a good name gate level has no warnings, things needed behind the gate do") {
	CHECK(checkLevel(nameGateLevel()).warnings.empty());
	CHECK(checkLevel(nameGateLevel()).nameGates == 1);
	CHECK(checkLevel(nameGateLevel()).nameCartouches == 1);

	LevelGrid exitBehind = nameGateLevel();
	exitBehind.set(14, 1, Tile{});
	exitBehind.set(6, 5, cellWith({Door, GateExit, 0}));
	const LevelReport r = checkLevel(exitBehind);
	CHECK(r.valid); // it can be finished, by those who know the name
	CHECK(warns(r, "the exit at (col 6, row 5) is behind a name gate"));

	LevelGrid keyBehind = nameGateLevel();
	keyBehind.set(6, 5, cellWith({Key, 1, 0}));
	keyBehind.set(12, 1, cellWith({Gate, 1, 0}));
	CHECK(warns(checkLevel(keyBehind), "the key at (col 6, row 5) is behind a name gate"));

	LevelGrid noCartouche = nameGateLevel();
	noCartouche.set(5, 1, Tile{});
	CHECK(warns(checkLevel(noCartouche), "a name gate without a name cartouche"));

	LevelGrid cartoucheBehind = nameGateLevel();
	cartoucheBehind.set(5, 1, Tile{});
	cartoucheBehind.set(6, 5, cellWith({Inscription, NAME_CARTOUCHE_ID, 0}));
	CHECK(warns(checkLevel(cartoucheBehind), "is not found with the name gate sealed"));

	LevelGrid cartoucheNear = nameGateLevel();
	cartoucheNear.set(5, 1, Tile{});
	cartoucheNear.set(8, 1, cellWith({Inscription, NAME_CARTOUCHE_ID, 0}));
	CHECK(warns(checkLevel(cartoucheNear), "is next to its name gate"));

	LevelGrid two = nameGateLevel();
	two.set(11, 1, cellWith({Door, GateTeleport, 4 | NAME_SEAL}));
	two.set(6, 5, cellWith({Door, GateTeleport, 4}));
	CHECK(warns(checkLevel(two), "2 name gates, a level has at most one"));

	LevelGrid lone = nameGateLevel();
	lone.set(9, 1, Tile{});
	CHECK(warns(checkLevel(lone), "a name cartouche without a name gate"));
}

TEST_CASE("the checker: the name gate is reached back from the name cartouche (plan name-gate-return)") {
	// Row 3: the entrance (col 1), the name gate (4, pair 3). A shaft at col 9 down to row 1: the cartouche (12), the
	// exit (14). The pocket with the partner on row 5.
	auto level = [](bool ladder) {
		LevelGrid grid;
		for (int col = 1; col <= 9; col++)
			grid.set(col, 3, Tile{});
		for (int row = 1; row <= 3; row++)
			grid.set(9, row, ladder ? Tile{Ladder, 0, 0} : Tile{});
		for (int col = 10; col <= 15; col++)
			grid.set(col, 1, Tile{});
		for (int col = 2; col <= 6; col++)
			grid.set(col, 5, Tile{});
		grid.set(1, 3, cellWith({Door, GateEntrance, 0}));
		grid.set(4, 3, cellWith({Door, GateTeleport, 3 | NAME_SEAL}));
		grid.set(12, 1, cellWith({Inscription, NAME_CARTOUCHE_ID, 0}));
		grid.set(14, 1, cellWith({Door, GateExit, 0}));
		grid.set(3, 5, cellWith({Door, GateTeleport, 3}));
		grid.set(5, 5, cellWith({Treasure, 3, 0}));
		return grid;
	};
	CHECK(warns(
		checkLevel(level(false)),
		"the name gate at (col 4, row 3) cannot be reached back from the name cartouche at (col 12, row 1)"
	));
	CHECK(checkLevel(level(true)).warnings.empty());
}

TEST_CASE("the campaign's secret rooms: about every second level, each name gate with its cartouche") {
	int rooms = 0;
	for (int n = 1; n <= CAMPAIGN_LEVELS; n++) {
		LevelGrid grid;
		REQUIRE(loadLevelFile(campaignLevelFile(n).c_str(), grid).empty());
		const LevelReport r = checkLevel(grid);
		CAPTURE(n);
		CHECK(r.nameGates <= 1);
		CHECK(r.nameCartouches == r.nameGates);
		rooms += r.nameGates;
	}
	CHECK(rooms >= 12);
	CHECK(static_cast<size_t>(rooms) <= shippedNames().Size());
}
