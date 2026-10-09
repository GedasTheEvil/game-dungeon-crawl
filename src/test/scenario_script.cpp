#include "scenario_script.h"
#include "../core/gameplay_config.h"
#include "../input/bindings.h"
#include "../input/input.h"
#include "../world/decor.h"
#include "../world/item_bag.h"
#include "../world/poison.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <sstream>

namespace Scenario {
namespace {
using Args = std::vector<std::string>;

bool parseFloat(const std::string& word, float& out) {
	char* end = nullptr;
	out = strtof(word.c_str(), &end);
	return end != word.c_str() && *end == '\0';
}

// Ticks, or a time: "500ms", "2s" (rounded up to whole ticks).
bool parseWait(const std::string& word, int& ticks) {
	char* end = nullptr;
	double value = strtod(word.c_str(), &end);
	std::string unit = end;
	if (end == word.c_str() || value < 0)
		return false;

	double ms = 0;
	if (unit.empty()) {
		ticks = static_cast<int>(value);
		return true;
	}
	if (unit == "ms")
		ms = value;
	else if (unit == "s")
		ms = value * 1000.0;
	else
		return false;
	ticks = static_cast<int>(std::ceil(ms / UPDATE_TICK_MS));
	return true;
}

// "melee", "ranged", "potion" or "amulet" -> ItemType value.
bool parseItemType(const std::string& word, int& type) {
	if (word == "melee")
		type = ItemType::MELEE_WEAPON;
	else if (word == "ranged")
		type = ItemType::RANGED_WEAPON;
	else if (word == "potion")
		type = ItemType::POTION;
	else if (word == "amulet")
		type = ItemType::AMULET;
	else
		return false;
	return true;
}

// The item's label with _ for its spaces: small_health, short_sword.
std::string itemSlug(ItemKind kind) {
	std::string slug = itemText(kind).label;
	std::replace(slug.begin(), slug.end(), ' ', '_');
	return slug;
}

// The item among itemAt(first) .. itemAt(end - 1) whose slug is word.
bool parseItemSlug(const std::string& word, int first, int end, ItemKind& item) {
	for (int i = first; i < end; i++)
		if (word == itemSlug(itemAt(i))) {
			item = itemAt(i);
			return true;
		}
	return false;
}

// Item counts are written as the type followed by the id: "potion2", "melee0"; ".level" gives the item level.
bool parseItemCountField(const std::string& word, Command& cmd) {
	size_t digits = word.find_first_of("0123456789");
	int type = 0;
	if (digits == std::string::npos || digits == 0 || !parseItemType(word.substr(0, digits), type))
		return false;
	char* end = nullptr;
	std::optional<ItemKind> item = itemFromFile(type, static_cast<int>(strtol(word.c_str() + digits, &end, 10)));
	if (!item)
		return false;
	cmd.item = *item;
	std::string suffix = end;
	if (suffix.empty())
		cmd.field = Field::ItemCount;
	else if (suffix == ".level")
		cmd.field = Field::ItemLevel;
	else
		return false;
	return true;
}

bool parseField(const std::string& word, Field& field) {
	for (const FieldDef& def : fieldDefs())
		if (word == def.name) {
			field = def.field;
			return true;
		}
	return false;
}

bool parseOp(const std::string& word, Op& op) {
	static const struct {
		const char* name;
		Op op;
	} OPS[] = {{"==", Op::Eq}, {"!=", Op::Ne}, {"<", Op::Lt}, {"<=", Op::Le}, {">", Op::Gt}, {">=", Op::Ge}};
	for (const auto& entry : OPS)
		if (word == entry.name) {
			op = entry.op;
			return true;
		}
	return false;
}

Move parseMove(const std::string& word) {
	return word == "left"	 ? Move::Left
		   : word == "right" ? Move::Right
		   : word == "up"	 ? Move::Up
		   : word == "down"	 ? Move::Down
							 : Move::None;
}

// ---- argument parsers, one per argument shape --------------------------------

bool noArgs(const Args& args, Command& /*cmd*/) { return args.empty(); }

// One word, kept as is: a path, a file name.
bool oneWord(const Args& args, Command& cmd) {
	if (args.size() != 1)
		return false;
	cmd.arg = args[0];
	return true;
}

// on|off -> a = 1 / 0.
bool onOff(const Args& args, Command& cmd) {
	if (args.size() != 1 || (args[0] != "on" && args[0] != "off"))
		return false;
	cmd.a = args[0] == "on" ? 1.f : 0.f;
	return true;
}

// A number of at least `min` -> a.
template <int MIN> bool numberAtLeast(const Args& args, Command& cmd) {
	return args.size() == 1 && parseFloat(args[0], cmd.a) && cmd.a >= static_cast<float>(MIN);
}

// Two numbers -> a, b.
bool twoNumbers(const Args& args, Command& cmd) {
	return args.size() == 2 && parseFloat(args[0], cmd.a) && parseFloat(args[1], cmd.b);
}

bool parseResolution(const Args& args, Command& cmd) { return twoNumbers(args, cmd) && cmd.a >= 64 && cmd.b >= 64; }

bool parseLevel(const Args& args, Command& cmd) {
	if (args.size() != 1)
		return false;
	float number = 0.f;
	cmd.arg = args[0];
	cmd.a = parseFloat(args[0], number) ? number : 0.f;
	return true;
}

bool parseWaitArgs(const Args& args, Command& cmd) { return args.size() == 1 && parseWait(args[0], cmd.ticks); }

bool parseWalk(const Args& args, Command& cmd) {
	if (args.size() != 2)
		return false;
	if (args[0] == "to") { // walk to X: along the row to map x X, whichever way it is
		cmd.walkTo = true;
		return parseFloat(args[1], cmd.a);
	}
	cmd.move = parseMove(args[0]);
	return cmd.move != Move::None && parseFloat(args[1], cmd.a) && cmd.a > 0;
}

bool parseHold(const Args& args, Command& cmd) {
	if (args.size() != 2)
		return false;
	cmd.move = parseMove(args[0]);
	return cmd.move != Move::None && parseWait(args[1], cmd.ticks);
}

bool parseScreenshot(const Args& args, Command& cmd) {
	return oneWord(args, cmd) && cmd.arg.find_first_of("/\\") == std::string::npos;
}

bool parseExpect(const Args& args, Command& cmd) {
	return args.size() == 3 && (parseField(args[0], cmd.field) || parseItemCountField(args[0], cmd)) &&
		   parseOp(args[1], cmd.op) && parseFloat(args[2], cmd.a);
}

// A character, a named key (a), or a special key by its docs/settings.md name (a = its code, b = 1).
bool parseKey(const Args& args, Command& cmd) {
	if (args.size() != 1)
		return false;
	static const struct {
		const char* name;
		unsigned char key;
	} NAMED[] = {{"enter", KEY_ENTER}, {"esc", KEY_ESCAPE}, {"space", KEY_SPACE}, {"tab", '\t'}, {"backspace", '\b'}};
	for (const auto& entry : NAMED)
		if (args[0] == entry.name)
			cmd.a = entry.key;
	if (cmd.a == 0.f && args[0].size() == 1)
		cmd.a = static_cast<unsigned char>(args[0][0]);
	if (std::optional<InputKey> named = parseKeyName(args[0]);
		cmd.a == 0.f && named && named->kind == InputKey::Kind::Special) {
		cmd.a = static_cast<float>(named->code); // a special key: `left`, `f12`
		cmd.b = 1.f;
	}
	return cmd.a != 0.f;
}

// give / chest: an item type and id, a count (ticks, default 1).
bool parseItemCount(const Args& args, Command& cmd) {
	int type = 0;
	float id = 0.f;
	float count = 1.f;
	if (args.size() < 2 || args.size() > 3 || !parseItemType(args[0], type) || !parseFloat(args[1], id) ||
		(args.size() == 3 && !parseFloat(args[2], count)))
		return false;
	std::optional<ItemKind> item = itemFromFile(type, static_cast<int>(id));
	if (!item)
		return false;
	cmd.item = *item;
	cmd.ticks = static_cast<int>(count);
	return true;
}

bool parseSelect(const Args& args, Command& cmd) {
	return args.size() == 1 && parseItemSlug(args[0], 0, ITEM_KIND_COUNT, cmd.item);
}

// The order (ticks) by its name.
bool parseSort(const Args& args, Command& cmd) {
	static const char* const ORDERS[SORT_ORDER_COUNT] = {"found", "name", "strength", "recent"};
	for (int o = 0; args.size() == 1 && o < SORT_ORDER_COUNT; o++)
		if (args[0] == ORDERS[o]) {
			cmd.ticks = o;
			return true;
		}
	return false;
}

bool parseEquip(const Args& args, Command& cmd) {
	return args.size() == 1 && parseItemSlug(args[0], 0, WEAPON_KIND_COUNT, cmd.item);
}

bool parseWear(const Args& args, Command& cmd) {
	cmd.item = ItemKind::Club; // "off": the worn amulet comes off
	return args.size() == 1 && (args[0] == "off" || parseItemSlug(args[0], FIRST_AMULET, ITEM_KIND_COUNT, cmd.item));
}

bool parsePoison(const Args& args, Command& cmd) {
	static const char* const TIERS[POISON_TIER_COUNT] = {"weak", "medium", "strong"};
	for (int t = 0; args.size() == 1 && t < POISON_TIER_COUNT; t++)
		if (args[0] == TIERS[t]) {
			cmd.ticks = t;
			return true;
		}
	return false;
}

// A column (a) and a prop of DECOR_NAMES (ticks).
bool parseProp(const Args& args, Command& cmd) {
	for (int d = 0; args.size() == 2 && parseFloat(args[0], cmd.a) && d < DECOR_COUNT; d++)
		if (args[1] == DECOR_NAMES[d]) {
			cmd.ticks = d;
			return true;
		}
	return false;
}

bool isSetupCommand(CommandType type) {
	return type == CommandType::Resolution || type == CommandType::Seed || type == CommandType::God;
}
} // namespace

const std::vector<CommandDef>& commandDefs() {
	static const std::vector<CommandDef> DEFS = {
		{"resolution", CommandType::Resolution, parseResolution, "<width> <height>"},
		{"seed", CommandType::Seed, numberAtLeast<0>, "<non-negative integer>"},
		{"god", CommandType::God, noArgs, ""},
		{"level", CommandType::Level, parseLevel, "<number|path|gen:SEED:DIFFICULTY>"},
		{"wait", CommandType::Wait, parseWaitArgs, "<ticks|Nms|Ns>"},
		{"walk", CommandType::Walk, parseWalk, "<left|right|up|down> <tiles> | walk to <map x>"},
		{"hold", CommandType::Hold, parseHold, "<left|right|up|down> <ticks|Nms|Ns>"},
		{"sprint", CommandType::Sprint, onOff, "<on|off>"},
		{"motion", CommandType::Motion, onOff, "<on|off>"},
		{"jump", CommandType::Jump, noArgs, ""},
		{"attack", CommandType::Attack, noArgs, ""},
		{"interact", CommandType::Interact, noArgs, ""},
		{"camera", CommandType::Camera, twoNumbers, "<rotM> <rotN>"},
		{"toon", CommandType::Toon, onOff, "<on|off>"},
		{"hitboxes", CommandType::Hitboxes, onOff, "<on|off>"},
		{"screenshot", CommandType::Screenshot, parseScreenshot, "<name> (no slashes)"},
		{"dump", CommandType::Dump, noArgs, ""},
		{"expect", CommandType::Expect, parseExpect, "<field|<item><id>[.level]> <==|!=|<|<=|>|>=> <number>"},
		{"key", CommandType::Key, parseKey, "<char|enter|esc|space|tab|backspace|special key name>"},
		{"give", CommandType::Give, parseItemCount, "<melee|ranged|potion|amulet> <id> [count], with a known id"},
		{"chest", CommandType::Chest, parseItemCount, "<melee|ranged|potion|amulet> <id> [count], with a known id"},
		{"select", CommandType::Select, parseSelect,
		 "<item>: club, short_sword, small_health, ... (its label, _ for spaces)"},
		{"sort", CommandType::Sort, parseSort, "<found|name|strength|recent>"},
		{"equip", CommandType::Equip, parseEquip, "<weapon>: club, short_sword, spear, self-bow, ..."},
		{"wear", CommandType::Wear, parseWear, "<amulet>|off: lesser_amulet_of_strength, amulet_of_health, ..."},
		{"xp", CommandType::Xp, numberAtLeast<0>, "<non-negative number>"},
		{"hurt", CommandType::Hurt, numberAtLeast<0>, "<hp>"},
		{"poison", CommandType::Poison, parsePoison, "<weak|medium|strong>"},
		{"poisonmonster", CommandType::PoisonMonster, parsePoison, "<weak|medium|strong>"},
		{"riddles", CommandType::Riddles, oneWord, "<file|directory>"},
		{"prop", CommandType::Prop, parseProp, "<col> <name>: a prop of DECOR_NAMES (web, pottery, ...)"},
		{"savegame", CommandType::SaveGame, oneWord, "<path>"},
		{"loadgame", CommandType::LoadGame, oneWord, "<path>"},
		{"mouse", CommandType::Mouse, twoNumbers, "<x%> <y%> (0..100, y from the bottom)"},
		{"press", CommandType::Press, twoNumbers, "<x%> <y%> (0..100, y from the bottom)"},
		{"release", CommandType::Release, twoNumbers, "<x%> <y%> (0..100, y from the bottom)"},
		{"click", CommandType::Click, twoNumbers, "<x%> <y%> (0..100, y from the bottom)"},
		{"killboss", CommandType::KillBoss, noArgs, ""},
		{"hurtboss", CommandType::HurtBoss, numberAtLeast<1>, "<hp>"},
		{"poisonboss", CommandType::PoisonBoss, parsePoison, "<weak|medium|strong>"},
		{"quit", CommandType::Quit, noArgs, ""},
	};
	return DEFS;
}

const std::vector<FieldDef>& fieldDefs() {
	static const std::vector<FieldDef> DEFS = {{"x", Field::X},
											   {"y", Field::Y},
											   {"hp", Field::Hp},
											   {"stamina", Field::Stamina},
											   {"level", Field::Level},
											   {"alive", Field::Alive},
											   {"won", Field::Won},
											   {"might", Field::Might},
											   {"armor", Field::Armor},
											   {"equip_type", Field::EquipType},
											   {"equip_id", Field::EquipId},
											   {"keys", Field::Keys},
											   {"poison", Field::Poison},
											   {"resist", Field::Resist},
											   {"xp", Field::XpTotal},
											   {"riddle", Field::Riddle},
											   {"bars", Field::Bars},
											   {"boss", Field::Boss},
											   {"minions", Field::Minions},
											   {"attacking", Field::Attacking},
											   {"decor_tier", Field::DecorTier},
											   {"nearest", Field::Nearest},
											   {"nearest_poison", Field::NearestPoison},
											   {"coffins", Field::Coffins},
											   {"chests", Field::Chests},
											   {"journal", Field::JournalRiddles},
											   {"journal_solved", Field::JournalSolved},
											   {"journal_notes", Field::JournalNotes},
											   {"journal_tried", Field::JournalTried},
											   {"maxhp", Field::MaxHp},
											   {"worn", Field::Worn},
											   {"safe", Field::Safe},
											   {"tab", Field::Tab},
											   {"selected", Field::Selected}};
	return DEFS;
}

std::string parseCommand(const std::string& line, Command& cmd) {
	std::istringstream words(line.substr(0, line.find('#')));
	std::vector<std::string> w;
	for (std::string word; words >> word;)
		w.push_back(word);
	if (w.empty())
		return "empty line";
	cmd.text.clear();
	for (size_t i = 0; i < w.size(); i++)
		cmd.text += (i ? " " : "") + w[i];

	for (const CommandDef& def : commandDefs()) {
		if (w[0] != def.name)
			continue;
		cmd.type = def.type;
		if (def.parse(Args(w.begin() + 1, w.end()), cmd))
			return "";
		if (def.usage[0] == '\0')
			return w[0] + " takes no arguments";
		return std::string("usage: ") + def.name + " " + def.usage;
	}
	return "unknown command '" + w[0] + "'";
}

bool compare(float lhs, Op op, float rhs) {
	switch (op) {
	case Op::Eq:
		return std::fabs(lhs - rhs) < 0.001f;
	case Op::Ne:
		return std::fabs(lhs - rhs) >= 0.001f;
	case Op::Lt:
		return lhs < rhs;
	case Op::Le:
		return lhs <= rhs;
	case Op::Gt:
		return lhs > rhs;
	case Op::Ge:
		return lhs >= rhs;
	}
	return false;
}

std::vector<std::string> parseScript(std::istream& in, const std::string& name, Script& script) {
	std::vector<std::string> errors;
	bool levelSeen = false;
	std::string raw;
	for (int lineNo = 1; std::getline(in, raw); lineNo++) {
		Command cmd;
		cmd.line = lineNo;
		std::string error = parseCommand(raw, cmd);
		if (cmd.text.empty())
			continue; // a blank line or a comment
		if (error.empty() && !levelSeen && !isSetupCommand(cmd.type) && cmd.type != CommandType::Level)
			error = "'level' must come before gameplay commands";
		if (error.empty() && levelSeen && cmd.type == CommandType::Resolution)
			error = "'resolution' must come before 'level'";
		if (!error.empty()) {
			std::string where = name;
			where += ":" + std::to_string(lineNo) + ": ";
			errors.push_back(where + error);
			continue;
		}

		if (cmd.type == CommandType::Level)
			levelSeen = true;
		if (cmd.type == CommandType::Resolution) {
			script.resX = static_cast<int>(cmd.a);
			script.resY = static_cast<int>(cmd.b);
		}
		if (cmd.type == CommandType::Seed)
			script.seed = static_cast<unsigned int>(cmd.a);
		if (cmd.type == CommandType::Expect && cmd.field == Field::Alive && cmd.op == Op::Eq && cmd.a == 0.f)
			script.deathExpected = true;
		script.commands.push_back(cmd);
	}
	if (errors.empty() && !levelSeen)
		errors.push_back(name + ": no 'level' command");
	return errors;
}

} // namespace Scenario
