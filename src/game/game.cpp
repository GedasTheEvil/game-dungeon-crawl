#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>
#include <cstdlib>
#include <malloc.h>
#include <stdexcept>
#include <ctime>
#include <cstdio>
#include <string>
#include <vector>
#include "window.h"
#include "../graphics/draw.h"
#include "../state/game_state.h"
#include "../core/logger.h"
#include "../core/paths.h"
#include "../core/timer.h"
#include "../test/scenario.h"
#include "../state/settings.h"
#include <algorithm>

static bool debugFlag = false; // --debug: the debug keys (F3 hitboxes, plan controls-help)

// A run in the background (--no-sound, a scenario): no music or sound effects, and the window on X11, so under
// xvfb-run it opens there and not on a Wayland desktop (SDL3 prefers Wayland; WAYLAND_DISPLAY outlives xvfb-run). An
// SDL_VIDEO_DRIVER set by the caller wins.
static void runInBackground() {
	setenv("SDL_AUDIO_DRIVER", "dummy", 1);
	setenv("SDL_VIDEO_DRIVER", "x11", 0);
}

// The command line: a scenario run, a hand-played level or scenario (Scenario::loadPlay), or the plain game.
// False: bad arguments, usage printed.
static bool parseArgs(int argc, char* argv[]) {
	std::vector<std::string> args;
	for (int i = 1; i < argc; i++) {
		if (std::string(argv[i]) == "--no-sound")
			runInBackground();
		else if (std::string(argv[i]) == "--debug")
			debugFlag = true;
		else
			args.emplace_back(argv[i]);
	}
	const std::string flag = args.empty() ? "" : args[0];
	bool ok = true;
	if (args.size() == 2 && flag == "--level")
		ok = Scenario::loadPlayLevel(Paths::fromLaunchDir(args[1]).c_str());
	else if (args.size() == 2 && flag == "--play")
		ok = Scenario::loadPlay(Paths::fromLaunchDir(args[1]).c_str());
	else if (!args.empty() &&
			 std::none_of(args.begin(), args.end(), [](const std::string& arg) { return arg.starts_with("--"); })) {
		std::vector<std::string> paths(args.size());
		std::transform(args.begin(), args.end(), paths.begin(), Paths::fromLaunchDir);
		ok = paths.size() == 1 ? Scenario::load(paths[0].c_str()) : Scenario::loadBatch(paths);
		runInBackground();
		GameClock::enableVirtual();
	} else if (!args.empty()) {
		fprintf(
			stderr,
			"usage: %s [--no-sound] [--debug] [--level N|PATH|gen:SEED:D[:ZONE] | --play SCENARIO.txt | "
			"SCENARIO.txt...]\n",
			argv[0]
		);
		ok = false;
	}
	return ok;
}

int main(int argc, char* argv[]) {
	// One malloc arena: the loading threads' own arenas (plan faster-loading) kept up to 0.3 GB of freed memory, and
	// were no faster.
	mallopt(M_ARENA_MAX, 1);
	Paths::enterRootDir(); // game.log, saves/ and the media from the repo root, wherever bin/game starts
	Logger::initialize();

	if (!parseArgs(argc, argv))
		return 2;
	bool isScenario = Scenario::active();

	try {
		if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
			LOG_ERRORF("game", "SDL initialization failed: %s", SDL_GetError());
			return -1;
		}

		// Initialize game state
		CreateGame();
		Game().debug = debugFlag;
		if (const std::string out = Scenario::outputDir(); !out.empty()) // never the real board in saves/
			Game().leaderboard.SetPath(out + "/leaderboard.txt");
		if (isScenario) {
			// The runner's hooks (plan game-code-without-scenario): saves in its tests/out/<name>/, no settings
			// file, the screenshot after each frame.
			Game().saves.folder.dir = Scenario::outputDir();
			SettingsFile::SetEnabled(false);
			setFrameHook(&Scenario::onFrameRendered);
			Game().render.resX = Scenario::resolutionX();
			Game().render.resY = Scenario::resolutionY();
			Game().settings.graphics.cutScenes = false; // timings stay as they were; `cutscenes on` turns them on
			Game().settings.graphics.cameraRecenter = false; // a `camera` view stays for the screenshot
		} else {
			Game().random.Seed(static_cast<uint64_t>(std::time(nullptr))); // a scenario seeds per level instead
			SettingsFile::Load(Game().settings);
			Game().render.resX = Game().settings.display.width;
			Game().render.resY = Game().settings.display.height;
		}
		Game().ApplySettings(false);

		if (!Window::open(Game().render.resX, Game().render.resY, Game().settings.display.fullscreen))
			return -1;
		// Quit (and closing the window) end the loop, so the cleanup below runs. A scenario: no user input, the
		// script drives every frame.
		if (isScenario)
			Window::runScenario();
		else
			Window::run();

		// Cleanup
		SettingsFile::Save(Game().settings); // only writes when the window was resized
		DestroyGame();
		Window::close();
		Logger::shutdown();
		SDL_Quit();
	} catch (const std::exception& e) {
		LOG_ERRORF("game", "Fatal error: %s", e.what());
		return -1;
	}

	return 0;
}
