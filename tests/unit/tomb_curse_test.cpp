// Tomb-curse containers (plan tomb-curse-containers): the coffin, a container from the tombs on whose lid may carry an
// execration text (rolled with the look, readable up close): opening such a coffin curses unless the trap ward stops
// it, and it holds a trapped chest's loot. The mimic jar: a sealed pottery jar that bites, leaves its smashed jar.
#include "../../external/doctest/doctest.h"
#include "../../src/level/campaign.h"
#include "../../src/level/containers.h"
#include "../../src/level/decor_scatter.h"
#include "../../src/level/level_gen.h"
#include "../../src/level/loot.h"
#include "sim_world.h"
#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>

namespace {
constexpr int ROW = 2; // tests/levels/tomb_curse: the corridor's row
constexpr int COFFIN_COL = 6;
constexpr int JAR_COL = 10;
constexpr int TRAPPED_COL = 12;
constexpr int DEEP_LEVEL = 16; // the temple tier: coffins among the containers

bool noted(const SimWorld& world, FieldNote note) {
	return std::any_of(world.said.begin(), world.said.end(), [note](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Note && e.note == note;
	});
}

const Monster* mimicJar(const SimWorld& world) {
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active() && world.dungeon.Monsters()[i].Type()->id == MonsterMimicJar)
			return &world.dungeon.Monsters()[i];
	return nullptr;
}

// The level on seed at DEEP_LEVEL, the treasure in col a coffin (with the text or not); the player beside the mimic
// jar walks up to it.
void atCoffin(SimWorld& world, uint64_t seed, bool inscribed, int col = COFFIN_COL) {
	REQUIRE(world.Load("tests/levels/tomb_curse", seed));
	world.dungeon.SetLevelNumber(DEEP_LEVEL);
	REQUIRE(world.dungeon.PlaceContainer(col, ROW, static_cast<int>(Container::Coffin), 0, inscribed));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(static_cast<float>(col) + 0.5f));
	world.said.clear();
}
} // namespace

TEST_CASE("the coffin: a container of the tombs, lying lengthwise; its trap is a curse; the jars stay for the mimic") {
	const ContainerDef& coffin = containerDef(Container::Coffin);
	CHECK(coffin.lengthwise);
	CHECK(std::string(CONTAINER_NAMES[static_cast<int>(Container::Coffin)]) == "coffin");
	for (int tier = 0; tier < DECOR_TIER_COUNT; tier++) {
		CAPTURE(tier);
		const int share = CONTAINER_PERCENT[tier][static_cast<int>(Container::Coffin)];
		CHECK((tier < decorTier(DECOR_TIER_FROM[2]) ? share == 0 : share > 0)); // from the tombs on
		CHECK(CONTAINER_PERCENT[tier][static_cast<int>(Container::Jar)] >= 10); // the mimic jar hides among them
	}
	for (const ContainerDef& def : CONTAINER_DEFS)
		CHECK((def.lengthwise == (&def == &coffin)));
	Rng rng(5);
	for (int i = 0; i < 30; i++) {
		CHECK(RollChestTrap(Container::Coffin, DEEP_LEVEL, rng) == ChestTrap::Curse);
		const Curse c = RollChestCurse(Container::Coffin, rng);
		CHECK((c != Curse::Palsy && c != Curse::Insanity && c != Curse::Disease));
	}
}

TEST_CASE("the campaign's coffins never stand side-on; only coffins carry a text, about the share of them") {
	int coffins = 0, inscribed = 0;
	for (int n = 1; n <= CAMPAIGN_LEVELS; n++) {
		CAPTURE(n);
		LevelGrid grid;
		const std::string file = campaignLevelFile(n);
		REQUIRE(loadLevelFile(file.c_str(), grid).empty());
		auto layout = std::make_unique<DecorLayout>();
		scatterDecor(grid.cells, file.c_str(), n, grid.meta.zone, *layout);
		for (int c = 0; c < LEVEL_CELL_COUNT; c++) {
			if (isWall(grid.cells[c]))
				continue;
			const TreasureLook& look = layout->treasure[c];
			if (look.kind != Container::Coffin) {
				CHECK_FALSE(look.inscribed);
				continue;
			}
			CHECK(std::abs(look.yaw) < 90);
			coffins++;
			inscribed += look.inscribed ? 1 : 0;
		}
	}
	REQUIRE(coffins > 200);
	CHECK(inscribed > coffins * (COFFIN_INSCRIBED_PERCENT - 8) / 100);
	CHECK(inscribed < coffins * (COFFIN_INSCRIBED_PERCENT + 8) / 100);
}

TEST_CASE("a coffin without a text opens as any chest: no curse, its note") {
	SimWorld world;
	atCoffin(world, 1, false);
	CHECK(world.dungeon.InteractVerb() == Interaction::Open);
	CHECK(world.dungeon.CoffinTellAt(COFFIN_COL, ROW) == 0.f);
	world.Interact();
	world.WaitTicks(1);
	CHECK(noted(world, FieldNote::Coffins));
	CHECK_FALSE(world.player.stats.curses.Any());
	CHECK_FALSE(world.Told("execration"));
	world.Wait(containerOpenMsMax() + UPDATE_TICK_MS);
	CHECK(world.dungeon.InteractVerb() == Interaction::Take);
}

TEST_CASE("a coffin with the text curses as it opens and holds one bonus more; the grand trap ward stops the curse") {
	for (uint64_t seed = 1; seed <= 10; seed++) {
		CAPTURE(seed);
		SimWorld world, warded;
		atCoffin(world, seed, true);
		atCoffin(warded, seed, true);
		warded.Wear(ItemKind::TrapWardGrand);
		warded.said.clear();
		CHECK(world.dungeon.InteractVerb() == Interaction::Open); // the prompt never tells
		world.Interact();
		warded.Interact();
		world.WaitTicks(1);
		warded.WaitTicks(1);
		CHECK(world.player.stats.curses.Any());
		CHECK(world.Told("execration"));
		CHECK(noted(world, FieldNote::TombCurses));
		CHECK(noted(world, FieldNote::Coffins));
		CHECK_FALSE(warded.player.stats.curses.Any());
		CHECK(warded.Told("Eye of Horus"));
		for (SimWorld* w : {&world, &warded}) {
			w->Wait(containerOpenMsMax() + UPDATE_TICK_MS);
			REQUIRE(w->dungeon.InteractVerb() == Interaction::Take);
			w->Interact();
			w->WaitTicks(1);
			CHECK(w->Told("\n+ ")); // a trapped chest's loot, ward or not
		}
	}
}

TEST_CASE("the same seed opens a cursed coffin the same way") {
	SimWorld a, b;
	atCoffin(a, 4, true);
	atCoffin(b, 4, true);
	a.Interact();
	b.Interact();
	a.WaitTicks(1);
	b.WaitTicks(1);
	for (int c = 0; c < CURSE_COUNT; c++)
		CHECK(a.player.stats.curses.Has(static_cast<Curse>(c)) == b.player.stats.curses.Has(static_cast<Curse>(c)));
}

TEST_CASE("the text shows only up close, on the cursed coffins: an inscribed one and a trapped one") {
	SimWorld world;
	atCoffin(world, 1, true);
	CHECK(world.dungeon.CoffinTellAt(COFFIN_COL, ROW) == doctest::Approx(1.f));
	REQUIRE(world.dungeon.PlaceContainer(TRAPPED_COL, ROW, static_cast<int>(Container::Coffin), 0));
	CHECK(world.dungeon.CoffinTellAt(TRAPPED_COL, ROW) == 0.f); // six cells away
	REQUIRE(world.dungeon.PlaceContainer(4, ROW, static_cast<int>(Container::Coffin), 0));
	CHECK(world.dungeon.CoffinTellAt(4, ROW) == 0.f); // no text
	REQUIRE(world.WalkTo(static_cast<float>(TRAPPED_COL) - 1.f)); // past the mimic jar, it lies in wait
	CHECK(world.dungeon.CoffinTellAt(COFFIN_COL, ROW) == 0.f);
	CHECK(world.dungeon.CoffinTellAt(TRAPPED_COL, ROW) > 0.f);
}

TEST_CASE("a trapped coffin's trap is its curse, one roll: no second curse on top") {
	for (uint64_t seed = 1; seed <= 10; seed++) {
		CAPTURE(seed);
		SimWorld world;
		REQUIRE(world.Load("tests/levels/tomb_curse", seed));
		world.dungeon.SetLevelNumber(DEEP_LEVEL);
		world.player.god = true; // past the mimic jar
		REQUIRE(world.dungeon.PlaceContainer(TRAPPED_COL, ROW, static_cast<int>(Container::Coffin), 0, true));
		world.WaitTicks(30);
		REQUIRE(world.WalkTo(static_cast<float>(TRAPPED_COL) + 0.5f));
		world.said.clear();
		world.Interact();
		world.WaitTicks(1);
		CHECK(world.player.stats.curses.Any());
		CHECK(noted(world, FieldNote::TrappedChests));
		CHECK(std::count_if(world.said.begin(), world.said.end(), [](const WorldEvent& e) {
				  return e.kind == WorldEvent::Kind::Status && e.text.find("execration") != std::string::npos;
			  }) == 1);
	}
}

TEST_CASE("the mimic jar passes for the jar container, a mimic of the tombs") {
	const MonsterKind& jar = *monsterKind(MonsterMimicJar);
	const MonsterKind& mimic = *monsterKind(MonsterMimic);
	CHECK(jar.glyph == '}');
	CHECK(jar.locomotion == Locomotion::Ambush);
	CHECK(jar.disguise == Container::Jar);
	CHECK(mimic.disguise == Container::Chest);
	CHECK(jar.scale == doctest::Approx(containerDef(Container::Jar).scale));
	CHECK(jar.threat < mimic.threat);
	CHECK(jar.damage < monsterKind(MonsterCursedMimic)->damage);
	for (int z = 0; z < ZONE_ID_COUNT; z++) { // levelgen's mimic jars: only in the tomb zones
		const auto id = static_cast<ZoneId>(z);
		CHECK((zoneFauna(id).jarPercent > 0) == (id == ZoneId::PriestsQuarter || id == ZoneId::Necropolis));
	}
}

TEST_CASE("the mimic jar lies in wait however near; opened, it wakes; killed, it leaves its smashed jar") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/tomb_curse"));
	world.player.god = true;
	world.WaitTicks(2);
	const Monster* m = mimicJar(world);
	REQUIRE(m != nullptr);
	REQUIRE(world.WalkTo(static_cast<float>(JAR_COL) + 0.5f));
	world.Wait(2000);
	CHECK(m->lurking());
	CHECK(world.dungeon.InteractVerb() == Interaction::Open); // as a jar
	world.Interact();
	world.WaitTicks(2);
	CHECK_FALSE(m->lurking());
	const int chests = world.dungeon.ChestCount();
	for (int dir : {1, -1}) // the player stands on its cell
		if (m->Alive())
			world.dungeon.AttackNearest(5000, DamageMix{100, 0, 0}, 2.f, dir);
	REQUIRE_FALSE(m->Alive());
	world.Wait(5000); // its die clip
	CHECK_FALSE(mimicJar(world));
	CHECK(world.dungeon.ChestCount() == chests + 1);
	const Tile left = world.dungeon.Cell(JAR_COL, ROW);
	REQUIRE(left.type == Treasure);
	const std::optional<ItemKind> item = itemFromFile(left.attr, left.value);
	REQUIRE(item.has_value());
	CHECK(isPotion(*item));
	CHECK(world.dungeon.LookAt(JAR_COL, ROW).kind == Container::Jar);
	world.SaveAndLoad();
	CHECK(world.dungeon.LookAt(JAR_COL, ROW).kind == Container::Jar);
	CHECK(world.dungeon.Cell(JAR_COL, ROW).type == Treasure);
}

TEST_CASE("a killed mimic jar's loot is a potion the mimic can leave") {
	Rng rng{9};
	for (int i = 0; i < 200; i++) {
		const ItemKind loot = RollMimicJarLoot(rng);
		CHECK(isPotion(loot));
		CHECK(potionDef(loot).mimicLoot);
	}
}

TEST_CASE("levelgen makes some of its mimics jars from the tombs' difficulty on, none early") {
	auto count = [](int difficulty) {
		int n = 0;
		for (uint32_t seed = 1; seed <= 30; seed++) {
			GenOptions opt;
			opt.seed = seed;
			opt.difficulty = difficulty;
			const GenResult gen = generateLevel(opt);
			for (const Tile& t : gen.grid.cells)
				n += t.type == MonsterSpawn && t.attr == MonsterMimicJar ? 1 : 0;
		}
		return n;
	};
	CHECK(count(4) == 0);
	CHECK(count(7) > 0);
}
