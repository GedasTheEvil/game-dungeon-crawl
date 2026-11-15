// Plan bleeding: cut damage's own condition in three tiers, the same for the player and the monsters.
#include "../../external/doctest/doctest.h"
#include "../../src/level/bleeding.h"
#include "../../src/level/level_check.h"
#include "../../src/level/monster_kinds.h"
#include "../../src/level/player_stats.h"
#include "sim_world.h"
#include <sstream>
#include <tuple>
#include <utility>

namespace {
int bleedTotal(BleedTier tier, int maxHp, bool water) {
	Bleeding b;
	b.Apply(tier);
	int hp = 0;
	for (int t = 0; t < 60000; t += UPDATE_TICK_MS)
		hp += b.Advance(UPDATE_TICK_MS, maxHp, water);
	return hp;
}
} // namespace

TEST_CASE("bleeding: each tier's total, a percent of max HP above a floor; in water it lasts twice as long") {
	for (auto [tier, maxHp, total] :
		 {std::tuple{BleedTier::Weak, 50, 10},
		  {BleedTier::Medium, 50, 40},
		  {BleedTier::Medium, 500, 100},
		  {BleedTier::Strong, 50, 36},
		  {BleedTier::Strong, 500, 180}}) {
		CHECK(bleedTotal(tier, maxHp, false) == total);
		CHECK(bleedTotal(tier, maxHp, true) == 2 * total);
	}
}

TEST_CASE("bleeding: the tiers run side by side, a tier again restarts its time") {
	Bleeding b;
	b.Apply(BleedTier::Weak);
	b.Apply(BleedTier::Strong);
	CHECK(b.Mask() == 5);
	b.Advance(4000, 50, false);
	b.Apply(BleedTier::Weak);
	CHECK(b.LeftMs(BleedTier::Weak) == BLEED_TIERS[0].durationMs);
	b.Stop();
	CHECK_FALSE(b.Any());
}

TEST_CASE("bleeding: the chance by the cut rate and the trap share, at most 100") {
	Resistances resist = NO_RESISTANCES;
	const auto cut = std::to_underlying(DamageType::Slash);
	CHECK(bleedChancePercent(40, resist) == 40);
	resist[cut] = RESISTS;
	CHECK(bleedChancePercent(40, resist) == 20);
	resist[cut] = IMMUNE;
	CHECK(bleedChancePercent(40, resist) == 0);
	resist[cut] = VULNERABLE;
	CHECK(bleedChancePercent(40, resist) == 80);
	CHECK(bleedChancePercent(70, resist) == 100);
	resist[cut] = NORMAL;
	CHECK(bleedChancePercent(40, resist, FULL_TRAP_SHARE / 2) == 20); // a half trap ward
	CHECK(bleedChancePercent(40, resist, 0) == 0); // a trap share of 0: no bleed
	resist[std::to_underlying(DamageType::Pierce)] = IMMUNE; // only the cut rate counts, also for the spikes' stab
	CHECK(bleedChancePercent(40, resist) == 40);
}

TEST_CASE("bleeding: a bite's chance is its percent times its cut share; no cut, no bleed") {
	CHECK(biteBleedPercent(BLEED_BITE_PERCENT, {0, 30, 70}) == 9); // a rat
	CHECK(biteBleedPercent(BLEED_BITE_PERCENT, {100, 0, 0}) == 0); // a mummy's fists
	CHECK(isBleeder(MonsterCamelSpider));
	CHECK_FALSE(isBleeder(MonsterMummy));
	CHECK_FALSE(isBleeder(MonsterCobra)); // fangs only stab
	CHECK_FALSE(isBleeder(MonsterEggCluster)); // never bites
}

TEST_CASE("bleeding: a boss whose kin cuts bleeds the player harder (AGENTS.md, Boss abilities)") {
	for (int id = 1; id <= MONSTER_TYPE_MAX; id++) {
		const MonsterKind& boss = *monsterKind(id);
		if (!boss.isBoss() || !isBleeder(boss.kin))
			continue;
		CAPTURE(boss.name);
		const MonsterKind& kin = *monsterKind(boss.kin);
		CHECK(isBleeder(id));
		CHECK(boss.bleed.tier >= kin.bleed.tier);
		CHECK(boss.bleed.percent >= kin.bleed.percent);
		CHECK((boss.bleed.tier > kin.bleed.tier || boss.bleed.percent > kin.bleed.percent));
		CHECK(boss.bloodless == kin.bloodless);
	}
}

TEST_CASE("bleeding: any heal stops it, the regeneration gives no HP while it runs") {
	PlayerStats stats;
	stats.LoseHP(20);
	stats.vitals.bleeding.Apply(BleedTier::Weak);
	stats.Heal(10);
	CHECK_FALSE(stats.vitals.bleeding.Any());
	stats.vitals.bleeding.Apply(BleedTier::Medium);
	stats.HealFully(); // Amun, a new life
	CHECK_FALSE(stats.vitals.bleeding.Any());
	stats.vitals.bleeding.Apply(BleedTier::Medium);
	stats.AddMaxHP(10); // the Elixir of Life, a Life boon
	CHECK_FALSE(stats.vitals.bleeding.Any());
	WorldEvents events;
	stats.vitals.bleeding.Apply(BleedTier::Strong);
	stats.AddXP(static_cast<int>(PlayerStats::LevelXP(stats.CurrentLevel() + 1)), events); // a level up
	CHECK_FALSE(stats.vitals.bleeding.Any());

	stats.LoseHP(20);
	const int hp = stats.CurrentHP();
	stats.WearRings(ringBonus(WornRings{ItemKind::GarnetGold, ItemKind::GarnetGold}));
	stats.vitals.bleeding.Apply(BleedTier::Weak);
	for (int ms = 0; ms < 6000; ms += UPDATE_TICK_MS)
		stats.Regenerate(true, UPDATE_TICK_MS);
	CHECK(stats.CurrentHP() == hp);
	stats.vitals.bleeding.Stop();
	for (int ms = 0; ms < 3000; ms += UPDATE_TICK_MS)
		stats.Regenerate(true, UPDATE_TICK_MS);
	CHECK(stats.CurrentHP() > hp);
}

TEST_CASE("bleeding: the player's running tiers are saved") {
	PlayerStats stats;
	stats.vitals.bleeding.Apply(BleedTier::Medium);
	stats.vitals.bleeding.Advance(3500, stats.CurrentMaxHP(), false);
	std::stringstream out;
	stats.Dump(out);
	PlayerStats loaded;
	std::stringstream in(out.str());
	loaded.LoadDump(in);
	CHECK(loaded.vitals.bleeding.Mask() == 2);
	CHECK(loaded.vitals.bleeding.LeftMs(BleedTier::Medium) == stats.vitals.bleeding.LeftMs(BleedTier::Medium));
	CHECK(loaded.Stamina() == stats.Stamina()); // the lines after it still read
}

TEST_CASE("bleeding: the player bleeds, the cut guard lowers the chance, the drain is not armour's") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/empty_row"));
	Player& p = world.player;
	CHECK(p.Bleed(BleedTier::Weak, 100, FULL_TRAP_SHARE, world.events, world.random.gameplay));
	CHECK(p.stats.vitals.bleeding.Mask() == 1);
	p.stats.AddArmor(1000);
	const int hp = p.stats.CurrentHP();
	for (int ms = 0; ms < 2000; ms += UPDATE_TICK_MS)
		p.UpdateBleeding(false, world.events);
	CHECK(p.stats.CurrentHP() == hp - 2);
	p.stats.vitals.bleeding.Stop();
	world.Give(ItemKind::SlashWardGrand);
	world.Wear(ItemKind::SlashWardGrand);
	REQUIRE(p.stats.resist[std::to_underlying(DamageType::Slash)] < NORMAL);
	int took = 0;
	for (int i = 0; i < 200; i++) {
		took += p.Bleed(BleedTier::Weak, 50, FULL_TRAP_SHARE, world.events, world.random.gameplay) ? 1 : 0;
		p.stats.vitals.bleeding.Stop();
	}
	CHECK(took < 85); // 50% times the ward's cut rate
	CHECK(p.Bleed(BleedTier::Weak, 100, 0, world.events, world.random.gameplay) == false); // a full trap ward
}

TEST_CASE("bleeding: bloodless kinds never bleed; a bleed kill is the player's only from their cut") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/empty_row"));
	const MonsterLinks links{&world.player, &world.journal, &world.events, 1, &world.random.gameplay};
	Monster mummy;
	mummy.Spawn(SimWorld::monsterTypes()[std::to_underlying(MonsterMummy)], 3, 1, links, world.random.effects);
	CHECK_FALSE(mummy.TakeBleed(BleedTier::Strong, 100, true, world.random.gameplay));
	CHECK_FALSE(mummy.Bleeds());

	auto bleedOut = [&](bool byPlayer) {
		Monster rat;
		rat.Spawn(SimWorld::monsterTypes()[std::to_underlying(MonsterRat)], 3, 1, links, world.random.effects);
		REQUIRE(rat.TakeBleed(BleedTier::Strong, 100, byPlayer, world.random.gameplay));
		bool rewarded = false;
		for (int i = 0; i < 20 && rat.Alive(); i++) { // a strong bleed again each second: it bleeds out
			rat.TakeBleed(BleedTier::Strong, 100, byPlayer, world.random.gameplay);
			for (int ms = 0; ms < 1000; ms += UPDATE_TICK_MS)
				rewarded = rat.UpdateBleeding() || rewarded;
		}
		CHECK_FALSE(rat.Alive());
		return rewarded;
	};
	CHECK(bleedOut(true));
	CHECK_FALSE(bleedOut(false));
}

TEST_CASE("bleeding: the checker wants a health potion in reach where something makes the player bleed") {
	auto warns = [](int potion, Tile danger) {
		LevelGrid grid;
		for (int col = 1; col < 12; col++)
			grid.set(col, 1, Tile{});
		grid.set(1, 1, Tile{Door, GateEntrance, 0});
		grid.set(11, 1, Tile{Door, GateExit, 0});
		if (potion >= 0)
			grid.set(10, 1, Tile{Treasure, ItemType::POTION, potion});
		grid.set(5, 1, danger);
		for (const std::string& w : checkLevel(grid).warnings)
			if (w.contains("no health potion"))
				return true;
		return false;
	};
	const Tile rat{MonsterSpawn, MonsterRat, 0};
	CHECK(warns(-1, rat));
	CHECK(warns(5, rat)); // a stamina potion does not stop it
	CHECK_FALSE(warns(0, rat)); // small health
	CHECK_FALSE(warns(1, rat)); // large health
	CHECK_FALSE(warns(4, rat)); // the elixir of life
	CHECK_FALSE(warns(-1, Tile{MonsterSpawn, MonsterCobra, 0})); // its fangs only stab
}
