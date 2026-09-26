#include "scenario.h"
#include "../graphics/gl_includes.h"
#include "../input/input.h"
#include "../input/input_actions.h"
#include "../state/game_state.h"
#include "../core/service_locator.h"
#include "../core/timer.h"
#include "../core/logger.h"
#include "../ui/screen_state.h"
#include "../ui/inventory.h"
#include "../world/loot.h"
#include "../world/level_gen.h"
#include <GL/gl.h>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "../../external/stb/stb_image_write.h"
#pragma GCC diagnostic pop

namespace {
constexpr int EXIT_OK = 0;
constexpr int EXIT_FAILED = 1;
constexpr int EXIT_BAD_SCRIPT = 2;
constexpr int EXIT_CRASH = 3;

constexpr int MAX_TICKS = 10 * 60 * 1000 / Scenario::TICK_MS; // 10 min of game time
constexpr int WALK_STALL_TICKS = 30;						  // no movement this long = blocked
constexpr float WALK_EPSILON = 0.0001f;

enum class CommandType {
	Resolution,
	Seed,
	God,
	Level,
	Wait,
	Walk,
	Jump,
	Attack,
	Interact,
	Camera,
	Screenshot,
	Dump,
	Expect,
	Key,
	Give,
	Chest,
	SaveGame,
	LoadGame,
	Mouse,
	Press,
	Release,
	Click,
	Quit,
};

enum class Field { X, Y, Hp, Stamina, Level, Alive, Won, Might, Armor, EquipType, EquipId, Keys, ItemCount, ItemLevel };
enum class Op { Eq, Ne, Lt, Le, Gt, Ge };

struct Command {
	CommandType type = CommandType::Quit;
	int line = 0;
	std::string text; // source line, for reports
	std::string arg;  // level path / screenshot name
	int ticks = 0;	  // wait
	GameplayAction action = GameplayAction::None;
	float a = 0.f; // walk distance, camera rotM, expect value
	float b = 0.f; // camera rotN
	int item = 0;  // give / expect count: item type
	int itemId = 0;
	Field field = Field::X;
	Op op = Op::Eq;
};

struct WalkProgress {
	bool started = false;
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
	GAME_STATE.dungeon.getC(x, y);
	return isAxisX(axisOf) ? x : y;
}

const char* screenName() {
	switch (ScreenState::GetDrawScreen(GAME_STATE)) {
	case ScreenState::DrawScreen::Menu:
		return "menu";
	case ScreenState::DrawScreen::Inventory:
		return "inventory";
	case ScreenState::DrawScreen::Riddle:
		return "riddle";
	case ScreenState::DrawScreen::Gameplay:
		return "gameplay";
	}
	return "?";
}

std::string stateLine() {
	float x = 0.f;
	float y = 0.f;
	GAME_STATE.dungeon.getC(x, y);
	char buf[256];
	snprintf(buf, sizeof(buf), "x=%.3f y=%.3f hp=%d stamina=%d level=%d screen=%s alive=%d won=%d", x, y,
			 GAME_STATE.Player->health, GAME_STATE.Player->Stamina(), GAME_STATE.curMap, screenName(),
			 GAME_STATE.Player->Alive() ? 1 : 0, GAME_STATE.IHaveWon ? 1 : 0);
	return buf;
}

float fieldValue(const Command& cmd) {
	Field field = cmd.field;
	float x = 0.f;
	float y = 0.f;
	GAME_STATE.dungeon.getC(x, y);
	switch (field) {
	case Field::X:
		return x;
	case Field::Y:
		return y;
	case Field::Hp:
		return static_cast<float>(GAME_STATE.Player->health);
	case Field::Stamina:
		return static_cast<float>(GAME_STATE.Player->Stamina());
	case Field::Level:
		return static_cast<float>(GAME_STATE.curMap);
	case Field::Alive:
		return GAME_STATE.Player->Alive() ? 1.f : 0.f;
	case Field::Won:
		return GAME_STATE.IHaveWon ? 1.f : 0.f;
	case Field::Might:
		return static_cast<float>(GAME_STATE.ui.Stats->CurrentMight());
	case Field::Armor:
		return static_cast<float>(GAME_STATE.ui.Stats->CurrentArmor());
	case Field::EquipType:
		return static_cast<float>(GAME_STATE.ui.invent->EquippedType());
	case Field::EquipId:
		return static_cast<float>(GAME_STATE.ui.invent->EquippedId());
	case Field::Keys:
		return static_cast<float>(GAME_STATE.dungeon.KeysHeld());
	case Field::ItemCount:
		return static_cast<float>(GAME_STATE.ui.invent->Count(cmd.item, cmd.itemId));
	case Field::ItemLevel:
		return static_cast<float>(GAME_STATE.ui.invent->Level(cmd.item, cmd.itemId));
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
	if (digits == std::string::npos || digits == 0 || !parseItemType(word.substr(0, digits), cmd.item))
		return false;
	char* end = nullptr;
	cmd.itemId = static_cast<int>(strtol(word.c_str() + digits, &end, 10));
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
				  {"keys", Field::Keys}};
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
		if (argc != 2 || !parseFloat(w[2], cmd.a) || cmd.a <= 0)
			return "usage: walk <left|right|up|down> <tiles>";
		if (w[1] == "left")
			cmd.action = GameplayAction::MoveLeft;
		else if (w[1] == "right")
			cmd.action = GameplayAction::MoveRight;
		else if (w[1] == "up")
			cmd.action = GameplayAction::MoveUp;
		else if (w[1] == "down")
			cmd.action = GameplayAction::MoveDown;
		else
			return "walk direction must be left|right|up|down";
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
	if (name == "expect") {
		cmd.type = CommandType::Expect;
		if (argc != 3 || !(parseField(w[1], cmd.field) || parseItemCountField(w[1], cmd)) || !parseOp(w[2], cmd.op) ||
			!parseFloat(w[3], cmd.a))
			return "usage: expect "
				   "<x|y|hp|stamina|level|alive|won|might|armor|equip_type|equip_id|keys|<item><id>[.level]> "
				   "<==|!=|<|<=|>|>=> <number>";
		return "";
	}
	if (name == "key") {
		cmd.type = CommandType::Key;
		if (argc != 1)
			return "usage: key <char|enter|esc|space|tab>";
		static const struct {
			const char* name;
			unsigned char key;
		} NAMED[] = {{"enter", KEY_ENTER}, {"esc", KEY_ESCAPE}, {"space", KEY_SPACE}, {"tab", '\t'}};
		for (const auto& entry : NAMED)
			if (w[1] == entry.name)
				cmd.a = entry.key;
		if (cmd.a == 0.f && w[1].size() == 1)
			cmd.a = static_cast<unsigned char>(w[1][0]);
		if (cmd.a == 0.f)
			return "key expects one character or enter|esc|space|tab";
		return "";
	}
	if (name == "give" || name == "chest") {
		cmd.type = name == "give" ? CommandType::Give : CommandType::Chest;
		float id = 0.f;
		float count = 1.f;
		if (argc < 2 || argc > 3 || !parseItemType(w[1], cmd.item) || !parseFloat(w[2], id) ||
			(argc == 3 && !parseFloat(w[3], count)))
			return "usage: " + name + " <melee|ranged|potion> <id> [count]";
		cmd.itemId = static_cast<int>(id);
		cmd.ticks = static_cast<int>(count);
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
	x = static_cast<int>(cmd.a / 100.f * static_cast<float>(GAME_STATE.render.resX));
	y = static_cast<int>((1.f - cmd.b / 100.f) * static_cast<float>(GAME_STATE.render.resY));
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
	GAME_STATE.dungeon.LoadGrid(result.grid, spec.c_str());
	return true;
}

bool loadLevel(const Command& cmd) {
	srand(gRunner.seed);
	bool loaded = false;
	if (cmd.a > 0.f)
		loaded = GAME_STATE.dungeon.LoadCampaignLevel(static_cast<int>(cmd.a));
	else if (cmd.arg.rfind("gen:", 0) == 0)
		loaded = loadGeneratedLevel(cmd.arg);
	else
		loaded = GAME_STATE.dungeon.Load(cmd.arg.c_str());
	if (!loaded)
		return false;

	if (cmd.a > 0.f)
		GAME_STATE.curMap = static_cast<int>(cmd.a);
	GAME_STATE.ui.menu.show = false;
	GAME_STATE.ui.menu.inGame = true;
	GAME_STATE.IHaveWon = false;
	GAME_STATE.Player->Reanimate();
	return true;
}

// Advances a walk by one tick. Returns true when the command is finished (either way).
bool stepWalk(const Command& cmd) {
	WalkProgress& walk = gRunner.walk;
	float pos = playerPos(cmd.action);
	if (!walk.started) {
		walk = WalkProgress{};
		walk.started = true;
		walk.startPos = pos;
		walk.lastPos = pos;
	}

	if (std::fabs(pos - walk.startPos) >= cmd.a - WALK_EPSILON) {
		report(cmd, true, std::to_string(walk.ticks) + " ticks, " + stateLine());
		walk.started = false;
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
		return true;
	}

	if (ScreenState::IsGameplayInteractionAllowed(GAME_STATE))
		executeGameplayAction(cmd.action);
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
		if (ScreenState::IsGameplayInteractionAllowed(GAME_STATE))
			executeGameplayAction(cmd.action);
		report(cmd, true, "");
		return true;
	case CommandType::Camera:
		GAME_STATE.camera.rotM = cmd.a;
		GAME_STATE.camera.rotN = cmd.b;
		report(cmd, true, "");
		return true;
	case CommandType::Screenshot:
		gRunner.pendingShot = cmd.arg;
		report(cmd, true, "");
		return true;
	case CommandType::Dump:
		report(cmd, true, stateLine());
		return true;
	case CommandType::Expect: {
		float actual = fieldValue(cmd);
		char detail[48];
		snprintf(detail, sizeof(detail), "actual %.3f", actual);
		report(cmd, compare(actual, cmd.op, cmd.a), detail);
		return true;
	}
	case CommandType::Key:
		keyPressed(static_cast<unsigned char>(cmd.a), 0, 0);
		report(cmd, true, std::string("screen=") + screenName());
		return true;
	case CommandType::Give:
		for (int i = 0; i < cmd.ticks; i++)
			GAME_STATE.ui.invent->GetItem(cmd.item, cmd.itemId);
		report(cmd, true, "");
		return true;
	case CommandType::SaveGame: // relative paths land in the output directory
		GAME_STATE.Save((cmd.arg.find('/') == std::string::npos ? gRunner.outDir + "/" + cmd.arg : cmd.arg).c_str());
		report(cmd, true, "");
		return true;
	case CommandType::LoadGame: {
		std::string path = cmd.arg.find('/') == std::string::npos ? gRunner.outDir + "/" + cmd.arg : cmd.arg;
		if (!std::filesystem::exists(path)) {
			report(cmd, false, "no such file");
			return true;
		}
		GAME_STATE.LoadSave(path.c_str());
		report(cmd, true, stateLine());
		return true;
	}
	case CommandType::Chest: { // opens N chests holding this item, like picking them up
		int bonus = 0;
		for (int i = 0; i < cmd.ticks; i++) {
			std::vector<LootItem> loot = RollChestLoot(cmd.item, cmd.itemId);
			bonus += static_cast<int>(loot.size()) - 1;
			for (const LootItem& entry : loot)
				GAME_STATE.ui.invent->GetItem(entry.type, entry.id);
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
		if (cmd.type == CommandType::Walk) {
			if (!stepWalk(cmd))
				return;
			gRunner.next++;
			continue;
		}
		runInstant(cmd);
		gRunner.next++;
	}
	if (!gRunner.pendingShot.empty())
		return;		 // render the last screenshot first
	finish(EXIT_OK); // implicit quit at end of script
}

void checkDeath() {
	if (gRunner.deathReported || gRunner.deathExpected || GAME_STATE.Player->Alive())
		return;
	if (!GAME_STATE.ui.menu.inGame)
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

	gRunner.active = true;
	return true;
}

bool Scenario::active() { return gRunner.active; }

int Scenario::resolutionX() { return gRunner.resX; }

int Scenario::resolutionY() { return gRunner.resY; }

bool Scenario::godMode() { return gRunner.god; }

void Scenario::tick() {
	try {
		if (!GAME_STATE.Cache_loaded) {
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
		Draw();
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

	int width = GAME_STATE.render.resX;
	int height = GAME_STATE.render.resY;
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
