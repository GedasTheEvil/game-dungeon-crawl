#ifndef SIM_WORLD_H
#define SIM_WORLD_H

#include "../../src/core/gameplay_config.h"
#include "../../src/core/timer.h"
#include "../../src/entities/model_info.h"
#include "../../src/entities/monster.h"
#include "../../src/entities/player.h"
#include "../../src/world/dungeon.h"
#include "../../src/world/item_bag.h"
#include "../../src/world/journal.h"
#include "../../src/world/rng.h"
#include "../../src/world/world_events.h"
#include <cstdint>
#include <utility>
#include <vector>

// The game's world without the game (docs/plan/solved/sim-library.md): a level, its monsters and the player, stepped on the
// virtual clock one UPDATE_TICK_MS tick at a time as the game loop does (game_loop.cpp), without input, the camera
// or the drawing. The monster types and the player's measures come from the real model files.
class SimWorld {
  public:
	Player player;
	Journal journal;
	ItemBag items;
	GameRandom random;
	WorldEvents events;
	std::vector<WorldEvent> said; // every event since the level loaded, oldest first
	Dungeon dungeon;

	SimWorld() {
		GameClock::enableVirtual();
		player.SetModel(playerModel());
		player.scale = PLAYER_SCALE;
		dungeon.Link({&player, &journal, &items, &random, nullptr, &events, &monsterTypes()});
	}

	// As the scenario command `level path` (with `seed`): a fresh player on the level's entrance.
	bool Load(const char* path, uint64_t seed = 1) {
		random.Seed(seed);
		if (!dungeon.Load(path))
			return false;
		dungeon.ClearWin();
		player.Reanimate();
		said.clear();
		return true;
	}

	// One tick: the world's update, then the player's poison and healing, then every pose (the game loop's order).
	void Tick() {
		GameClock::advance(UPDATE_TICK_MS);
		dungeon.Update();
		player.stats.UpdateStamina(events);
		player.UpdatePoison(events);
		player.stats.Regenerate(dungeon.PlayerSafe(), UPDATE_TICK_MS);
		dungeon.AnimateMonsters();
		player.Animate();
		for (WorldEvent& e : events.Take())
			said.push_back(std::move(e));
	}

	void Wait(int ms) {
		for (int t = 0; t < ms; t += UPDATE_TICK_MS)
			Tick();
	}

	// As the scenario command `walk to x`: holds the walk key, one step a tick before the update (stepHeldWalk), until
	// the player's map x reaches x. False: blocked for 30 ticks.
	bool WalkTo(float x) {
		const float dir = x > X() ? 1.f : -1.f;
		int stuck = 0;
		while ((X() - x) * dir < 0.f) {
			const float from = X();
			if (dungeon.Move(dir * PLAYER_MOVE_STEP * dungeon.PlayerWalkFactor(), 0) && !dungeon.PlayerWading())
				player.stats.NoteWalked();
			Tick();
			stuck = X() == from ? stuck + 1 : 0;
			if (stuck >= 30)
				return false;
		}
		return true;
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

	[[nodiscard]] int Hp() const { return player.stats.CurrentHP(); }

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
};

#endif
