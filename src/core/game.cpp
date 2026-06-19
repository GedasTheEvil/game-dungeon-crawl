#include "../graphics/gl_includes.h"

#include <GL/glu.h>
#include <GL/gl.h>
#include <SDL/SDL.h>
#include <stdlib.h>
#include <stdexcept>
#include "../input/input.h"
#include "../graphics/textures.h"
#include "sound.h"
#include "service_locator.h"
#include "logger.h"

int window = 1;
int fs = 0;

static void UpdateTimerCallback(int) {
	Update();
	glutTimerFunc(16, UpdateTimerCallback, 0);
}

void initGl(GLsizei width, GLsizei height) // We call this right after our OpenGL window is created.
{
	(void)width;
	(void)height;

	glEnable(GL_TEXTURE_2D); // Enable texture mapping.

	glBlendFunc(GL_DST_COLOR, GL_ZERO); // Set the blending function for translucency (note off at init time)
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0);		 // Enables Clearing Of The Depth Buffer
	glDepthFunc(GL_LEQUAL);	 // type of depth test to do.
	glEnable(GL_DEPTH_TEST); // enables depth testing.
	glShadeModel(GL_SMOOTH); // Enables Smooth Color Shading

	glDisable(GL_LIGHTING);
}

void reSizeGlScene(GLsizei width, GLsizei height) {
	if (height == 0) // Prevent A Divide By Zero If The Window Is Too Small
		height = 1;

	glViewport(0, 0, width, height); // Reset The Current Viewport And Perspective Transformation

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(45.0f, static_cast<GLfloat>(width) / static_cast<GLfloat>(height), 0.1f, 10000.0f);
	glMatrixMode(GL_MODELVIEW);
	LOG_INFOF("graphics", "Resized to : %d x %d", width, height);
	GAME_STATE.render.resX = width;
	GAME_STATE.render.resY = height;
}

// sita eilute reikalinga, kad kompiliuojant per win nemestu erroro su "WinMain@16" undefined
#undef main

int main(int argc, char* argv[]) {
	Logger::initialize();

	try {
		if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
			LOG_ERRORF("game", "SDL initialization failed: %s", SDL_GetError());
			return -1;
		}

		// Initialize game state
		ServiceLocator::initialize(std::make_unique<GameState>());

		glutInit(&argc, argv);
		glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_ALPHA);

		glutInitWindowSize(GAME_STATE.render.resX, GAME_STATE.render.resY);

		glutInitWindowPosition(0, 0);

		window = glutCreateWindow("Dungeon Crawl");

		glutDisplayFunc(&Draw);

		if (fs)
			glutFullScreen();

		glutIdleFunc(&Idle);

		glutTimerFunc(16, UpdateTimerCallback, 0);

		glutReshapeFunc(&reSizeGlScene);

		glutKeyboardFunc(&keyPressed);

		glutSpecialFunc(&specialKeyPressed);
		glutSpecialUpFunc(&specialKeyReleased);

		glutMouseFunc(processMouse);
		glutMotionFunc(processMouseActiveMotion);
		glutPassiveMotionFunc(processMousePassiveMotion);
		glutEntryFunc(processMouseEntry);

		initGl(GAME_STATE.render.resX, GAME_STATE.render.resY);

		glutMainLoop();

		// Cleanup
		ServiceLocator::shutdown();
		Logger::shutdown();
		SDL_Quit();
	} catch (const std::exception& e) {
		LOG_ERRORF("game", "Fatal error: %s", e.what());
		return -1;
	}

	return 1;
}
