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
#include "../level/gameplay_config.h"
#include "../level/campaign.h"
#include "../level/texts.h"
#include "../sim/dungeon_save.h"
#include "../render/animated_model.h"
#include "../render/ink.h"
#include "../graphics/frame_rate.h"
#include "../render/lighting.h"
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

	if (!dungeon.LoadCampaignLevel(dungeon.LevelNumber()))
		LOG_WARNING("game", "Failed loading map");

	leaderboard.Load();

	messages.Clear();
	cacheLoaded = true;
}
//==============================================================
void GameState::ShowStatus(const char* format, ...) {
	char buf[256];
	va_list args;
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	PushStatus(buf, StatusTone::Plain);
}

void GameState::PushStatus(const std::string& text, StatusTone tone) {
	messages.Push(text, tone, GameClock::now(), dungeon.LevelNumber());
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
	case WorldSound::GustRise:
		return sounds.gustRise;
	case WorldSound::GustBlow:
		return sounds.gustBlow;
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
	case WorldSound::LidCoffin:
		return sounds.lidCoffin;
	case WorldSound::PagePickup:
		return sounds.pageTurn;
	case WorldSound::DungThud:
		return sounds.dungThud;
	case WorldSound::ShotDry:
		return sounds.shotDry;
	case WorldSound::ShotTooClose:
		return sounds.shotTooClose;
	case WorldSound::HeadBump:
		return sounds.headBump;
	case WorldSound::ShrinePray:
	case WorldSound::ShrinePrayMontu:
	case WorldSound::ShrinePrayPtah:
	case WorldSound::ShrinePrayAmun:
	case WorldSound::ShrinePraySerket:
		return sounds.shrinePray[static_cast<int>(sound) - static_cast<int>(WorldSound::ShrinePray)];
	}
	return sounds.arrowHit;
}
bool isGustSound(WorldSound sound) { return sound == WorldSound::GustRise || sound == WorldSound::GustBlow; }
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
		case WorldEvent::Kind::Sound: // a gust's wind drowns the rest (plan sandstorm-halls)
			soundOf(assets.sounds, event.sound).Play(isGustSound(event.sound) ? 1.f : 1.f - dungeon.SoundMuffle());
			break;
		case WorldEvent::Kind::Status:
			PushStatus(event.text, event.tone);
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
				.Play(1.f - dungeon.SoundMuffle());
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
	messages.Clear();
	player->stats = PlayerStats{};
	player->stats.run.campaign = true; // from New Game on level 1: the win goes on the leaderboard
	player->stats.run.nameSeed = random.gameplay.next(); // each level's royal name (plan secret-name-gates)
	ui.inventory->Reset();
	journal.Begin();
	ApplyViewState(UiViewState{}); // facing right, the first tabs and pages
	dungeon.LoadCampaignLevel(1);
	dungeon.ClearWin();
	player->Reanimate();
	Autosave(AutosaveMoment::LevelStart);
	// The story starts in the notebook: the prologue, open before the first frame of play (plan journal-intro).
	if (journal.TakeIntro()) {
		ui.journal.OpenDiary("closes the journal");
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
	if (!player->Alive() || dungeon.Won() || (moment != AutosaveMoment::LevelStart && dungeon.PlayerInDanger()))
		return;
	ui.autosaved.Show(Save(AutosavePath().c_str()) ? "Autosaved" : "Autosave failed", GameClock::now());
}

bool GameState::LoadAutosave() { return LoadSaveIntoGame(AutosavePath()); }

bool GameState::LoadSaveIntoGame(const std::string& path) {
	if (!LoadSave(path.c_str()))
		return false;
	ui.screen = Screen::Gameplay;
	ui.menu.ResetSubScreens();
	ui.menu.inGame = true;
	return true;
}

bool GameState::UnsavedProgress() const {
	constexpr int UNSAVED_PLAY_MS = 3000; // a look around after a load is no progress
	return player->stats.run.PlayMs() - savedPlayMs >= UNSAVED_PLAY_MS;
}

std::string GameState::QuicksavePath() const {
	return Scenario::active() ? Scenario::outputDir() + "/quicksave.sav" : SaveSlots::QUICKSAVE;
}

bool GameState::HasQuicksave() const { return SaveSlots::DescribeFile(QuicksavePath()).used; }

void GameState::Quicksave() {
	if (!player->Alive() || dungeon.Won()) // a save is never of a won game (plan win-ending-flow)
		return;
	ui.autosaved.Show(Save(QuicksavePath().c_str()) ? "Quick saved" : "Quick save failed", GameClock::now());
}

void GameState::LoadQuicksave() {
	if (!HasQuicksave()) {
		ui.autosaved.Show("No quick save", GameClock::now());
		return;
	}
	ui.autosaved.Show(
		LoadSaveIntoGame(QuicksavePath()) ? "Quick loaded" : "Could not load the quick save",
		GameClock::now()
	);
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

bool GameState::LoadLatestSave() {
	const auto latest = LatestSave();
	return latest && LoadSaveIntoGame(latest->path);
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
// The settings a save keeps (plan save-remembers-ui-state): the player's look and the two screens' pages.
UiViewState GameState::ViewState() const {
	UiViewState view;
	view.facing = camera.Facing();
	ui.inventory->ViewTo(view);
	ui.journal.ViewTo(view);
	return view;
}

void GameState::ApplyViewState(const UiViewState& view) {
	camera.rotW = view.facing > 0 ? 70.f : -110.f;
	player->rotA = camera.rotW;
	ui.inventory->ViewFrom(view);
	ui.journal.ViewFrom(view);
}
//==============================================================
bool GameState::Save(const char filename[]) {
	if (!cacheLoaded) {
		LOG_ERROR("game", "can't save without loading cashe");
		return false;
	}
	std::ostringstream text;
	UiViewState view = ViewState();
	writeSaveGame(text, SaveParts{dungeon, *player, ui.inventory->Bag(), journal, view});
	if (!writeFileSafely(filename, text.str())) {
		LOG_ERRORF("game", "can't write save file %s", filename);
		return false;
	}
	savedPlayMs = player->stats.run.PlayMs();
	return true;
}
//==============================================================
// Read and checked before the game changes: a missing or broken file leaves the game as it was (plan
// save-load-safety).
bool GameState::LoadSave(const char filename[]) {
	if (!cacheLoaded) {
		LOG_ERROR("game", "can't load without loading cashe");
		return false;
	}
	LOG_INFOF("game", "Loading save %s", filename);
	std::string text;
	if (!readWholeFile(filename, text)) {
		LOG_ERRORF("game", "can't open save file %s", filename);
		return false;
	}
	UiViewState view = ViewState(); // a broken save puts these back too
	const bool loaded = loadSaveGame(text, SaveParts{dungeon, *player, ui.inventory->Bag(), journal, view});
	ui.inventory->BagLoaded();
	ApplyViewState(view);
	if (!loaded) {
		LOG_ERRORF("game", "broken save file %s", filename);
		return false;
	}
	messages.Clear(); // the log is not saved
	dungeon.ClearWin(); // a save is never of a won game
	const int levelNumber = dungeon.LevelNumber();
	dungeon.scatterDecorations(campaignLevelFile(levelNumber).c_str(), levelNumber);
	LOG_INFOF("game", "Loaded level %d", levelNumber);
	PlayerHud::reset();
	savedPlayMs = player->stats.run.PlayMs();
	return true;
}
//==============================================================
