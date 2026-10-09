#include "game_state.h"
#include "settings.h"
#include "../ui/player_hud.h"
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
#include "../graphics/ink.h"
#include "../graphics/lighting.h"
#include "../graphics/particles.h"

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
	playerView.Load("characters/archeologist", std::move(playerTexture), *player);
	player->scale = PLAYER_SCALE;
	dungeon.Link({player.get(), &journal, &ui.inventory->Bag(), &random, &assets, &events, &assets.monsterTypes});

	timers.idleModel.Reset();
	statusTimer = Timer(STATUS_MS);

	if (!dungeon.LoadCampaignLevel(dungeon.LevelNumber()))
		LOG_WARNING("game", "Failed loading map");

	assets.sounds.soundtrack.Play();

	saves.LoadNames();

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
namespace {
Sound& soundOf(SoundBank& sounds, WorldSound sound) {
	switch (sound) {
	case WorldSound::ArrowHit:
		return sounds.arrowHit;
	case WorldSound::ArrowWall:
		return sounds.arrowWall;
	case WorldSound::StoneHit:
		return sounds.stoneHit;
	case WorldSound::StoneWall:
		return sounds.stoneWall;
	case WorldSound::KeyPickup:
		return sounds.keyPickup;
	case WorldSound::GateOpen:
		return sounds.gateOpen;
	case WorldSound::GateLocked:
		return sounds.gateLocked;
	case WorldSound::Lever:
		return sounds.lever;
	case WorldSound::RockRumble:
		return sounds.rockRumble;
	case WorldSound::RockCrash:
		return sounds.rockCrash;
	case WorldSound::Teleport:
		return sounds.teleport;
	case WorldSound::SummonDig:
		return sounds.summonDig;
	case WorldSound::SummonDrop:
		return sounds.summonDrop;
	case WorldSound::Wade:
		return sounds.wade;
	case WorldSound::Splash:
		return sounds.splash;
	case WorldSound::PlateClick:
		return sounds.plateClick;
	case WorldSound::Dart:
		return sounds.dart;
	}
	return sounds.arrowHit;
}
const Sound& characterSound(const CharacterModel& model, CharacterSound sound) {
	switch (sound) {
	case CharacterSound::Die:
		return model.dieSound;
	case CharacterSound::Attack:
		return model.attackSound;
	case CharacterSound::Jump:
		return model.jumpSound;
	case CharacterSound::Wake:
		return model.wakeSound;
	case CharacterSound::Spit:
		return model.spitSound;
	}
	return model.dieSound;
}
} // namespace

void GameState::ApplyWorldEvents() {
	for (const WorldEvent& event : events.Take())
		switch (event.kind) {
		case WorldEvent::Kind::Sound:
			soundOf(assets.sounds, event.sound).Play();
			break;
		case WorldEvent::Kind::Status:
			ShowStatus("%s", event.text.c_str());
			break;
		case WorldEvent::Kind::Note:
			journal.LearnNote(event.note);
			break;
		case WorldEvent::Kind::CharacterSound:
			characterSound(event.who == PLAYER_CHARACTER ? playerView.Model()
														 : assets.monsterModels[static_cast<size_t>(event.who)],
						   event.character)
				.Play();
			break;
		case WorldEvent::Kind::AskRiddle:
			ui.riddle->Ask();
			ui.screen = Screen::Riddle;
			break;
		}
}
//==============================================================
void GameState::NewGame() {
	PlayerHud::reset();
	player->stats = PlayerStats{};
	ui.inventory->Reset();
	journal.Clear();
	dungeon.LoadCampaignLevel(1);
	dungeon.ClearWin();
	player->Reanimate();
}
//==============================================================
void GameState::ApplySettings(bool save) {
	Ink::setToon(settings.graphics.toon);
	Particles::shown = settings.graphics.blood;
	Lighting::setFlicker(settings.graphics.lightFlicker);
	Audio::SetVolumes(settings.sound.music, settings.sound.effects);
	if (save)
		SettingsFile::Save(settings);
}
//==============================================================
void GameState::DrawLoad(float xxx, const char text[]) {
	LOG_INFOF("loading", "DrawLoad %.0f%%: %s", static_cast<double>(xxx), text);

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

	dump << dungeon.LevelNumber() << " ";

	player->stats.Dump(dump);
	ui.inventory->Dump(dump);
	dungeon.Dump(dump);
	journal.Dump(dump);

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

	int levelNumber = 1;
	dump >> levelNumber;
	dungeon.SetLevelNumber(levelNumber);
	LOG_INFOF("game", "Got MapNo : %d", levelNumber);

	player->stats.LoadDump(dump);
	LOG_INFO("game", "Done loading Stats");
	ui.inventory->LoadDump(dump);
	LOG_INFO("game", "Done loading Inventory");
	player->stats.Wear(amuletBonus(ui.inventory->Bag().Worn()), false); // the saved HP is already the worn one's
	dungeon.LoadDump(dump);
	journal.Load(dump);
	dungeon.scatterDecorations(campaignLevelFile(levelNumber).c_str(), levelNumber);
	LOG_INFO("game", "Done loading map");
	dump.close();
	PlayerHud::reset();
}
//==============================================================
