#include "game_state.h"
#include "loading_text.h"
#include "settings.h"
#include "../ui/player_hud.h"
#include <filesystem>
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
#include "../world/texts.h"
#include "../graphics/animated_model.h"
#include "../graphics/ink.h"
#include "../graphics/frame_rate.h"
#include "../graphics/lighting.h"
#include "../graphics/particles_draw.h"
#include "../core/paths.h"
#include "../test/scenario.h"
#include <sstream>

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
	DrawLoad(10, "Loading", "");
	assets.Load([this](float percent, const char* step, const char* detail) { DrawLoad(percent, step, detail); });
	ui.riddle = std::make_unique<Riddle>();
	ui.inventory = std::make_unique<Inventory>();
	ui.endScreens = std::make_unique<EndScreens>();

	Texture playerTexture;
	playerTexture.LoadPNG(MEDIA_DIR "textures/characters/archeologist.png");
	player = std::make_unique<Player>();
	playerView.Load("characters/archeologist", std::move(playerTexture), *player);
	player->scale = PLAYER_SCALE;
	for (const std::string& message : inscriptions.Load(CONTENT_DIR "inscriptions"))
		LOG_WARNINGF("game", "Inscriptions: %s", message.c_str());
	LOG_INFO("game", "Loaded " + std::to_string(inscriptions.Size()) + " inscriptions");
	for (const std::string& message : names.Load(CONTENT_DIR "names"))
		LOG_WARNINGF("game", "Names: %s", message.c_str());
	LOG_INFO("game", "Loaded " + std::to_string(names.Size()) + " royal names");
	std::vector<std::string> signProblems = assets.decor.hieroglyphs.Load(HIEROGLYPH_MANIFEST);
	for (const std::string& message : checkSigns(assets.decor.hieroglyphs, inscriptions, names))
		signProblems.push_back(message);
	for (const std::string& message : signProblems)
		LOG_WARNINGF("game", "Hieroglyphs: %s", message.c_str());
	// The journal's and the cards' prose (plan texts-to-content), read here once instead of on the first page shown.
	for (const std::string& message : textProblems())
		LOG_WARNINGF("game", "Texts: %s", message.c_str());
	LOG_INFO("game", "Loaded " + std::to_string(gameTexts().Size()) + " texts");
	dungeon.Link(
		{player.get(),
		 &journal,
		 &ui.inventory->Bag(),
		 &random,
		 &assets,
		 &events,
		 &assets.monsterTypes,
		 &inscriptions,
		 &names}
	);

	timers.idleModel.Reset();
	statusTimer = Timer(STATUS_MS);

	if (!dungeon.LoadCampaignLevel(dungeon.LevelNumber()))
		LOG_WARNING("game", "Failed loading map");

	assets.sounds.soundtrack.Play();

	saves.LoadNames();
	leaderboard.Load();

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
	case WorldSound::SandHiss:
		return sounds.sandHiss;
	case WorldSound::SandPour:
		return sounds.sandPour;
	case WorldSound::SandStep:
		return sounds.sandStep;
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
	case WorldSound::SpikeScrape:
		return sounds.spikeScrape;
	case WorldSound::SpikeThunk:
		return sounds.spikeThunk;
	case WorldSound::FireHiss:
		return sounds.fireHiss;
	case WorldSound::FireRoar:
		return sounds.fireRoar;
	case WorldSound::DripPlips:
		return sounds.dripPlips;
	case WorldSound::DripCreak:
		return sounds.dripCreak;
	case WorldSound::DripSplat:
		return sounds.dripSplat;
	case WorldSound::MirrorTurn:
		return sounds.mirrorTurn;
	case WorldSound::SunDisc:
		return sounds.sunDisc;
	case WorldSound::RingsDown:
		return sounds.ringsDown;
	case WorldSound::RingsUp:
		return sounds.ringsUp;
	case WorldSound::ChevronLock:
		return sounds.chevronLock;
	case WorldSound::ChevronFail:
		return sounds.chevronFail;
	case WorldSound::DialPress:
		return sounds.dialPress;
	case WorldSound::Dodge:
		return sounds.dodge;
	case WorldSound::PowerHit:
		return sounds.powerHit;
	case WorldSound::LidWood:
		return sounds.lidWood;
	case WorldSound::LidReed:
		return sounds.lidReed;
	case WorldSound::LidStone:
		return sounds.lidStone;
	case WorldSound::JarSmash:
		return sounds.jarSmash;
	case WorldSound::PagePickup:
		return sounds.pageTurn;
	case WorldSound::DungThud:
		return sounds.dungThud;
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
	case CharacterSound::AttackRanged:
		return model.attackRangedSound;
	case CharacterSound::Roar:
		return model.roarSound;
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
			characterSound(
				event.who == PLAYER_CHARACTER ? playerView.Model()
											  : assets.monsterModels[static_cast<size_t>(event.who)],
				event.character
			)
				.Play();
			break;
		case WorldEvent::Kind::AskRiddle:
			ui.riddle->Ask();
			ui.screen = Screen::Riddle;
			break;
		case WorldEvent::Kind::Autosave:
			Autosave(event.moment);
			break;
		case WorldEvent::Kind::AskName: // plan secret-name-gates
			if (const RoyalName* name = dungeon.LevelName()) {
				ui.riddle->AskName(*name);
				ui.screen = Screen::Riddle;
			} else
				ShowStatus("The gate is silent.");
			break;
		}
}
//==============================================================
void GameState::NewGame() {
	PlayerHud::reset();
	player->stats = PlayerStats{};
	player->stats.run.campaign = true; // from New Game on level 1: the win goes on the leaderboard
	player->stats.run.nameSeed = random.gameplay.next(); // each level's royal name (plan secret-name-gates)
	ui.inventory->Reset();
	journal.Begin();
	dungeon.LoadCampaignLevel(1);
	dungeon.ClearWin();
	player->Reanimate();
	Autosave(AutosaveMoment::LevelStart);
	// The story starts in the notebook: the prologue, open before the first frame of play (plan journal-intro).
	if (journal.TakeIntro()) {
		ui.journal.OpenIntro();
		ui.screen = Screen::Journal;
	}
}
//==============================================================
// Scenarios never touch saves/: theirs lands in tests/out/<name>/.
std::string GameState::AutosavePath() const {
	return Scenario::active() ? Scenario::outputDir() + "/autosave.sav" : SaveSlots::AUTOSAVE;
}

bool GameState::HasAutosave() const { return SaveSlots::DescribeFile(AutosavePath()).used; }

void GameState::Autosave(AutosaveMoment moment) {
	if (!player->Alive() || (moment != AutosaveMoment::LevelStart && dungeon.PlayerInDanger()))
		return;
	Save(AutosavePath().c_str());
	ui.autosaved.Show("Autosaved", GameClock::now());
}

void GameState::LoadAutosave() { LoadSaveIntoGame(AutosavePath()); }

void GameState::LoadSaveIntoGame(const std::string& path) {
	LoadSave(path.c_str());
	ui.screen = Screen::Gameplay;
	ui.menu.ResetSubScreens();
	ui.menu.inGame = true;
}

std::string GameState::QuicksavePath() const {
	return Scenario::active() ? Scenario::outputDir() + "/quicksave.sav" : SaveSlots::QUICKSAVE;
}

bool GameState::HasQuicksave() const { return SaveSlots::DescribeFile(QuicksavePath()).used; }

void GameState::Quicksave() {
	if (!player->Alive())
		return;
	Save(QuicksavePath().c_str());
	ui.autosaved.Show("Quick saved", GameClock::now());
}

void GameState::LoadQuicksave() {
	if (!HasQuicksave()) {
		ui.autosaved.Show("No quick save", GameClock::now());
		return;
	}
	LoadSaveIntoGame(QuicksavePath());
	ui.autosaved.Show("Quick loaded", GameClock::now());
}

std::string GameState::SlotPath(int slot) const {
	return Scenario::active() ? Scenario::outputDir() + "/save" + std::to_string(slot) + ".sav"
							  : SaveSlots::FileName(slot);
}

std::optional<SaveSlots::Latest> GameState::LatestSave() const {
	std::array<std::string, SaveSlots::COUNT> slots;
	for (int slot = 0; slot < SaveSlots::COUNT; slot++)
		slots[static_cast<size_t>(slot)] = SlotPath(slot);
	return SaveSlots::PickLatest(AutosavePath(), slots, QuicksavePath());
}

void GameState::LoadLatestSave() {
	if (const auto latest = LatestSave())
		LoadSaveIntoGame(latest->path);
}

// A load puts the saved count back, so the file learns the death now: a retry, or Continue after a restart, keeps it.
void GameState::CountDeath() {
	const int deaths = ++player->stats.run.deaths;
	std::string text;
	{
		std::ifstream in(AutosavePath());
		if (!in)
			return;
		std::stringstream all;
		all << in.rdbuf();
		text = all.str();
	}
	if (!RunStats::SetDeaths(text, deaths)) {
		LOG_WARNING("game", "The autosave has no run statistics: its deaths stay");
		return;
	}
	// The file's time stays the save's own: the newest save (plan retry-latest-save) goes by it.
	std::error_code error;
	const auto written = std::filesystem::last_write_time(AutosavePath(), error);
	{
		std::ofstream out(AutosavePath());
		out << text;
	}
	if (!error)
		std::filesystem::last_write_time(AutosavePath(), written, error);
}
//==============================================================
void GameState::ApplySettings(bool save) {
	Ink::setToon(settings.graphics.toon);
	Particles::shown = settings.graphics.blood;
	Lighting::setFlicker(settings.graphics.lightFlicker);
	AnimatedModel::SetAnimationRate(settings.display.animationRate);
	FrameRate::set(settings.display.frameRate);
	dungeon.SetCutScenes(settings.graphics.cutScenes);
	dungeon.SetMotionEffects(settings.graphics.motionEffects);
	Audio::SetVolumes(settings.sound.music, settings.sound.effects);
	if (save)
		SettingsFile::Save(settings);
}
//==============================================================
void GameState::DrawLoad(float xxx, const char step[], const char detail[]) {
	LOG_INFOF("loading", "DrawLoad %.0f%%: %s [%s]", static_cast<double>(xxx), step, detail);

	if (xxx > 100)
		xxx = 100;

	xxx *= 1.18;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear The Screen And The Depth Buffer
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION); // Select The Projection Matrix
	glLoadIdentity(); // Reset The Projection Matrix
	glOrtho(0, 140, 0, 140, -200, 200); // Set Up An Ortho Screen
	glMatrixMode(GL_MODELVIEW); // Select The Modelview Matrix

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
	assets.fonts.loading.print(loading_text::LEFT, 17, step);
	glColor3f(0.9f, 0.9f, 0.9f); // the detail line, a little dimmer
	assets.fonts.loadingDetail.print(loading_text::LEFT, 9.5f, detail);
	glColor3f(1, 1, 1);
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
	dungeon.ClearWin(); // a save is never of a won game

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
