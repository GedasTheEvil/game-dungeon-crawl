#ifndef SIM_WORLD_H
#define SIM_WORLD_H

#include "../../src/level/tuning/gameplay_player.h"
#include "../../src/level/tuning/gameplay_shared.h"
#include "../../src/level/tuning/gameplay_world.h"
#include "../../src/core/timer.h"
#include "../../src/sim/model/model_info.h"
#include "../../src/sim/monster/monster.h"
#include "../../src/sim/player.h"
#include "../../src/sim/player_control.h"
#include "../../src/sim/model/render_config.h"
#include "../../src/level/campaign.h"
#include "../../src/sim/dungeon/dungeon.h"
#include "../../src/sim/dungeon/dungeon_save.h"
#include "../../src/level/rules/item_bag.h"
#include "../../src/level/rules/items.h"
#include "../../src/level/inscriptions.h"
#include "../../src/level/rules/journal.h"
#include "../../src/level/files/names.h"
#include "../../src/level/rules/player_actions.h"
#include "../../src/core/rng.h"
#include "../../src/level/rules/world_events.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// A shared generator for the calls that need one and never roll it (PlayerStats::LayCurse without the skill).
inline Rng& testRng() {
	static Rng rng(1);
	return rng;
}

// The game's world without the game (plan sim-library): a level, its monsters and the player, stepped on the virtual
// clock one UPDATE_TICK_MS tick at a time as the game loop does (game_loop.cpp), without the screens, the camera or the
// drawing. The keys are its methods, as the scenario commands are (docs/testing.md): Walk, Climb, Jump, Interact,
// Attack. The monster types and the player's measures come from the real model files.
class SimWorld {
  public:
	Player player;
	Journal journal;
	InscriptionBook inscriptions; // empty: a test adds the texts it reads
	NameList names; // empty: a test loads the royal names it needs (plan secret-name-gates)
	ItemBag items;
	GameRandom random;
	WorldEvents events;
	std::vector<WorldEvent> said; // every event since the level loaded, oldest first
	Dungeon dungeon;
	int facing = -1; // the camera's: -1 left (as the game starts), +1 right; walking turns it
	bool refusalTold = false; // this press of the attack key was refused and told (PlayerControl)

	SimWorld() {
		GameClock::enableVirtual();
		player.SetModel(playerModel());
		player.scale = PLAYER_SCALE;
		dungeon.Link({&player, &journal, &items, &random, &events, &monsterTypes(), &inscriptions, &names});
	}

	// As the scenario command `level path` (with `seed`): a fresh player on the level's entrance. A number: the
	// campaign level.
	bool Load(const char* path, uint64_t seed = 1) {
		random.Seed(seed);
		if (!dungeon.Load(path))
			return false;
		return started();
	}
	bool Load(int campaignLevel, uint64_t seed = 1) {
		random.Seed(seed);
		if (!dungeon.LoadCampaignLevel(campaignLevel))
			return false;
		return started();
	}

	// The game's player controls on this world (plan player-control-in-sim): input.cpp and game_loop.cpp call the same.
	PlayerControl Control() { return {dungeon, player, items, events, refusalTold}; }

	// One tick: the held keys' step, the world's update, a power hit's charge, the attack's hit, then the player's
	// poison and healing, then every pose (the game loop's order). A shot leaves at about the fists' height (the game
	// asks its drawn fist).
	void Tick() {
		GameClock::advance(UPDATE_TICK_MS);
		dungeon.Update();
		player.UpdateCharge();
		if (player.Alive())
			Control().UpdateAttack(facing, 0.65f * player.Height());
		Control().AfterUpdate();
		for (WorldEvent& e : events.Take())
			said.push_back(std::move(e));
	}

	void Wait(int ms) {
		for (int t = 0; t < ms; t += UPDATE_TICK_MS)
			Tick();
	}
	// Ticks that cover a gate's fade out and in (GATE_TRAVEL_FADE_MS each) after Interact on an exit or a teleporter.
	static constexpr int GATE_TRAVEL_TICKS = 2 * GATE_TRAVEL_FADE_MS / UPDATE_TICK_MS + 3;
	// As `wait N` (ticks).
	void WaitTicks(int ticks) {
		for (int t = 0; t < ticks; t++)
			Tick();
	}

	// One step of the walk key that way (dir -1 / +1), as stepHeldWalk does before the update. True: moved.
	bool Step(int dir) {
		facing = dir;
		return Control().Walk(static_cast<float>(dir) * PLAYER_MOVE_STEP * Control().MoveMultiplier(), 0.f);
	}

	// As `walk to x`: holds the walk key until the player's map x reaches x. False: blocked for 30 ticks.
	bool WalkTo(float x) {
		const int dir = x > X() ? 1 : -1;
		int stuck = 0;
		while ((X() - x) * static_cast<float>(dir) < 0.f) {
			const float from = X();
			Step(dir);
			Tick();
			stuck = X() == from ? stuck + 1 : 0;
			if (stuck >= 30)
				return false;
		}
		return true;
	}

	// As `walk left|right N`.
	bool Walk(float tiles) { return WalkTo(X() + tiles); }

	// As `hold left|right T`: the walk key down for ms, whether the player moves or not; dir -1 / +1.
	void HoldWalk(int dir, int ms) {
		for (int t = 0; t < ms; t += UPDATE_TICK_MS) {
			Step(dir);
			Tick();
		}
	}

	// As `walk up|down N` on a ladder. False: blocked for 30 ticks.
	bool Climb(float tiles) {
		const float to = Y() + tiles;
		const int dir = tiles > 0.f ? 1 : -1;
		int stuck = 0;
		while ((Y() - to) * static_cast<float>(dir) < 0.f) {
			const float from = Y();
			const float step = dir > 0 ? PLAYER_FORWARD_MOVE_STEP : -PLAYER_MOVE_STEP;
			Control().Walk(0.f, step * Control().MoveMultiplier());
			Tick();
			stuck = Y() == from ? stuck + 1 : 0;
			if (stuck >= 30)
				return false;
		}
		return true;
	}

	void Jump() { dungeon.StartJump(facing); }
	void Interact() { dungeon.Interact(); }
	// A closed chest: interact, its opening clip (plan treasure-containers), interact again to take the item.
	void OpenChest() {
		Interact();
		Wait(containerOpenMsMax() + UPDATE_TICK_MS);
		Interact();
	}

	// As `give`: found in a chest (its journal note).
	void Give(ItemKind kind) { items.Find(kind, journal); }
	// As `equip` (given first if not held): the weapon in hand.
	void Equip(ItemKind weapon) {
		if (items.Count(weapon) == 0)
			Give(weapon);
		items.Use(weapon, bagUser(player));
	}
	// The player's actions over this world's bag, journal and events (player_actions.h, as the screens).
	PlayerActor Actor() { return {player.stats, items, journal, events, bagUser(player)}; }
	// As `wear` (given first if not held): the amulet on, its bonus applied.
	void Wear(ItemKind amulet) {
		if (items.Count(amulet) == 0)
			Give(amulet);
		PlayerActor actor = Actor();
		if (items.Worn() != amulet)
			useItem(actor, amulet);
	}
	// As `wear <ring> left|right` (given first if not held): the ring on that hand, the rings' bonus applied.
	void WearRing(ItemKind ring, Hand hand) {
		if (items.Count(ring) == 0)
			Give(ring);
		PlayerActor actor = Actor();
		if (items.RingOn(hand) != ring)
			wearRingOn(actor, ring, hand);
	}

	// A press of the attack key with the equipped weapon (PlayerControl::Attack): false while a swing or its recovery
	// runs, on a ladder, with Palsy and a ranged weapon, or with no shot left. It hits (or shoots) at the weapon's hit
	// time, in a later Tick.
	bool Attack() {
		refusalTold = false;
		return Control().Attack(facing) != PlayerControl::Began::None;
	}

	// As the attack key held down for ms, then let go (PlayerControl::Press, plan skill-power-hit): with the skill, the
	// toggle on and a melee weapon, a power hit's charge, released into the swing; otherwise a plain Attack at the
	// press. True: a swing began.
	bool HoldAttack(int ms) {
		refusalTold = false;
		const PlayerControl::Began began = Control().Press(facing);
		Wait(ms);
		if (began != PlayerControl::Began::Charge)
			return began != PlayerControl::Began::None;
		return Control().ReleaseCharge() != PlayerControl::Began::None;
	}

	// As `savegame` then `loadgame` (SaveService::Write, Read): the level number, the stats, the bag, the dungeon and
	// the journal through a file, and back.
	void SaveAndLoad() {
		const std::string path = "build/sim_world.sav";
		UiViewState view;
		const SaveParts parts{dungeon, player, items, journal, view};
		{
			std::ofstream out(path);
			writeSaveGame(out, parts);
		}
		std::ifstream in(path);
		if (!readSaveGame(in, parts))
			throw std::runtime_error("the save does not read back");
		dungeon.scatterDecorations(campaignLevelFile(dungeon.LevelNumber()).c_str(), dungeon.LevelNumber());
	}

	[[nodiscard]] float X() {
		float x = 0.f, y = 0.f;
		dungeon.getC(x, y);
		return x;
	}
	[[nodiscard]] float Y() {
		float x = 0.f, y = 0.f;
		dungeon.getC(x, y);
		return y;
	}
	[[nodiscard]] int Hp() const { return player.stats.CurrentHP(); }
	[[nodiscard]] int Xp() const { return player.stats.CurrentXP(); }
	[[nodiscard]] int Nearest() const { return dungeon.NearestMonsterHealth(); }

	// The journal's entry of a monster type; null if it was never met.
	[[nodiscard]] const JournalCreature* Creature(int type) const {
		for (const JournalCreature& c : journal.Creatures())
			if (c.type == type)
				return &c;
		return nullptr;
	}
	[[nodiscard]] bool Saw(int type, CreatureMove move) const {
		const JournalCreature* c = Creature(type);
		return c != nullptr && c->Saw(move);
	}
	// Damage types whose effect on a creature is written down, its poison resistance counted as one more, summed over
	// the creatures (the scenario field journal_tried).
	[[nodiscard]] int JournalTried() const {
		int n = 0;
		for (const JournalCreature& c : journal.Creatures())
			for (unsigned bits = c.tried; bits != 0; bits &= bits - 1)
				n++;
		return n;
	}
	// A status line said since the level loaded, containing text.
	[[nodiscard]] bool Told(const std::string& text) const {
		return std::any_of(said.begin(), said.end(), [&](const WorldEvent& e) {
			return e.kind == WorldEvent::Kind::Status && e.text.contains(text);
		});
	}

	static const MonsterTypes& monsterTypes() {
		static MonsterTypes types;
		static const bool loaded = LoadMonsterTypes(types);
		(void)loaded;
		return types;
	}

	static const ModelInfo& playerModel() {
		static ModelInfo info;
		static const bool loaded = LoadClips("characters/archeologist", PLAYER_CLIPS, info);
		(void)loaded;
		return info;
	}

  private:
	bool started() {
		dungeon.ClearWin();
		player.Reanimate();
		said.clear();
		return true;
	}
};

#endif
