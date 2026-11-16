#include "game_state.h"
#include "ui_context.h"
#include "../graphics/loading_text.h"
#include "settings.h"
#include "../ui/player_hud.h"
#include <GL/gl.h>
#include "../render/gl_includes.h"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include "../core/logger.h"
#include <memory>
#include "../level/gameplay_player.h"
#include "../level/texts.h"
#include "../render/animated_model.h"
#include "../render/brightness.h"
#include "../render/ink.h"
#include "../graphics/frame_rate.h"
#include "../render/lighting.h"
#include "../graphics/particles_draw.h"
#include "../core/paths.h"
#include <utility>

namespace {
std::unique_ptr<GameState> gGame;
} // namespace

void CreateGame() { gGame = std::make_unique<GameState>(); }

void DestroyGame() { gGame.reset(); }

GameState& Game() {
	assert(gGame && "CreateGame() not called");
	return *gGame;
}

GameState::GameState()
	: uiOwned(
		  std::make_unique<UIContext>(ScreenWorld{
			  assets,
			  render,
			  settings,
			  *player,
			  bag,
			  itemCommands,
			  journal,
			  dungeon,
			  events,
			  random,
			  leaderboard,
			  saves,
			  messages,
			  inscriptions,
			  [this](bool save) { ApplySettings(save); },
			  [this] { NewGame(); },
			  alwaysGift
		  })
	  ),
	  ui(*uiOwned) {}

GameState::~GameState() {
	void* selfPtr = this;
	LOG_DEBUGF("game", "Deleting cashe %p", selfPtr);
}

UiScreen* UIContext::TabScreen() {
	switch (screen) {
	case Screen::Inventory:
		return inventory.get();
	case Screen::Map:
		return &map;
	case Screen::Journal:
		return &journal;
	case Screen::Skills:
		return &skills;
	case Screen::Menu:
	case Screen::Riddle:
	case Screen::Death:
	case Screen::Gameplay:
		return nullptr;
	}
	return nullptr;
}

void GameState::Load() {
	assets.LoadLoadingScreen();
	assets.DrawLoadingScreen(10, "Loading", "");
	assets.Load([this](float percent, const char* step, const char* detail) {
		assets.DrawLoadingScreen(percent, step, detail);
	});
	ui.riddle = std::make_unique<Riddle>(ui.world, ui);
	ui.inventory = std::make_unique<Inventory>(ui.world, ui);

	Texture playerTexture;
	playerTexture.LoadPNG(MEDIA_DIR "textures/characters/archeologist.png");
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
	dungeon.Link({player.get(), &journal, &bag, &random, &events, &assets.monsterTypes, &inscriptions, &names});

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
bool isGustSound(WorldSound sound) { return sound == WorldSound::GustRise || sound == WorldSound::GustBlow; }
} // namespace

void GameState::ApplyWorldEvents() {
	for (const WorldEvent& event : events.Take())
		switch (event.kind) {
		case WorldEvent::Kind::Sound: // a gust's wind drowns the rest (plan sandstorm-halls)
			assets.sounds.Of(event.sound).Play(isGustSound(event.sound) ? 1.f : 1.f - dungeon.SoundMuffle());
			break;
		case WorldEvent::Kind::Status:
			PushStatus(event.text, event.tone);
			break;
		case WorldEvent::Kind::Note:
			journal.LearnNote(event.note);
			break;
		case WorldEvent::Kind::CharacterSound:
			(event.who == PLAYER_CHARACTER ? playerView.Model() : assets.monsterModels[static_cast<size_t>(event.who)])
				.SoundOf(event.character)
				.Play(1.f - dungeon.SoundMuffle());
			break;
		case WorldEvent::Kind::AskRiddle:
			ui.riddle->Ask();
			ui.screen = Screen::Riddle;
			break;
		case WorldEvent::Kind::Autosave:
			saves.Autosave(event.moment);
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
	itemCommands.Reset();
	ui.inventory->Reset();
	journal.Begin();
	ApplyViewState(UiViewState{}); // facing right, the first tabs and pages
	dungeon.LoadCampaignLevel(1);
	dungeon.ClearWin();
	player->Reanimate();
	saves.Autosave(AutosaveMoment::LevelStart);
	// The story starts in the notebook: the prologue, open before the first frame of play (plan journal-intro).
	if (journal.TakeIntro()) {
		ui.journal.OpenDiary("closes the journal");
		ui.screen = Screen::Journal;
	}
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
	Audio::SetVolumes(
		playedVolume(settings.sound.music, settings.sound),
		playedVolume(settings.sound.effects, settings.sound)
	);
	Brightness::set(settings.display.brightness);
	if (save)
		SettingsFile::Save(settings);
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
