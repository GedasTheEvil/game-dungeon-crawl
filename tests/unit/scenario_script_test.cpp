// The scenario script parser (src/test/scenario_script.h): every command parses, bad lines give an error.
#include "../../external/doctest/doctest.h"
#include "../../src/core/gameplay_config.h"
#include "../../src/test/scenario_script.h"
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace Scenario;

namespace {
// A good line per command: a new command needs one here.
const std::map<std::string, std::string> EXAMPLES = {
	{"resolution", "resolution 800 600"},
	{"seed", "seed 7"},
	{"god", "god"},
	{"level", "level 3"},
	{"wait", "wait 2s"},
	{"walk", "walk right 2.5"},
	{"hold", "hold left 500ms"},
	{"sprint", "sprint on"},
	{"motion", "motion off"},
	{"jump", "jump"},
	{"attack", "attack"},
	{"interact", "interact"},
	{"camera", "camera 10 -5"},
	{"toon", "toon on"},
	{"hitboxes", "hitboxes off"},
	{"screenshot", "screenshot start"},
	{"dump", "dump"},
	{"expect", "expect hp >= 10"},
	{"key", "key i"},
	{"give", "give potion 2 3"},
	{"chest", "chest melee 1"},
	{"select", "select small_health"},
	{"sort", "sort strength"},
	{"equip", "equip spear"},
	{"wear", "wear amulet_of_health"},
	{"xp", "xp 1000"},
	{"hurt", "hurt 5"},
	{"poison", "poison medium"},
	{"poisonmonster", "poisonmonster weak"},
	{"riddles", "riddles tests/riddles"},
	{"prop", "prop 4 cat"},
	{"savegame", "savegame one.sav"},
	{"loadgame", "loadgame one.sav"},
	{"mouse", "mouse 50 50"},
	{"press", "press 10 90"},
	{"release", "release 10 90"},
	{"click", "click 10 90"},
	{"killboss", "killboss"},
	{"hurtboss", "hurtboss 100"},
	{"poisonboss", "poisonboss strong"},
	{"quit", "quit"},
};

Command parsed(const std::string& line) {
	Command cmd;
	const std::string error = parseCommand(line, cmd);
	CHECK_MESSAGE(error.empty(), line, ": ", error);
	return cmd;
}

std::string errorOf(const std::string& line) {
	Command cmd;
	return parseCommand(line, cmd);
}

std::vector<std::string> scriptErrors(const std::string& text, Script& script) {
	std::istringstream in(text);
	return parseScript(in, "test.txt", script);
}
} // namespace

TEST_CASE("every command parses its example and refuses bad arguments") {
	for (const CommandDef& def : commandDefs()) {
		CAPTURE(def.name);
		const auto example = EXAMPLES.find(def.name);
		REQUIRE_MESSAGE(example != EXAMPLES.end(), "no example for ", def.name);
		CHECK(parsed(example->second).type == def.type);
		CHECK(errorOf(std::string(def.name) + " x x x x").find(def.name) != std::string::npos);
	}
	CHECK(EXAMPLES.size() == commandDefs().size());
}

TEST_CASE("command names and field names are unique") {
	std::map<std::string, int> names;
	for (const CommandDef& def : commandDefs())
		names[def.name]++;
	for (const auto& [name, count] : names)
		CHECK_MESSAGE(count == 1, name);
	std::map<std::string, int> fields;
	for (const FieldDef& def : fieldDefs())
		fields[def.name]++;
	for (const auto& [name, count] : fields)
		CHECK_MESSAGE(count == 1, name);
}

TEST_CASE("arguments land in the command") {
	Command wait = parsed("wait 2s # a comment");
	CHECK(wait.ticks == (2000 + UPDATE_TICK_MS - 1) / UPDATE_TICK_MS);
	CHECK(wait.text == "wait 2s");
	CHECK(parsed("wait 7").ticks == 7);

	Command walk = parsed("walk up 3");
	CHECK(walk.move == Move::Up);
	CHECK(walk.a == doctest::Approx(3.f));
	Command walkTo = parsed("walk to 12.5");
	CHECK(walkTo.walkTo);
	CHECK(walkTo.a == doctest::Approx(12.5f));

	Command level = parsed("level gen:7:3");
	CHECK(level.arg == "gen:7:3");
	CHECK(level.a == 0.f);
	CHECK(parsed("level 12").a == doctest::Approx(12.f));

	Command expect = parsed("expect potion2.level == 2");
	CHECK(expect.field == Field::ItemLevel);
	CHECK(expect.item == ItemKind::Might);
	CHECK(parsed("expect melee1 > 0").field == Field::ItemCount);
	CHECK(parsed("expect journal_solved != 1").op == Op::Ne);

	Command key = parsed("key esc");
	CHECK(key.a == doctest::Approx(27.f));
	CHECK(key.b == 0.f);
	Command special = parsed("key f12");
	CHECK(special.b == doctest::Approx(1.f));

	CHECK(parsed("give amulet 9").item == ItemKind::HealthMinor);
	CHECK(parsed("give potion 0").ticks == 1);
	CHECK(parsed("wear off").item == ItemKind::Club);
	CHECK(parsed("poison strong").ticks == 2);
	CHECK(parsed("sprint on").a == doctest::Approx(1.f));
}

TEST_CASE("bad lines say what is wrong") {
	CHECK(errorOf("dance") == "unknown command 'dance'");
	CHECK(errorOf("god mode") == "god takes no arguments");
	CHECK(errorOf("wait soon") == "usage: wait <ticks|Nms|Ns>");
	CHECK_FALSE(errorOf("walk sideways 2").empty());
	CHECK_FALSE(errorOf("walk left 0").empty());
	CHECK_FALSE(errorOf("expect mana == 1").empty());
	CHECK_FALSE(errorOf("expect hp =< 1").empty());
	CHECK_FALSE(errorOf("expect potion99 == 1").empty());
	CHECK_FALSE(errorOf("give potion 99").empty());
	CHECK_FALSE(errorOf("equip small_health").empty()); // not a weapon
	CHECK_FALSE(errorOf("wear club").empty());			// not an amulet
	CHECK_FALSE(errorOf("screenshot a/b").empty());
	CHECK_FALSE(errorOf("hurtboss 0").empty());
	CHECK_FALSE(errorOf("resolution 32 32").empty());
	CHECK_FALSE(errorOf("prop 3 dragon").empty());
	CHECK_FALSE(errorOf("key").empty());
	CHECK(errorOf("   # only a comment") == "empty line");
}

TEST_CASE("a script: setup first, then level, then the rest") {
	Script script;
	CHECK(scriptErrors("resolution 640 480\nseed 5\n\n# comment\ngod\nlevel 1\nexpect alive == 0\n", script).empty());
	CHECK(script.commands.size() == 5);
	CHECK(script.resX == 640);
	CHECK(script.resY == 480);
	CHECK(script.seed == 5);
	CHECK(script.deathExpected);
	CHECK(script.commands[3].line == 6);

	Script early;
	const std::vector<std::string> errors = scriptErrors("jump\nlevel 1\nresolution 800 600\nwait x\n", early);
	REQUIRE(errors.size() == 3);
	CHECK(errors[0] == "test.txt:1: 'level' must come before gameplay commands");
	CHECK(errors[1] == "test.txt:3: 'resolution' must come before 'level'");
	CHECK(errors[2] == "test.txt:4: usage: wait <ticks|Nms|Ns>");

	Script none;
	CHECK(scriptErrors("seed 1\n", none) == std::vector<std::string>{"test.txt: no 'level' command"});
}

TEST_CASE("expect compares within a thousandth for == and !=") {
	CHECK(compare(1.0004f, Op::Eq, 1.f));
	CHECK_FALSE(compare(1.01f, Op::Eq, 1.f));
	CHECK(compare(1.01f, Op::Ne, 1.f));
	CHECK(compare(1.f, Op::Le, 1.f));
	CHECK_FALSE(compare(1.f, Op::Lt, 1.f));
	CHECK(compare(2.f, Op::Gt, 1.f));
	CHECK(compare(1.f, Op::Ge, 1.f));
}
