#ifndef SCENARIO_SCRIPT_H
#define SCENARIO_SCRIPT_H

// Scenario scripts as text: the commands, the `expect` fields and the parser, without the game (scenario.cpp runs
// them). Command reference: docs/testing.md; the unit tests check it lists every row of COMMANDS and FIELDS.

#include "../world/items.h"
#include <istream>
#include <string>
#include <vector>

namespace Scenario {

enum class CommandType : unsigned char {
	Resolution,
	Seed,
	God,
	Level,
	Wait,
	Walk,
	Hold,
	Sprint,
	Motion,
	Jump,
	Attack,
	Interact,
	Camera,
	Toon,
	Hitboxes,
	Screenshot,
	Dump,
	Expect,
	Key,
	Give,
	Chest,
	Select,
	Equip,
	Wear,
	Xp,
	Hurt,
	Poison,
	PoisonMonster,
	Riddles,
	Prop,
	SaveGame,
	LoadGame,
	Mouse,
	Press,
	Release,
	Click,
	KillBoss,
	HurtBoss,
	PoisonBoss,
	Quit,
};

// What `expect` reads (scenario_fields.cpp).
enum class Field : unsigned char {
	X,
	Y,
	Hp,
	Stamina,
	Level,
	Alive,
	Won,
	Might,
	Armor,
	EquipType,
	EquipId,
	Keys,
	Poison,
	XpTotal,
	Riddle,
	Bars,
	Boss,
	Minions,
	Attacking,
	DecorTier,
	Nearest,
	NearestPoison,
	Coffins,
	JournalRiddles,
	JournalSolved,
	JournalNotes,
	JournalTried,
	Chests,
	ItemCount, // written as the item's type + id: potion2, melee0
	ItemLevel, // the same + ".level"
	MaxHp,
	Worn,
	Safe
};
enum class Op : unsigned char { Eq, Ne, Lt, Le, Gt, Ge };

// A walk key: walk, hold.
enum class Move : unsigned char { None, Left, Right, Down, Up };

struct Command {
	CommandType type = CommandType::Quit;
	int line = 0;
	std::string text; // source line, for reports
	std::string arg;  // level path / screenshot name
	int ticks = 0;	  // wait, hold
	Move move = Move::None;
	bool walkTo = false;			// walk to: a is the target map x
	float a = 0.f;					// walk distance, camera rotM, expect value
	float b = 0.f;					// camera rotN
	ItemKind item = ItemKind::Club; // give / chest / expect count
	Field field = Field::X;
	Op op = Op::Eq;
};

// One row per command: a new command is a row here (and its run in scenario.cpp, which -Wswitch asks for).
struct CommandDef {
	const char* name;
	CommandType type;
	// Reads the words after the name into cmd; false on bad arguments.
	bool (*parse)(const std::vector<std::string>& args, Command& cmd);
	const char* usage; // the arguments, shown as "usage: <name> <usage>" on bad ones
};
[[nodiscard]] const std::vector<CommandDef>& commandDefs();

struct FieldDef {
	const char* name;
	Field field;
};
// The named fields; the item counts (ItemCount, ItemLevel) are not in here.
[[nodiscard]] const std::vector<FieldDef>& fieldDefs();

// One script line (a comment from '#' on is dropped) into cmd: the error message, empty on success. An empty line
// is an error ("empty line"), scripts skip them.
std::string parseCommand(const std::string& line, Command& cmd);

// lhs OP rhs; == and != within 0.001.
[[nodiscard]] bool compare(float lhs, Op op, float rhs);

struct Script {
	std::vector<Command> commands;
	int resX = 1280;
	int resY = 720;
	unsigned int seed = 1;
	bool deathExpected = false; // the script has `expect alive == 0`
};
// The whole script, with the order rules (setup commands, then `level`, then the rest). The errors, one
// "<name>:<line>: <message>" per bad line, or "<name>: no 'level' command"; empty on success.
std::vector<std::string> parseScript(std::istream& in, const std::string& name, Script& script);

} // namespace Scenario

#endif
