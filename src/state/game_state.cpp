#include "game_state.h"
#include <GL/gl.h>
#include "../graphics/gl_includes.h"
#include <fstream>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include "../core/logger.h"
#include <memory>
#include "../core/gameplay_config.h"
#include "../world/campaign.h"

namespace {
std::unique_ptr<GameState> gGame;
} // namespace

void CreateGame() { gGame = std::make_unique<GameState>(); }

void DestroyGame() { gGame.reset(); }

GameState& Game() {
	assert(gGame && "CreateGame() not called");
	return *gGame;
}

GameState::GameState() = default;

GameState::~GameState() {
	void* selfPtr = this;
	LOG_DEBUGF("game", "Deleting cashe %p", selfPtr);
}

void GameState::Load() {
	assets.LoadLoadingScreen();
	DrawLoad(10, "Loading");
	assets.Load([this](float percent, const char* text) { DrawLoad(percent, text); });
	ui.riddle = std::make_unique<Riddle>();
	ui.inventory = std::make_unique<Inventory>();
	ui.endScreens = std::make_unique<EndScreens>();

	Texture playerTexture;
	playerTexture.LoadPNG("textures/characters/archeologist.png");
	player = std::make_unique<Player>();
	player->Load("characters/archeologist", playerTexture);
	player->scale = 15;

	timers.idleModel.Reset();
	statusTimer = Timer(STATUS_MS);

	if (!dungeon.LoadCampaignLevel(curMap))
		LOG_WARNING("game", "Failed loading map");

	assets.sounds.soundtrack.Play();

	std::ifstream f("saves/gamelist.dat");
	if (f) {
		std::string token; // one name per line, cut to fit (a long one would overrun the buffer)
		for (auto& saveName : saveNames)
			if (f >> token)
				snprintf(saveName.name, sizeof(saveName.name), "%s", token.c_str());
	} else {
		LOG_WARNING("game", "Failed loading save list");
	}

	status.clear();
	cacheLoaded = true;
}
//==============================================================
void GameState::ShowStatus(const char* format, ...) {
	char buf[256];
	va_list args;
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	status = buf;
	statusTimer.Reset();
}
//==============================================================
void GameState::NewGame() {
	player->stats = PlayerStats{};
	ui.inventory->Reset();
	curMap = 1;
	dungeon.LoadCampaignLevel(curMap);
	hasWon = false;
	player->Reanimate();
}
//==============================================================
void GameState::DrawLoad(float xxx, const char text[]) {
	LOG_INFOF("loading", "DrawLoad: %s", text);

	if (xxx > 100)
		xxx = 100;

	xxx *= 1.18;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear The Screen And The Depth Buffer
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);		// Select The Projection Matrix
	glLoadIdentity();					// Reset The Projection Matrix
	glOrtho(0, 140, 0, 140, -200, 200); // Set Up An Ortho Screen
	glMatrixMode(GL_MODELVIEW);			// Select The Modelview Matrix

	// Background image
	assets.textures.loadingBackground.Bind();

	glBegin(GL_QUADS);
	glNormal3f(0, 0, 1);
	glTexCoord2f(0, 0);
	glVertex3i(0, 0, -40);
	glTexCoord2f(0, 1);
	glVertex3i(0, 140, -40);
	glTexCoord2f(1, 1);
	glVertex3i(140, 140, -40);
	glTexCoord2f(1, 0);
	glVertex3i(140, 0, -40);
	glEnd();

	// progressbar
	assets.textures.loadingBar.Bind();

	glColor3f(1.2, 0.6, 0);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex3f(10, 28, 0);
	glTexCoord2f(1, 0);
	glVertex3f(xxx + 10, 28, 0);
	glTexCoord2f(1, 1);
	glVertex3f(xxx + 10, 38, 0);
	glTexCoord2f(0, 1);
	glVertex3f(10, 38, 0);
	glEnd();

	glColor3f(1, 1, 1);

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glEnable(GL_BLEND);
	assets.fonts.loading.print(10, 15, text);
	glDisable(GL_BLEND);

	glFlush();

	glutSwapBuffers();
}
//==============================================================
void GameState::Save(const char filename[]) {
	if (!cacheLoaded) {
		LOG_ERROR("game", "can't save without loading cashe");
		return;
	}

	std::ofstream dump(filename);
	if (!dump) {
		LOG_ERRORF("game", "can't open save file %s", filename);
		return;
	}

	dump << curMap << " ";

	player->stats.Dump(dump);
	ui.inventory->Dump(dump);
	dungeon.Dump(dump);

	dump.close();
}
//==============================================================
void GameState::LoadSave(const char filename[]) {
	if (!cacheLoaded) {
		LOG_ERROR("game", "can't load without loading cashe");
		return;
	}

	player->Reanimate();

	LOG_INFOF("game", "Loading save %s", filename);
	std::ifstream dump(filename);
	if (!dump) {
		LOG_ERRORF("game", "can't open save file %s", filename);
		return;
	}

	dump >> curMap;
	LOG_INFOF("game", "Got MapNo : %d", curMap);

	player->stats.LoadDump(dump);
	LOG_INFO("game", "Done loading Stats");
	ui.inventory->LoadDump(dump);
	LOG_INFO("game", "Done loading Inventory");
	dungeon.LoadDump(dump);
	dungeon.scatterDecorations(campaignLevelFile(curMap).c_str());
	LOG_INFO("game", "Done loading map");
	dump.close();
}
//==============================================================
