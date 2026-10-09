#include "scenario.h"
#include "scenario_fields.h"
#include "scenario_script.h"
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
#include <GL/gl.h>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>

#include "../../external/stb/stb_image_write.h"

using namespace Scenario;

namespace {
constexpr int EXIT_OK = 0;
constexpr int EXIT_FAILED = 1;
constexpr int EXIT_BAD_SCRIPT = 2;
constexpr int EXIT_CRASH = 3;

constexpr int MAX_TICKS = 10 * 60 * 1000 / Scenario::TICK_MS; // 10 min of game time
constexpr int WALK_STALL_TICKS = 30;						  // no movement this long = blocked
constexpr float WALK_EPSILON = 0.0001f;

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
	Script script;
	size_t next = 0;
	bool god = false;
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

GameplayAction moveAction(Move move) {
	switch (move) {
	case Move::Left:
		return GameplayAction::MoveLeft;
	case Move::Right:
		return GameplayAction::MoveRight;
	case Move::Down:
		return GameplayAction::MoveDown;
	case Move::Up:
		return GameplayAction::MoveUp;
	case Move::None:
		break;
	}
	return GameplayAction::None;
}

bool isAxisX(GameplayAction action) {
	return action == GameplayAction::MoveLeft || action == GameplayAction::MoveRight;
}

float playerPos(GameplayAction axisOf) {
	float x = 0.f;
	float y = 0.f;
	Game().dungeon.getC(x, y);
	return isAxisX(axisOf) ? x : y;
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
	int total = static_cast<int>(gRunner.script.commands.size());
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

// Screen position in percent (y from the bottom, like the UI code) -> window pixels.
void toPixels(const Command& cmd, int& x, int& y) {
	x = static_cast<int>(cmd.a / 100.f * static_cast<float>(Game().render.resX));
	y = static_cast<int>((1.f - cmd.b / 100.f) * static_cast<float>(Game().render.resY));
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
	Game().dungeon.LoadGrid(result.grid, spec.c_str(), genDecorDepth(difficulty));
	return true;
}

bool loadLevel(const Command& cmd) {
	Game().random.Seed(static_cast<uint64_t>(gRunner.script.seed));
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
	float pos = playerPos(cmd.walkTo ? GameplayAction::MoveRight : moveAction(cmd.move));
	if (!walk.started) {
		walk = WalkProgress{};
		walk.started = true;
		walk.startPos = pos;
		walk.lastPos = pos;
		walk.action = !cmd.walkTo	 ? moveAction(cmd.move)
					  : cmd.a >= pos ? GameplayAction::MoveRight
									 : GameplayAction::MoveLeft;
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
			executeGameplayAction(cmd.type == CommandType::Jump		? GameplayAction::Jump
								  : cmd.type == CommandType::Attack ? GameplayAction::Attack
																	: GameplayAction::Interact);
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
	case CommandType::PoisonBoss:
		report(cmd, Game().dungeon.BossHealth() > 0, "");
		Game().dungeon.PoisonBoss(static_cast<PoisonTier>(cmd.ticks));
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
	case CommandType::Sort: // like a click on its button, for the open tab
		if (ScreenState::GetDrawScreen(Game()) != Screen::Inventory) {
			report(cmd, false, std::string("the inventory is not open, screen ") + screenName());
			return true;
		}
		Game().ui.inventory->SetSort(static_cast<SortOrder>(cmd.ticks));
		report(cmd, true, "");
		return true;
	case CommandType::Wear: // like a click on its slot in the inventory
		if (!Game().ui.inventory->Wear(isAmulet(cmd.item) ? std::optional(cmd.item) : std::nullopt)) {
			report(cmd, false, "not held");
			return true;
		}
		report(cmd, true, stateLine());
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
	case CommandType::Poison: // as from a poisoned bite: the status line, the field note and the amulet's ward too
		Game().player->Poison(static_cast<PoisonTier>(cmd.ticks), Game().events, Game().random.gameplay);
		report(cmd, true, stateLine());
		return true;
	case CommandType::PoisonMonster: // as by the player; whether it took: expect nearest_poison
		Game().dungeon.PoisonNearestMonster(static_cast<PoisonTier>(cmd.ticks));
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
			std::vector<ItemKind> loot = RollChestLoot(cmd.item, Game().ui.inventory->Bag().Owned(),
													   Game().dungeon.LevelNumber(), Game().random.gameplay);
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
	while (gRunner.next < gRunner.script.commands.size()) {
		// Screenshot must be taken from this tick's frame before the script moves on.
		if (!gRunner.pendingShot.empty())
			return;

		const Command& cmd = gRunner.script.commands[gRunner.next];
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
				setWalkHeld(moveAction(cmd.move), true); // Update() takes the step, as for a held key in play
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
	if (gRunner.deathReported || gRunner.script.deathExpected || Game().player->Alive())
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

	const std::vector<std::string> errors = parseScript(in, path, gRunner.script);
	for (const std::string& error : errors)
		fprintf(stderr, "%s\n", error.c_str());
	if (!errors.empty())
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

int Scenario::resolutionX() { return gRunner.script.resX; }

int Scenario::resolutionY() { return gRunner.script.resY; }

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
