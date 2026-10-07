#include "scenario.h"
#include "../graphics/gl_includes.h"
#include "../input/input.h"
#include "../graphics/draw.h"
#include "../state/game_loop.h"
#include "../input/input_actions.h"
#include "../state/game_state.h"
#include "../graphics/ink.h"
#include "../core/timer.h"
#include "../core/logger.h"
#include "../ui/screen_state.h"
#include "../ui/inventory.h"
#include "../world/loot.h"
#include "../world/level_gen.h"
#include <algorithm>
#include <GL/gl.h>
#include "../world/decor.h"
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>

#include "../../external/stb/stb_image_write.h"

namespace {
constexpr int EXIT_OK = 0;
constexpr int EXIT_FAILED = 1;
constexpr int EXIT_BAD_SCRIPT = 2;
constexpr int EXIT_CRASH = 3;

constexpr int MAX_TICKS = 10 * 60 * 1000 / Scenario::TICK_MS; // 10 min of game time
constexpr int WALK_STALL_TICKS = 30;						  // no movement this long = blocked
constexpr float WALK_EPSILON = 0.0001f;

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
	Xp,
	Hurt,
	Poison,
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
	Quit,
};

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
	Nearest,
	Coffins,
	JournalRiddles,
	JournalSolved,
	JournalNotes,
	JournalTried,
	Chests,
	ItemCount,
	ItemLevel
};
enum class Op : unsigned char { Eq, Ne, Lt, Le, Gt, Ge };

struct Command {
	CommandType type = CommandType::Quit;
	int line = 0;
	std::string text; // source line, for reports
	std::string arg;  // level path / screenshot name
	int ticks = 0;	  // wait, hold
	GameplayAction action = GameplayAction::None;
	bool walkTo = false;			// walk to: a is the target map x
	float a = 0.f;					// walk distance, camera rotM, expect value
	float b = 0.f;					// camera rotN
	ItemKind item = ItemKind::Club; // give / chest / expect count
	Field field = Field::X;
	Op op = Op::Eq;
};

struct WalkProgress {
	bool started = false;
	GameplayAction action = GameplayAction::None; // walk to X: the way to the target, chosen at the start
	float startPos = 0.f;
	float lastPos = 0.f;
	int stalledTicks = 0;
	int ticks = 0;
};

struct Runner {
	bool active = false;
	std::vector<Command> commands;
	size_t next = 0;
	int resX = 1280;
	int resY = 720;
	unsigned int seed = 1;
	bool god = false;
	bool deathExpected = false; // script has `expect alive == 0`
	bool deathReported = false;

	std::string outDir;
	std::ofstream result;
	int tick = 0;
	bool waiting = false;
	int waitLeft = 0;
	WalkProgress walk;
	std::string pendingShot;
	bool drawAll = false; // SCENARIO_DRAW_ALL: full frames between screenshots, to watch a run
	int shotCounter = 0;
	int executed = 0;
	std::vector<std::string> failures;
};

Runner gRunner;

bool isAxisX(GameplayAction action) {
	return action == GameplayAction::MoveLeft || action == GameplayAction::MoveRight;
}

float playerPos(GameplayAction axisOf) {
	float x = 0.f;
	float y = 0.f;
	Game().dungeon.getC(x, y);
	return isAxisX(axisOf) ? x : y;
}

// The item's label with _ for its spaces: small_health, sword.
std::string itemSlug(ItemKind kind) {
	std::string slug = itemText(kind).label;
	std::replace(slug.begin(), slug.end(), ' ', '_');
	return slug;
}

const char* screenName() {
	switch (ScreenState::GetDrawScreen(Game())) {
	case Screen::Menu:
		return "menu";
	case Screen::Inventory:
		return "inventory";
	case Screen::Riddle:
		return "riddle";
	case Screen::Map:
		return "map";
	case Screen::Journal:
		return "journal";
	case Screen::Gameplay:
		return "gameplay";
	}
	return "?";
}

std::string stateLine() {
	float x = 0.f;
	float y = 0.f;
	Game().dungeon.getC(x, y);
	char buf[256];
	snprintf(buf, sizeof(buf), "x=%.3f y=%.3f hp=%d stamina=%d level=%d screen=%s alive=%d won=%d", x, y,
			 Game().player->stats.CurrentHP(), Game().player->stats.Stamina(), Game().dungeon.LevelNumber(),
			 screenName(), Game().player->Alive() ? 1 : 0, Game().dungeon.Won() ? 1 : 0);
	return buf;
}

float fieldValue(const Command& cmd) {
	Field field = cmd.field;
	float x = 0.f;
	float y = 0.f;
	Game().dungeon.getC(x, y);
	switch (field) {
	case Field::X:
		return x;
	case Field::Y:
		return y;
	case Field::Hp:
		return static_cast<float>(Game().player->stats.CurrentHP());
	case Field::Stamina:
		return static_cast<float>(Game().player->stats.Stamina());
	case Field::Level:
		return static_cast<float>(Game().dungeon.LevelNumber());
	case Field::Alive:
		return Game().player->Alive() ? 1.f : 0.f;
	case Field::Won:
		return Game().dungeon.Won() ? 1.f : 0.f;
	case Field::Might:
		return static_cast<float>(Game().player->stats.CurrentMight());
	case Field::Armor:
		return static_cast<float>(Game().player->stats.CurrentArmor());
	case Field::EquipType:
		return static_cast<float>(fileIdOf(Game().ui.inventory->EquippedKind()).type);
	case Field::EquipId:
		return static_cast<float>(fileIdOf(Game().ui.inventory->EquippedKind()).id);
	case Field::Keys:
		return static_cast<float>(Game().dungeon.KeysHeld());
	case Field::Poison:
		return static_cast<float>(Game().player->stats.poison.Mask());
	case Field::XpTotal:
		return static_cast<float>(Game().player->stats.CurrentXP());
	case Field::Riddle:
		return Game().ui.screen == Screen::Riddle ? 1.f : 0.f;
	case Field::Bars:
		return static_cast<float>(Game().dungeon.MonsterBarsShown());
	case Field::Boss:
		return static_cast<float>(Game().dungeon.BossHealth());
	case Field::Minions:
		return static_cast<float>(Game().dungeon.LivingMinions());
	case Field::Attacking: // a swing or a bow draw under way
		return Game().player->attackStartMs >= 0 ? 1.f : 0.f;
	case Field::Nearest:
		return static_cast<float>(Game().dungeon.NearestMonsterHealth());
	case Field::Coffins:
		return static_cast<float>(Game().dungeon.CoffinCount());
	case Field::Chests:
		return static_cast<float>(Game().dungeon.ChestCount());
	case Field::JournalRiddles:
		return static_cast<float>(Game().journal.Riddles().size());
	case Field::JournalNotes:
		return static_cast<float>(Game().journal.Notes().size());
	case Field::JournalTried: {
		int known = 0;
		for (const JournalCreature& c : Game().journal.Creatures())
			for (int d = 0; d < DAMAGE_TYPE_COUNT; d++)
				known += c.Tried(static_cast<DamageType>(d)) ? 1 : 0;
		return static_cast<float>(known);
	}
	case Field::JournalSolved: {
		const auto& riddles = Game().journal.Riddles();
		return static_cast<float>(
			std::count_if(riddles.begin(), riddles.end(), [](const JournalRiddle& r) { return r.solved; }));
	}
	case Field::ItemCount:
		return static_cast<float>(Game().ui.inventory->Count(cmd.item));
	case Field::ItemLevel:
		return static_cast<float>(Game().ui.inventory->Level(cmd.item));
	}
	return 0.f;
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

// ---- reporting -------------------------------------------------------------

void report(const Command& cmd, bool ok, const std::string& detail) {
	char head[64];
	snprintf(head, sizeof(head), "tick %05d L%d ", gRunner.tick, cmd.line);
	std::string line = std::string(head) + cmd.text + " -> " + (ok ? "OK" : "FAIL");
	if (!detail.empty())
		line += " (" + detail + ")";
	gRunner.result << line << '\n';
	gRunner.result.flush();

	if (ok)
		gRunner.executed++;
	else
		gRunner.failures.push_back("line " + std::to_string(cmd.line) + ": " + cmd.text + ": " + detail);
}

void reportEvent(const std::string& text) {
	char head[32];
	snprintf(head, sizeof(head), "tick %05d ", gRunner.tick);
	gRunner.result << head << text << '\n';
	gRunner.result.flush();
}

[[noreturn]] void finish(int code) {
	int total = static_cast<int>(gRunner.commands.size());
	if (code == EXIT_OK && !gRunner.failures.empty())
		code = EXIT_FAILED;

	printf("%s %d/%d\n", code == EXIT_OK ? "PASS" : "FAIL", gRunner.executed, total);
	for (const std::string& failure : gRunner.failures)
		printf("FAIL %s\n", failure.c_str());
	fflush(stdout);

	gRunner.result << (code == EXIT_OK ? "PASS" : "FAIL") << ' ' << gRunner.executed << '/' << total << '\n';
	gRunner.result.close();
	Logger::shutdown();
	std::_Exit(code);
}

void onCrashSignal(int sig) {
	char msg[64];
	int len = snprintf(msg, sizeof(msg), "CRASH signal %d\n", sig);
	if (len > 0) {
		ssize_t written = write(STDOUT_FILENO, msg, static_cast<size_t>(len));
		(void)written;
	}
	_exit(EXIT_CRASH);
}

// ---- parsing ---------------------------------------------------------------

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
	ticks = static_cast<int>(std::ceil(ms / Scenario::TICK_MS));
	return true;
}

// "melee", "ranged" or "potion" -> ItemType value.
bool parseItemType(const std::string& word, int& type) {
	if (word == "melee")
		type = ItemType::MELEE_WEAPON;
	else if (word == "ranged")
		type = ItemType::RANGED_WEAPON;
	else if (word == "potion")
		type = ItemType::POTION;
	else
		return false;
	return true;
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
	static const struct {
		const char* name;
		Field field;
	} FIELDS[] = {{"x", Field::X},
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
				  {"xp", Field::XpTotal},
				  {"riddle", Field::Riddle},
				  {"bars", Field::Bars},
				  {"boss", Field::Boss},
				  {"minions", Field::Minions},
				  {"attacking", Field::Attacking},
				  {"nearest", Field::Nearest},
				  {"coffins", Field::Coffins},
				  {"chests", Field::Chests},
				  {"journal", Field::JournalRiddles},
				  {"journal_solved", Field::JournalSolved},
				  {"journal_notes", Field::JournalNotes},
				  {"journal_tried", Field::JournalTried}};
	for (const auto& entry : FIELDS)
		if (word == entry.name) {
			field = entry.field;
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

bool parseFloat(const std::string& word, float& out) {
	char* end = nullptr;
	out = strtof(word.c_str(), &end);
	return end != word.c_str() && *end == '\0';
}

// A walk key by name, None if it is not one.
GameplayAction walkAction(const std::string& word) {
	return word == "left"	 ? GameplayAction::MoveLeft
		   : word == "right" ? GameplayAction::MoveRight
		   : word == "up"	 ? GameplayAction::MoveUp
		   : word == "down"	 ? GameplayAction::MoveDown
							 : GameplayAction::None;
}

// Returns an error message, empty on success.
std::string parseLine(const std::vector<std::string>& w, Command& cmd) {
	const std::string& name = w[0];
	size_t argc = w.size() - 1;
	auto needArgs = [&](size_t n) { return argc == n ? "" : name + " expects " + std::to_string(n) + " argument(s)"; };

	if (name == "resolution") {
		cmd.type = CommandType::Resolution;
		if (argc != 2 || !parseFloat(w[1], cmd.a) || !parseFloat(w[2], cmd.b) || cmd.a < 64 || cmd.b < 64)
			return "usage: resolution <width> <height>";
		return "";
	}
	if (name == "seed") {
		cmd.type = CommandType::Seed;
		if (argc != 1 || !parseFloat(w[1], cmd.a) || cmd.a < 0)
			return "usage: seed <non-negative integer>";
		return "";
	}
	if (name == "god") {
		cmd.type = CommandType::God;
		return needArgs(0);
	}
	if (name == "level") {
		cmd.type = CommandType::Level;
		if (argc != 1)
			return "usage: level <number|path>";
		float number = 0.f;
		cmd.arg = w[1];
		cmd.a = parseFloat(w[1], number) ? number : 0.f;
		return "";
	}
	if (name == "wait") {
		cmd.type = CommandType::Wait;
		if (argc != 1 || !parseWait(w[1], cmd.ticks))
			return "usage: wait <ticks|Nms|Ns>";
		return "";
	}
	if (name == "walk") {
		cmd.type = CommandType::Walk;
		if (argc == 2 && w[1] == "to") { // walk to X: along the row to map x X, whichever way it is
			cmd.walkTo = true;
			if (!parseFloat(w[2], cmd.a))
				return "usage: walk to <map x>";
			return "";
		}
		if (argc != 2 || !parseFloat(w[2], cmd.a) || cmd.a <= 0)
			return "usage: walk <left|right|up|down> <tiles> | walk to <map x>";
		cmd.action = walkAction(w[1]);
		if (cmd.action == GameplayAction::None)
			return "walk direction must be left|right|up|down";
		return "";
	}
	if (name == "hold") { // the walk key down for T ticks, moving or not (walk fails when blocked)
		cmd.type = CommandType::Hold;
		cmd.action = argc == 2 ? walkAction(w[1]) : GameplayAction::None;
		if (cmd.action == GameplayAction::None || !parseWait(w[2], cmd.ticks))
			return "usage: hold <left|right|up|down> <ticks|Nms|Ns>";
		return "";
	}
	if (name == "motion") {
		cmd.type = CommandType::Motion;
		if (argc != 1 || (w[1] != "on" && w[1] != "off"))
			return "usage: motion <on|off>";
		cmd.a = w[1] == "on" ? 1.f : 0.f;
		return "";
	}
	if (name == "sprint") {
		cmd.type = CommandType::Sprint;
		if (argc != 1 || (w[1] != "on" && w[1] != "off"))
			return "usage: sprint <on|off>";
		cmd.a = w[1] == "on" ? 1.f : 0.f;
		return "";
	}
	if (name == "jump" || name == "attack" || name == "interact") {
		cmd.type = name == "jump" ? CommandType::Jump : name == "attack" ? CommandType::Attack : CommandType::Interact;
		cmd.action = name == "jump"		? GameplayAction::Jump
					 : name == "attack" ? GameplayAction::Attack
										: GameplayAction::Interact;
		return needArgs(0);
	}
	if (name == "camera") {
		cmd.type = CommandType::Camera;
		if (argc != 2 || !parseFloat(w[1], cmd.a) || !parseFloat(w[2], cmd.b))
			return "usage: camera <rotM> <rotN>";
		return "";
	}
	if (name == "toon") {
		cmd.type = CommandType::Toon;
		if (argc != 1 || (w[1] != "on" && w[1] != "off"))
			return "usage: toon <on|off>";
		cmd.a = w[1] == "on" ? 1.f : 0.f;
		return "";
	}
	if (name == "hitboxes") {
		cmd.type = CommandType::Hitboxes;
		if (argc != 1 || (w[1] != "on" && w[1] != "off"))
			return "usage: hitboxes <on|off>";
		cmd.a = w[1] == "on" ? 1.f : 0.f;
		return "";
	}
	if (name == "screenshot") {
		cmd.type = CommandType::Screenshot;
		if (argc != 1 || w[1].find_first_of("/\\") != std::string::npos)
			return "usage: screenshot <name> (no slashes)";
		cmd.arg = w[1];
		return "";
	}
	if (name == "dump") {
		cmd.type = CommandType::Dump;
		return needArgs(0);
	}
	if (name == "killboss") {
		cmd.type = CommandType::KillBoss;
		return needArgs(0);
	}
	if (name == "hurtboss") {
		cmd.type = CommandType::HurtBoss;
		if (argc != 1 || !parseFloat(w[1], cmd.a) || cmd.a < 1)
			return "usage: hurtboss <hp>";
		return "";
	}
	if (name == "expect") {
		cmd.type = CommandType::Expect;
		if (argc != 3 || !(parseField(w[1], cmd.field) || parseItemCountField(w[1], cmd)) || !parseOp(w[2], cmd.op) ||
			!parseFloat(w[3], cmd.a))
			return "usage: expect "
				   "<x|y|hp|stamina|level|alive|won|might|armor|equip_type|equip_id|keys|xp|riddle|bars|boss|minions|"
				   "attacking|"
				   "coffins|"
				   "<item><id>[.level]> "
				   "<==|!=|<|<=|>|>=> <number>";
		return "";
	}
	if (name == "key") {
		cmd.type = CommandType::Key;
		if (argc != 1)
			return "usage: key <char|enter|esc|space|tab|backspace|special key name>";
		static const struct {
			const char* name;
			unsigned char key;
		} NAMED[] = {
			{"enter", KEY_ENTER}, {"esc", KEY_ESCAPE}, {"space", KEY_SPACE}, {"tab", '\t'}, {"backspace", '\b'}};
		for (const auto& entry : NAMED)
			if (w[1] == entry.name)
				cmd.a = entry.key;
		if (cmd.a == 0.f && w[1].size() == 1)
			cmd.a = static_cast<unsigned char>(w[1][0]);
		if (std::optional<InputKey> named = parseKeyName(w[1]);
			cmd.a == 0.f && named && named->kind == InputKey::Kind::Special) {
			cmd.a = static_cast<float>(named->code); // a special key (docs/settings.md names): `left`, `f12`
			cmd.b = 1.f;
		}
		if (cmd.a == 0.f)
			return "key expects one character, enter|esc|space|tab|backspace or a special key name";
		return "";
	}
	if (name == "give" || name == "chest") {
		cmd.type = name == "give" ? CommandType::Give : CommandType::Chest;
		int type = 0;
		float id = 0.f;
		float count = 1.f;
		std::string usage = "usage: " + name + " <melee|ranged|potion> <id> [count], with a known id";
		if (argc < 2 || argc > 3 || !parseItemType(w[1], type) || !parseFloat(w[2], id) ||
			(argc == 3 && !parseFloat(w[3], count)))
			return usage;
		std::optional<ItemKind> item = itemFromFile(type, static_cast<int>(id));
		if (!item)
			return usage;
		cmd.item = *item;
		cmd.ticks = static_cast<int>(count);
		return "";
	}
	if (name == "select") {
		cmd.type = CommandType::Select;
		for (int i = 0; argc == 1 && i < ITEM_KIND_COUNT; i++)
			if (w[1] == itemSlug(itemAt(i))) {
				cmd.item = itemAt(i);
				return "";
			}
		return "usage: select <item>: club, short_sword, spear, self-bow, small_health, ... (its label, _ for spaces)";
	}
	if (name == "equip") {
		cmd.type = CommandType::Equip;
		for (int i = 0; argc == 1 && i < WEAPON_KIND_COUNT; i++)
			if (w[1] == itemSlug(itemAt(i))) {
				cmd.item = itemAt(i);
				return "";
			}
		return "usage: equip <weapon>: club, short_sword, spear, self-bow, ...";
	}
	if (name == "xp") {
		cmd.type = CommandType::Xp;
		if (argc != 1 || !parseFloat(w[1], cmd.a) || cmd.a < 0)
			return "usage: xp <non-negative number>";
		return "";
	}
	if (name == "hurt") {
		cmd.type = CommandType::Hurt;
		if (argc != 1 || !parseFloat(w[1], cmd.a) || cmd.a < 0)
			return "usage: hurt <hp>";
		return "";
	}
	if (name == "poison") {
		cmd.type = CommandType::Poison;
		static const char* const TIERS[POISON_TIER_COUNT] = {"weak", "medium", "strong"};
		for (int t = 0; argc == 1 && t < POISON_TIER_COUNT; t++)
			if (w[1] == TIERS[t]) {
				cmd.ticks = t;
				return "";
			}
		return "usage: poison weak|medium|strong";
	}
	if (name == "prop") {
		cmd.type = CommandType::Prop;
		for (int d = 0; argc == 2 && parseFloat(w[1], cmd.a) && d < DECOR_COUNT; d++)
			if (w[2] == DECOR_NAMES[d]) {
				cmd.ticks = d;
				return "";
			}
		return "usage: prop <col> <name>: a prop of DECOR_NAMES (web, pottery, ..., thoth_ibis_standing, ...)";
	}
	if (name == "riddles") {
		cmd.type = CommandType::Riddles;
		if (argc != 1)
			return "usage: riddles <file|directory>";
		cmd.arg = w[1];
		return "";
	}
	if (name == "savegame" || name == "loadgame") {
		cmd.type = name == "savegame" ? CommandType::SaveGame : CommandType::LoadGame;
		if (argc != 1)
			return "usage: " + name + " <path>";
		cmd.arg = w[1];
		return "";
	}
	if (name == "mouse" || name == "press" || name == "release" || name == "click") {
		cmd.type = name == "mouse"	   ? CommandType::Mouse
				   : name == "press"   ? CommandType::Press
				   : name == "release" ? CommandType::Release
									   : CommandType::Click;
		if (argc != 2 || !parseFloat(w[1], cmd.a) || !parseFloat(w[2], cmd.b))
			return "usage: " + name + " <x%> <y%> (0..100, y from the bottom)";
		return "";
	}
	if (name == "quit") {
		cmd.type = CommandType::Quit;
		return needArgs(0);
	}
	return "unknown command '" + name + "'";
}

// Screen position in percent (y from the bottom, like the UI code) -> window pixels.
void toPixels(const Command& cmd, int& x, int& y) {
	x = static_cast<int>(cmd.a / 100.f * static_cast<float>(Game().render.resX));
	y = static_cast<int>((1.f - cmd.b / 100.f) * static_cast<float>(Game().render.resY));
}

bool isSetupCommand(CommandType type) {
	return type == CommandType::Resolution || type == CommandType::Seed || type == CommandType::God;
}

// ---- execution -------------------------------------------------------------

// "gen:SEED:DIFFICULTY" -> a generated level.
bool loadGeneratedLevel(const std::string& spec) {
	unsigned int seed = 0;
	int difficulty = 0;
	if (sscanf(spec.c_str(), "gen:%u:%d", &seed, &difficulty) != 2)
		return false;
	GenOptions options;
	options.seed = seed;
	options.difficulty = difficulty;
	GenResult result = generateLevel(options);
	if (!result.ok)
		return false;
	Game().dungeon.LoadGrid(result.grid, spec.c_str());
	return true;
}

bool loadLevel(const Command& cmd) {
	Game().random.Seed(static_cast<uint64_t>(gRunner.seed));
	bool loaded = false;
	if (cmd.a > 0.f)
		loaded = Game().dungeon.LoadCampaignLevel(static_cast<int>(cmd.a));
	else if (cmd.arg.rfind("gen:", 0) == 0)
		loaded = loadGeneratedLevel(cmd.arg);
	else
		loaded = Game().dungeon.Load(cmd.arg.c_str());
	if (!loaded)
		return false;

	Game().ui.screen = Screen::Gameplay;
	Game().ui.menu.inGame = true;
	Game().dungeon.ClearWin();
	Game().player->Reanimate();
	return true;
}

// Advances a walk by one tick. Returns true when the command is finished (either way).
bool stepWalk(const Command& cmd) {
	WalkProgress& walk = gRunner.walk;
	float pos = playerPos(cmd.walkTo ? GameplayAction::MoveRight : cmd.action);
	if (!walk.started) {
		walk = WalkProgress{};
		walk.started = true;
		walk.startPos = pos;
		walk.lastPos = pos;
		walk.action = !cmd.walkTo ? cmd.action : cmd.a >= pos ? GameplayAction::MoveRight : GameplayAction::MoveLeft;
	}

	const bool arrived = cmd.walkTo ? (walk.action == GameplayAction::MoveRight ? pos >= cmd.a - WALK_EPSILON
																				: pos <= cmd.a + WALK_EPSILON)
									: std::fabs(pos - walk.startPos) >= cmd.a - WALK_EPSILON;
	if (arrived) {
		report(cmd, true, std::to_string(walk.ticks) + " ticks, " + stateLine());
		walk.started = false;
		releaseWalk();
		return true;
	}

	if (std::fabs(pos - walk.lastPos) < WALK_EPSILON)
		walk.stalledTicks++;
	else
		walk.stalledTicks = 0;
	walk.lastPos = pos;

	if (walk.stalledTicks >= WALK_STALL_TICKS) {
		char moved[32];
		snprintf(moved, sizeof(moved), "%.3f", std::fabs(pos - walk.startPos));
		report(cmd, false, std::string("blocked after ") + moved + " tiles, " + stateLine());
		walk.started = false;
		releaseWalk();
		return true;
	}

	setWalkHeld(walk.action, true); // Update() takes the step, as for a held key in play
	walk.ticks++;
	return false;
}

// Runs a non-blocking command immediately. Returns false for commands that span ticks.
bool runInstant(const Command& cmd) {
	switch (cmd.type) {
	case CommandType::Resolution:
	case CommandType::Seed:
		report(cmd, true, "");
		return true;
	case CommandType::God:
		gRunner.god = true;
		report(cmd, true, "");
		return true;
	case CommandType::Level:
		if (!loadLevel(cmd)) {
			report(cmd, false, "cannot load level");
			finish(EXIT_BAD_SCRIPT);
		}
		report(cmd, true, stateLine());
		return true;
	case CommandType::Jump:
	case CommandType::Attack:
	case CommandType::Interact:
		if (ScreenState::IsGameplayInteractionAllowed(Game()))
			executeGameplayAction(cmd.action);
		report(cmd, true, "");
		return true;
	case CommandType::Camera:
		Game().camera.rotM = cmd.a;
		Game().camera.rotN = cmd.b;
		report(cmd, true, "");
		return true;
	case CommandType::Sprint: // shift down / up
		Game().player->stats.SetSprintRequested(cmd.a > 0.5f);
		report(cmd, true, "");
		return true;
	case CommandType::Motion: // Options > Display > Motion effects
		Game().settings.graphics.motionEffects = cmd.a > 0.5f;
		report(cmd, true, "");
		return true;
	case CommandType::Toon:
		Game().settings.graphics.toon = cmd.a > 0.5f;
		Ink::setToon(cmd.a > 0.5f);
		report(cmd, true, "");
		return true;
	case CommandType::Hitboxes:
		Game().render.Hitboxes = cmd.a > 0.5f;
		report(cmd, true, "");
		return true;
	case CommandType::Screenshot:
		gRunner.pendingShot = cmd.arg;
		report(cmd, true, "");
		return true;
	case CommandType::Dump:
		report(cmd, true, stateLine());
		return true;
	case CommandType::KillBoss:
		report(cmd, Game().dungeon.BossHealth() > 0, "");
		Game().dungeon.HurtBoss(Game().dungeon.BossHealth());
		return true;
	case CommandType::HurtBoss:
		report(cmd, Game().dungeon.BossHealth() > 0, "");
		Game().dungeon.HurtBoss(static_cast<int>(cmd.a));
		return true;
	case CommandType::Expect: {
		float actual = fieldValue(cmd);
		char detail[48];
		snprintf(detail, sizeof(detail), "actual %.3f", actual);
		report(cmd, compare(actual, cmd.op, cmd.a), detail);
		return true;
	}
	case CommandType::Key:
		if (cmd.b > 0.f)
			specialKeyPressed(static_cast<int>(cmd.a), 0, 0);
		else
			keyPressed(static_cast<unsigned char>(cmd.a), 0, 0);
		report(cmd, true, std::string("screen=") + screenName());
		return true;
	case CommandType::Give:
		for (int i = 0; i < cmd.ticks; i++)
			Game().ui.inventory->AddItem(cmd.item);
		report(cmd, true, "");
		return true;
	case CommandType::Select: // like a click on its slot, also when it is on another tab
		if (ScreenState::GetDrawScreen(Game()) != Screen::Inventory) {
			report(cmd, false, std::string("the inventory is not open, screen ") + screenName());
			return true;
		}
		Game().ui.inventory->SelectItem(cmd.item);
		report(cmd, true, "");
		return true;
	case CommandType::Equip: // like a click on its slot in the inventory
		if (!Game().ui.inventory->Equip(cmd.item)) {
			report(cmd, false, "not held");
			return true;
		}
		report(cmd, true, "");
		return true;
	case CommandType::Xp: // levels up like killing monsters: more max HP, fully healed
		Game().player->stats.AddXP(static_cast<int>(cmd.a), Game().events);
		report(cmd, true, stateLine());
		return true;
	case CommandType::Hurt: // straight off the HP: no armour, no god, no death check
		Game().player->stats.LoseHP(static_cast<int>(cmd.a));
		report(cmd, true, stateLine());
		return true;
	case CommandType::Poison: // as from a poisoned bite: the status line and the field note too
		Game().player->Poison(static_cast<PoisonTier>(cmd.ticks), Game().events);
		report(cmd, true, stateLine());
		return true;
	case CommandType::Prop: { // on the player's row
		float x = 0.f;
		float y = 0.f;
		Game().dungeon.getC(x, y);
		const bool placed =
			Game().dungeon.PlaceDecor(static_cast<int>(cmd.a), static_cast<int>(std::floor(y)), cmd.ticks);
		report(cmd, placed, placed ? "" : "outside the level");
		return true;
	}
	case CommandType::Riddles:
		report(cmd, true, std::to_string(Game().ui.riddle->Load(cmd.arg)) + " riddles");
		return true;
	case CommandType::SaveGame: // relative paths land in the output directory
		Game().Save((cmd.arg.find('/') == std::string::npos ? gRunner.outDir + "/" + cmd.arg : cmd.arg).c_str());
		report(cmd, true, "");
		return true;
	case CommandType::LoadGame: {
		std::string path = cmd.arg.find('/') == std::string::npos ? gRunner.outDir + "/" + cmd.arg : cmd.arg;
		if (!std::filesystem::exists(path)) {
			report(cmd, false, "no such file");
			return true;
		}
		Game().LoadSave(path.c_str());
		report(cmd, true, stateLine());
		return true;
	}
	case CommandType::Chest: { // opens N chests holding this item, like picking them up
		int bonus = 0;
		for (int i = 0; i < cmd.ticks; i++) {
			std::vector<ItemKind> loot =
				RollChestLoot(cmd.item, Game().ui.inventory->Bag().Owned(), Game().random.gameplay);
			bonus += static_cast<int>(loot.size()) - 1;
			for (ItemKind entry : loot)
				Game().ui.inventory->AddItem(entry);
		}
		report(cmd, true, std::to_string(bonus) + " bonus items");
		return true;
	}
	case CommandType::Mouse:
	case CommandType::Press:
	case CommandType::Release:
	case CommandType::Click: {
		int x = 0;
		int y = 0;
		toPixels(cmd, x, y);
		processMousePassiveMotion(x, y);
		if (cmd.type == CommandType::Press || cmd.type == CommandType::Click)
			processMouse(MOUSE_LEFT_BUTTON, GLUT_DOWN, x, y);
		if (cmd.type == CommandType::Release || cmd.type == CommandType::Click)
			processMouse(MOUSE_LEFT_BUTTON, GLUT_UP, x, y);
		report(cmd, true, "");
		return true;
	}
	case CommandType::Quit:
		report(cmd, true, "");
		finish(EXIT_OK);
	case CommandType::Wait:
	case CommandType::Walk:
	case CommandType::Hold:
		return false;
	}
	return true;
}

// Executes commands until one needs more ticks.
void runCommands() {
	while (gRunner.next < gRunner.commands.size()) {
		// Screenshot must be taken from this tick's frame before the script moves on.
		if (!gRunner.pendingShot.empty())
			return;

		const Command& cmd = gRunner.commands[gRunner.next];
		if (cmd.type == CommandType::Wait) {
			if (!gRunner.waiting) {
				gRunner.waiting = true;
				gRunner.waitLeft = cmd.ticks;
			}
			if (gRunner.waitLeft > 0) {
				gRunner.waitLeft--;
				return;
			}
			gRunner.waiting = false;
			report(cmd, true, "");
			gRunner.next++;
			continue;
		}
		if (cmd.type == CommandType::Hold) {
			if (!gRunner.waiting) {
				gRunner.waiting = true;
				gRunner.waitLeft = cmd.ticks;
			}
			if (gRunner.waitLeft > 0) {
				gRunner.waitLeft--;
				setWalkHeld(cmd.action, true); // Update() takes the step, as for a held key in play
				return;
			}
			gRunner.waiting = false;
			releaseWalk();
			report(cmd, true, stateLine());
			gRunner.next++;
			continue;
		}
		if (cmd.type == CommandType::Walk) {
			if (!stepWalk(cmd))
				return;
			gRunner.next++;
			continue;
		}
		runInstant(cmd);
		Game().ApplyWorldEvents(); // an expect on the next line sees what this command made the world do
		gRunner.next++;
	}
	if (!gRunner.pendingShot.empty())
		return;		 // render the last screenshot first
	finish(EXIT_OK); // implicit quit at end of script
}

void checkDeath() {
	if (gRunner.deathReported || gRunner.deathExpected || Game().player->Alive())
		return;
	if (!Game().ui.menu.inGame)
		return;
	gRunner.deathReported = true;
	reportEvent("player died: " + stateLine());
	gRunner.failures.push_back("player died at tick " + std::to_string(gRunner.tick));
}
} // namespace

bool Scenario::load(const char* path) {
	std::ifstream in(path);
	if (!in) {
		fprintf(stderr, "scenario: cannot open %s\n", path);
		return false;
	}

	bool ok = true;
	bool levelSeen = false;
	std::string raw;
	for (int lineNo = 1; std::getline(in, raw); lineNo++) {
		std::string text = raw.substr(0, raw.find('#'));
		std::istringstream words(text);
		std::vector<std::string> w;
		for (std::string word; words >> word;)
			w.push_back(word);
		if (w.empty())
			continue;

		Command cmd;
		cmd.line = lineNo;
		for (size_t i = 0; i < w.size(); i++)
			cmd.text += (i ? " " : "") + w[i];

		std::string error = parseLine(w, cmd);
		if (error.empty() && !levelSeen && !isSetupCommand(cmd.type) && cmd.type != CommandType::Level)
			error = "'level' must come before gameplay commands";
		if (error.empty() && levelSeen && cmd.type == CommandType::Resolution)
			error = "'resolution' must come before 'level'";
		if (!error.empty()) {
			fprintf(stderr, "%s:%d: %s\n", path, lineNo, error.c_str());
			ok = false;
			continue;
		}

		if (cmd.type == CommandType::Level)
			levelSeen = true;
		if (cmd.type == CommandType::Resolution) {
			gRunner.resX = static_cast<int>(cmd.a);
			gRunner.resY = static_cast<int>(cmd.b);
		}
		if (cmd.type == CommandType::Seed)
			gRunner.seed = static_cast<unsigned int>(cmd.a);
		if (cmd.type == CommandType::Expect && cmd.field == Field::Alive && cmd.op == Op::Eq && cmd.a == 0.f)
			gRunner.deathExpected = true;
		gRunner.commands.push_back(cmd);
	}

	if (ok && !levelSeen) {
		fprintf(stderr, "%s: no 'level' command\n", path);
		ok = false;
	}
	if (!ok)
		return false;

	std::filesystem::path scriptPath(path);
	gRunner.outDir = "tests/out/" + scriptPath.stem().string();
	std::error_code ec;
	std::filesystem::remove_all(gRunner.outDir, ec);
	std::filesystem::create_directories(gRunner.outDir, ec);
	gRunner.result.open(gRunner.outDir + "/result.txt");
	if (!gRunner.result) {
		fprintf(stderr, "scenario: cannot write %s/result.txt\n", gRunner.outDir.c_str());
		return false;
	}
	gRunner.result << "scenario " << path << '\n';

	signal(SIGSEGV, onCrashSignal);
	signal(SIGABRT, onCrashSignal);
	signal(SIGFPE, onCrashSignal);

	const char* drawAll = std::getenv("SCENARIO_DRAW_ALL");
	gRunner.drawAll = drawAll != nullptr && std::string(drawAll) == "1";
	gRunner.active = true;
	return true;
}

bool Scenario::active() { return gRunner.active; }

int Scenario::resolutionX() { return gRunner.resX; }

int Scenario::resolutionY() { return gRunner.resY; }

bool Scenario::godMode() { return gRunner.god; }

int Scenario::tickDelayMs() { return gRunner.drawAll ? TICK_MS : 0; }

void Scenario::tick() {
	try {
		if (!Game().cacheLoaded) {
			Update(); // loads assets, draws the loading bar
			return;
		}

		GameClock::advance(TICK_MS);
		gRunner.tick++;
		if (gRunner.tick > MAX_TICKS) {
			gRunner.failures.push_back("hard time limit reached");
			finish(EXIT_FAILED);
		}

		runCommands();
		Update();
		// Frames without a screenshot still run Draw (the UI screens advance their own animations there, such as the
		// inventory's turntable; the game world does not) but fill one pixel: software GL under Xvfb spends nearly all
		// its time on fill. SCENARIO_DRAW_ALL=1 (HEADLESS=0) draws them in full.
		const bool blind = gRunner.pendingShot.empty() && !gRunner.drawAll;
		if (blind) {
			glEnable(GL_SCISSOR_TEST);
			glScissor(0, 0, 1, 1);
		}
		Draw();
		if (blind)
			glDisable(GL_SCISSOR_TEST);
		checkDeath();

		if (!gRunner.pendingShot.empty()) {
			reportEvent("screenshot '" + gRunner.pendingShot + "' not captured on screen " + screenName());
			gRunner.failures.push_back("screenshot '" + gRunner.pendingShot + "' not captured");
			gRunner.pendingShot.clear();
		}
	} catch (const std::exception& e) {
		printf("CRASH exception: %s\n", e.what());
		fflush(stdout);
		std::_Exit(EXIT_CRASH);
	}
}

void Scenario::onFrameRendered() {
	if (!gRunner.active || gRunner.pendingShot.empty())
		return;

	int width = Game().render.resX;
	int height = Game().render.resY;
	if (width <= 0 || height <= 0)
		return;
	std::vector<unsigned char> pixels(static_cast<size_t>(width) * static_cast<size_t>(height) * 3);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

	char file[512];
	snprintf(file, sizeof(file), "%s/%03d_%s.png", gRunner.outDir.c_str(), ++gRunner.shotCounter,
			 gRunner.pendingShot.c_str());
	stbi_flip_vertically_on_write(1);
	if (stbi_write_png(file, width, height, 3, pixels.data(), width * 3))
		reportEvent(std::string("saved ") + file);
	else
		gRunner.failures.push_back(std::string("cannot write ") + file);
	gRunner.pendingShot.clear();
}
