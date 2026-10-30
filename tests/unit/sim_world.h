#ifndef SIM_WORLD_H
#define SIM_WORLD_H

#include "../../src/core/gameplay_config.h"
#include "../../src/core/timer.h"
#include "../../src/entities/model_info.h"
#include "../../src/entities/monster.h"
#include "../../src/entities/player.h"
#include "../../src/graphics/render_config.h"
#include "../../src/world/campaign.h"
#include "../../src/world/dungeon.h"
#include "../../src/world/item_bag.h"
#include "../../src/world/items.h"
#include "../../src/world/inscriptions.h"
#include "../../src/world/journal.h"
#include "../../src/world/names.h"
#include "../../src/world/rng.h"
#include "../../src/world/world_events.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

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

	SimWorld() {
		GameClock::enableVirtual();
		player.SetModel(playerModel());
		player.scale = PLAYER_SCALE;
		dungeon.Link({&player, &journal, &items, &random, nullptr, &events, &monsterTypes(), &inscriptions, &names});
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

	// One tick: the held keys' step, the world's update, a power hit's charge, the attack's hit, then the player's
	// poison and healing, then every pose (the game loop's order).
	void Tick() {
		GameClock::advance(UPDATE_TICK_MS);
		dungeon.Update();
		player.UpdateCharge();
		if (player.Alive())
			updateAttack();
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
		return walk(static_cast<float>(dir) * PLAYER_MOVE_STEP * moveMultiplier(), 0.f);
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
			walk(0.f, dir > 0 ? PLAYER_FORWARD_MOVE_STEP * moveMultiplier() : -PLAYER_MOVE_STEP * moveMultiplier());
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
		items.Use(weapon, vitals());
	}
	// As `wear` (given first if not held): the amulet on, its bonus applied.
	void Wear(ItemKind amulet) {
		if (items.Count(amulet) == 0)
			Give(amulet);
		if (items.Worn() != amulet)
			items.Use(amulet, vitals());
		player.stats.Wear(amuletBonus(items.Worn()), true);
	}
	// As `wear <ring> left|right` (given first if not held): the ring on that hand, the rings' bonus applied.
	void WearRing(ItemKind ring, Hand hand) {
		if (items.Count(ring) == 0)
			Give(ring);
		if (items.RingOn(hand) != ring)
			items.WearOn(ring, hand, true);
		player.stats.WearRings(ringBonus(items.Rings()));
	}

	// The attack key with the equipped weapon (input.cpp tryAttack): false while a swing or its recovery runs, or on a
	// ladder. It hits (or shoots) at the weapon's hit time, in a later Tick.
	bool Attack() {
		if (player.attackStartMs >= 0 || !player.attackTimer.TimePassed() || !dungeon.AttackAllowed(true))
			return false;
		player.swingPower = 0;
		swing(false);
		return true;
	}

	// As the attack key held down for ms, then let go (input.cpp, plan skill-power-hit): with the skill, the toggle on
	// and a melee weapon, a power hit's charge, released into the swing; otherwise a plain Attack at the press. True: a
	// swing began.
	bool HoldAttack(int ms) {
		if (!player.CanCharge(isRanged(items.Equipped())) || player.attackStartMs >= 0 ||
			!player.attackTimer.TimePassed(true) || !player.StartCharge()) {
			const bool began = Attack();
			Wait(ms);
			return began;
		}
		Wait(ms);
		const bool charged = player.ReleaseCharge() > 0;
		if (!dungeon.AttackAllowed(true))
			return false;
		swing(charged);
		return true;
	}

	// As `savegame` then `loadgame` (GameState::Save, LoadSave): the level number, the stats, the bag, the dungeon and
	// the journal through a file, and back.
	void SaveAndLoad() {
		const std::string path = "build/sim_world.sav";
		{
			std::ofstream out(path);
			out << dungeon.LevelNumber() << " ";
			player.stats.Dump(out);
			items.Save(out);
			dungeon.Dump(out);
			journal.Dump(out);
		}
		player.Reanimate();
		std::ifstream in(path);
		int level = 1;
		in >> level;
		dungeon.SetLevelNumber(level);
		player.stats.LoadDump(in);
		items.Load(in);
		player.stats.Wear(amuletBonus(items.Worn()), false);
		player.stats.WearRings(ringBonus(items.Rings()));
		dungeon.LoadDump(in);
		journal.Load(in);
		dungeon.scatterDecorations(campaignLevelFile(level).c_str(), level);
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
			return e.kind == WorldEvent::Kind::Status && e.text.find(text) != std::string::npos;
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

	// input.cpp: no sprint while wading, except with the Favour of Hapi skill or the Blessing of Hapi potion.
	[[nodiscard]] float moveMultiplier() const {
		return dungeon.PlayerSprintMultiplier(player.stats.SprintMoveMultiplier());
	}

	bool walk(float dx, float dy) {
		const bool sprints = dungeon.PlayerCanSprint();
		const float factor =
			dungeon.PlayerWalkFactor() * player.stats.WalkFactor(player.jump.jumping || player.jump.falling);
		const bool moved = dungeon.Move(dx * factor, dy * factor);
		if (moved && sprints)
			player.stats.NoteWalked(dungeon.PlayerHalvedWadeDrain());
		return moved;
	}

	[[nodiscard]] Vitals vitals() const {
		return {player.Alive(),
				player.stats.CurrentHP(),
				player.stats.CurrentMaxHP(),
				player.stats.Stamina(),
				player.stats.MaxStamina(),
				player.stats.poison.Any(),
				player.stats.PotionResistPercent()};
	}

	// input.cpp: the swing begins; a charged one at its windup.
	void swing(bool charged) {
		const WeaponMotion& motion = weaponDef(items.Equipped()).motion;
		player.attackTimer.Reset();
		player.attackTimer.SetInterval(motion.AttackMs());
		player.attackStartMs = GameClock::now() - (charged ? windupMs(motion) : 0);
		player.attackLanded = false;
	}

	// game_loop.cpp updateAttack: the hit at the weapon's hit time; the hand height of a shot is about the fists'.
	void updateAttack() {
		if (player.attackStartMs < 0)
			return;
		const ItemKind kind = items.Equipped();
		const WeaponDef& def = weaponDef(kind);
		const int t = GameClock::now() - player.attackStartMs;
		if (!player.attackLanded && t >= def.motion.hitMs) {
			player.attackLanded = true;
			const int damage = player.stats.Damage(weaponDamage(kind, def.damage, items.Level(kind)));
			if (isRanged(kind))
				dungeon.Shoot(
					missileOf(kind),
					damage,
					def.mix,
					facing,
					0.65f * player.Height(),
					player.stats.WeaponReach(kind)
				);
			else
				dungeon.AttackNearest(
					player.PowerDamage(damage),
					def.mix,
					player.stats.WeaponReach(kind),
					facing,
					player.SwingShare()
				);
		}
		if (t >= def.motion.swingMs)
			player.attackStartMs = -1;
	}
};

#endif
