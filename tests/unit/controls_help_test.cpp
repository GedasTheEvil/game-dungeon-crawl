// How to Play and the first hints (plan controls-help): the page lists every action with its bound keys, a hint names
// the key bound now, the run keeps the hints shown in the save, and the dungeon tells a jump or a ladder ahead.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/level/controls_help.h"
#include "../../src/level/run_stats.h"
#include "../../src/level/settings_ini.h"
#include <sstream>

namespace {
bool listed(const std::vector<HelpGroup>& groups, const std::string& keys) {
	for (const HelpGroup& g : groups)
		for (const HelpRow& r : g.rows)
			if (r.keys.find(keys) != std::string::npos)
				return true;
	return false;
}

// tests/levels/first_hints: the start floor's row, a gap at cols 5-6, a ladder at col 11, a wall at col 14.
constexpr int GAP_COL = 5;
constexpr int LADDER_COL = 11;
constexpr int LEDGE_COL = 14;
} // namespace

TEST_CASE("How to Play: every action's first key is on the page, with the fixed keys") {
	const Bindings bindings;
	const std::vector<HelpGroup> groups = controlsHelp(bindings);
	REQUIRE(groups.size() == 5);
	for (int i = 0; i < BIND_ACTION_COUNT; i++) {
		const InputKey* key = bindings.Of(static_cast<BindAction>(i)).First();
		REQUIRE(key != nullptr);
		CHECK_MESSAGE(listed(groups, keyCap(*key)), bindActionName(static_cast<BindAction>(i)));
	}
	CHECK(listed(groups, "F5"));
	CHECK(listed(groups, "F9"));
	CHECK(listed(groups, "Esc"));
	CHECK_FALSE(listed(groups, "F3")); // a debug key, only with --debug
}

TEST_CASE("How to Play: the page follows a rebinding, an unbound action shows a dash") {
	Bindings bindings;
	bindings.Bind(BindAction::Jump, 0, InputKey::Char('x'));
	bindings.Clear(BindAction::Jump, MOUSE_SLOT);
	CHECK(bindingCaps(bindings.Of(BindAction::Jump)) == "X");
	CHECK(bindingCaps(Binding{}) == "-");
	CHECK(bindingCaps(Bindings{}.Of(BindAction::Attack)) == "V / Enter / LMB");
}

TEST_CASE("first hints: the text names the key bound now") {
	Bindings bindings;
	CHECK(hintText(FirstHint::Jump, bindings) == "Jump: Space or RMB");
	CHECK(hintText(FirstHint::Climb, bindings) == "Climb: W up, S down");
	CHECK(hintText(FirstHint::Sprint, bindings) == "Sprint: Shift (hold, uses stamina)");
	CHECK(hintText(FirstHint::PowerHit, bindings) == "Power hit: hold V or LMB, let go to strike");
	CHECK(hintText(FirstHint::QuickHeal, bindings) == "Drink a healing potion: 3");
	bindings.Bind(BindAction::Jump, 0, InputKey::Char('x'));
	bindings.Clear(BindAction::Jump, MOUSE_SLOT);
	CHECK(hintText(FirstHint::Jump, bindings) == "Jump: X");
	bindings.Clear(BindAction::Jump, 0);
	CHECK(hintText(FirstHint::Jump, bindings) == "Jump: bind a key in Options > Controls");
}

TEST_CASE("first hints: the run keeps the hints shown in the save; an older save has none") {
	RunStats run;
	run.hints = hintBit(FirstHint::Jump) | hintBit(FirstHint::QuickAntidote);
	std::stringstream out;
	run.Dump(out);
	RunStats back;
	back.LoadDump(out);
	CHECK(back.hints == run.hints);

	std::string old = out.str();
	old = old.substr(0, old.find("HINTS"));
	std::istringstream in(old);
	back.LoadDump(in);
	CHECK(back.hints == 0);
}

TEST_CASE("first hints: [gameplay] hints is on by default and reads off") {
	std::vector<std::string> warnings;
	std::istringstream empty("");
	CHECK(parseSettings(empty, warnings).gameplay.hints);
	std::istringstream off("[gameplay]\nhints = off\n");
	const Settings read = parseSettings(off, warnings);
	CHECK_FALSE(read.gameplay.hints);
	CHECK(warnings.empty());
	std::istringstream written(writeSettings(read));
	CHECK_FALSE(parseSettings(written, warnings).gameplay.hints);
}

TEST_CASE("first hints: the dungeon tells a gap or a ledge ahead and a ladder at hand") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/first_hints"));
	const int row = static_cast<int>(world.Y());
	CHECK_FALSE(world.dungeon.PlayerFacesJump(1)); // the start: open floor ahead
	REQUIRE(world.dungeon.PlacePlayer(GAP_COL - 1, row));
	REQUIRE(world.WalkTo(static_cast<float>(GAP_COL) - 0.4f));
	CHECK(world.dungeon.PlayerFacesJump(1));
	CHECK_FALSE(world.dungeon.PlayerFacesJump(-1));
	CHECK_FALSE(world.dungeon.PlayerAtLadder());

	REQUIRE(world.dungeon.PlacePlayer(LADDER_COL, row));
	world.Wait(100);
	CHECK(world.dungeon.PlayerAtLadder());

	REQUIRE(world.dungeon.PlacePlayer(LEDGE_COL - 1, row));
	REQUIRE(world.WalkTo(static_cast<float>(LEDGE_COL) - 0.4f));
	CHECK(world.dungeon.PlayerFacesJump(1));
}
