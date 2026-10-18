#include "../graphics/gl_includes.h"

#include <GL/glu.h>
#include <GL/gl.h>
#include <SDL/SDL.h>
#include <stdlib.h>
#include <stdexcept>
#include <ctime>
#include <cstdio>
#include <string>
#include <vector>
#include "../input/input.h"
#include "gameplay_config.h"
#include "../graphics/draw.h"
#include "../state/game_loop.h"
#include "../graphics/textures.h"
#include "sound.h"
#include "../state/game_state.h"
#include "logger.h"
#include "paths.h"
#include "timer.h"
#include "../test/scenario.h"
#include "../state/settings.h"
#include <algorithm>

int window = 1;

static void UpdateTimerCallback(int) {
	Update();
	Scenario::runPlaySetup();
	glutTimerFunc(UPDATE_TICK_MS, UpdateTimerCallback, 0);
}

static void scenarioTickCallback(int) {
	Scenario::tick();
	glutTimerFunc(Scenario::tickDelayMs(), scenarioTickCallback, 0);
}

// Scenario frames are drawn from the tick, so window expose events must not render extra frames.
static void noopDisplay() {}

void initGl(GLsizei width, GLsizei height) // We call this right after our OpenGL window is created.
{
	(void)width;
	(void)height;

	glEnable(GL_TEXTURE_2D); // Enable texture mapping.

	glBlendFunc(GL_DST_COLOR, GL_ZERO); // Set the blending function for translucency (note off at init time)
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0); // Enables Clearing Of The Depth Buffer
	glDepthFunc(GL_LEQUAL); // type of depth test to do.
	glEnable(GL_DEPTH_TEST); // enables depth testing.
	glShadeModel(GL_SMOOTH); // Enables Smooth Color Shading

	glDisable(GL_LIGHTING);
}

void reSizeGlScene(GLsizei width, GLsizei height) {
	if (height == 0) // Prevent A Divide By Zero If The Window Is Too Small
		height = 1;

	// Every screen sets its own projection each frame (the scene's in graphics/draw.cpp), so only the viewport here.
	glViewport(0, 0, width, height);
	LOG_INFOF("graphics", "Resized to : %d x %d", width, height);
	Game().render.resX = width;
	Game().render.resY = height;
	// A window resized by hand opens at that size next time (saved on quit); fullscreen keeps the window's size.
	Settings::Display& display = Game().settings.display;
	if (!display.fullscreen && !Scenario::active()) {
		display.width = std::clamp(static_cast<int>(width), MIN_WINDOW_W, MAX_WINDOW_W);
		display.height = std::clamp(static_cast<int>(height), MIN_WINDOW_H, MAX_WINDOW_H);
	}
}

// SDL redefines main as SDL_main; without this, Windows builds fail with "WinMain@16" undefined.
#undef main

// The command line: a scenario run, a hand-played level or scenario (Scenario::loadPlay), or the plain game.
// False: bad arguments, usage printed.
static bool parseArgs(int argc, char* argv[]) {
	std::vector<std::string> args;
	for (int i = 1; i < argc; i++) {
		if (std::string(argv[i]) == "--no-sound")
			setenv("SDL_AUDIODRIVER", "dummy", 1); // no music or sound effects, for runs in the background
		else
			args.emplace_back(argv[i]);
	}
	const std::string flag = args.empty() ? "" : args[0];
	bool ok = true;
	if (args.size() == 2 && flag == "--level")
		ok = Scenario::loadPlayLevel(Paths::fromLaunchDir(args[1]).c_str());
	else if (args.size() == 2 && flag == "--play")
		ok = Scenario::loadPlay(Paths::fromLaunchDir(args[1]).c_str());
	else if (args.size() == 1 && flag.rfind("--", 0) != 0) {
		ok = Scenario::load(Paths::fromLaunchDir(args[0]).c_str());
		setenv("SDL_AUDIODRIVER", "dummy", 1);
		GameClock::enableVirtual();
	} else if (!args.empty()) {
		fprintf(stderr, "usage: %s [--no-sound] [--level N|PATH|gen:SEED:D | --play SCENARIO.txt | SCENARIO.txt]\n",
				argv[0]);
		ok = false;
	}
	return ok;
}

int main(int argc, char* argv[]) {
	Paths::enterRootDir(); // game.log, saves/ and the media from the repo root, wherever bin/game starts
	Logger::initialize();

	if (!parseArgs(argc, argv))
		return 2;
	bool isScenario = Scenario::active();

	try {
		if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
			LOG_ERRORF("game", "SDL initialization failed: %s", SDL_GetError());
			return -1;
		}

		// Initialize game state
		CreateGame();
		if (const std::string out = Scenario::outputDir(); !out.empty()) // never the real board in saves/
			Game().leaderboard.SetPath(out + "/leaderboard.txt");
		if (isScenario) {
			Game().render.resX = Scenario::resolutionX();
			Game().render.resY = Scenario::resolutionY();
			Game().settings.graphics.cutScenes = false; // timings stay as they were; `cutscenes on` turns them on
		} else {
			Game().random.Seed(static_cast<uint64_t>(std::time(nullptr))); // a scenario seeds per level instead
			SettingsFile::Load(Game().settings);
			Game().render.resX = Game().settings.display.width;
			Game().render.resY = Game().settings.display.height;
		}
		Game().ApplySettings(false);

		glutInit(&argc, argv);
		glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_ALPHA);

		glutInitWindowSize(Game().render.resX, Game().render.resY);

		glutInitWindowPosition(0, 0);

		window = glutCreateWindow("Dungeon Crawl");
		if (Game().settings.display.fullscreen)
			glutFullScreen();

		glutReshapeFunc(&reSizeGlScene);

		if (isScenario) {
			// No idle redraws and no user input: the script drives every frame.
			glutDisplayFunc(&noopDisplay);
			glutTimerFunc(Scenario::tickDelayMs(), scenarioTickCallback, 0);
		} else {
			glutDisplayFunc(&Draw);

			glutIdleFunc(&Idle);

			glutTimerFunc(UPDATE_TICK_MS, UpdateTimerCallback, 0);

			glutKeyboardFunc(&keyPressed);
			glutKeyboardUpFunc(&keyReleased);

			glutSpecialFunc(&specialKeyPressed);
			glutSpecialUpFunc(&specialKeyReleased);

			glutMouseFunc(processMouse);
			glutMotionFunc(processMouseActiveMotion);
			glutPassiveMotionFunc(processMousePassiveMotion);
			glutEntryFunc(processMouseEntry);
		}

		initGl(Game().render.resX, Game().render.resY);

		// Quit (and closing the window) return from the loop, so the cleanup below runs.
		glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
		glutMainLoop();

		// Cleanup
		SettingsFile::Save(Game().settings); // only writes when the window was resized
		DestroyGame();
		Logger::shutdown();
		SDL_Quit();
	} catch (const std::exception& e) {
		LOG_ERRORF("game", "Fatal error: %s", e.what());
		return -1;
	}

	return 0;
}
