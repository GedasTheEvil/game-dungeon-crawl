// Terrain affinity (plan terrain-affinity): in a native terrain a monster walks faster, bites more often and harder;
// in a hostile one slower, less often and softer, and it keeps out of it when it can.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/level/level_check.h"
#include "../../src/level/terrain.h"
#include <cmath>

namespace {
const TerrainRow& row(Terrain t) { return TERRAIN_ROWS[static_cast<int>(t)]; }

const Monster* find(const SimWorld& world, int type) {
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active() && world.dungeon.Monsters()[i].Type()->id == type)
			return &world.dungeon.Monsters()[i];
	return nullptr;
}

int cellOf(float x) { return static_cast<int>(std::floor(x)); }

// The walk step, bite interval and damage of base 100 it has now, against its kind's.
void checkFactors(const Monster& m, float speed, float interval, float damage) {
	const MonsterKind& kind = *m.Type();
	CHECK(m.WalkStep() == doctest::Approx(MONSTER_SEEK_STEP * kind.speed * speed));
	CHECK(m.AttackMs() == static_cast<int>(std::lround(static_cast<float>(kind.attackMs) * interval)));
	CHECK(m.HitDamage(100) == static_cast<int>(std::lround(100.f * damage)));
}
} // namespace

TEST_CASE("the factors: one terrain, the strongest of several, native and hostile cancel, none") {
	const Terrains water{Terrain::Water};
	const TerrainFactors native = terrainFactors(water, water, {});
	CHECK(native.speed == row(Terrain::Water).native.speed);
	CHECK(native.interval == row(Terrain::Water).native.interval);
	CHECK(native.damage == row(Terrain::Water).native.damage);
	const TerrainFactors hostile = terrainFactors(water, {}, water);
	CHECK(hostile.speed == row(Terrain::Water).hostile.speed);
	CHECK(hostile.damage == row(Terrain::Water).hostile.damage);
	CHECK(terrainFactors(Terrains{Terrain::Sand}, water, {}).speed == 1.f); // not its terrain

	// The spiders: native to the dark and webs. In a dark web, the strongest bonus per factor, no stacking.
	const Terrains spiderNative{Terrain::Darkness, Terrain::Web};
	const Terrains spiderHostile{Terrain::Light, Terrain::Fire};
	const TerrainFactors both = terrainFactors(Terrains{Terrain::Darkness, Terrain::Web}, spiderNative, spiderHostile);
	CHECK(both.speed == std::max(row(Terrain::Darkness).native.speed, row(Terrain::Web).native.speed));
	CHECK(both.interval == std::min(row(Terrain::Darkness).native.interval, row(Terrain::Web).native.interval));
	CHECK(both.damage == std::max(row(Terrain::Darkness).native.damage, row(Terrain::Web).native.damage));
	// A brazier's cell is light and fire: the strongest penalty.
	const TerrainFactors burnt = terrainFactors(Terrains{Terrain::Light, Terrain::Fire}, spiderNative, spiderHostile);
	CHECK(burnt.speed == std::min(row(Terrain::Light).hostile.speed, row(Terrain::Fire).hostile.speed));
	CHECK(burnt.interval == std::max(row(Terrain::Light).hostile.interval, row(Terrain::Fire).hostile.interval));
	// A web in a lit cell: native and hostile at once cancel out, the cell is neutral.
	const Terrains litWeb{Terrain::Web, Terrain::Light};
	CHECK(terrainMood(litWeb, spiderNative, spiderHostile) == TerrainMood::Neutral);
	const TerrainFactors none = terrainFactors(litWeb, spiderNative, spiderHostile);
	CHECK(none.speed == 1.f);
	CHECK(none.interval == 1.f);
	CHECK(none.damage == 1.f);
}

TEST_CASE("a slowed wader hostile to water takes the lower of the wade and the hostile speed, not their product") {
	CHECK(
		wadeWithTerrain(WADE_SPEED_FACTOR, HOSTILE_TERRAIN.speed) ==
		doctest::Approx(std::min(WADE_SPEED_FACTOR, HOSTILE_TERRAIN.speed))
	);
	CHECK(wadeWithTerrain(WADE_SPEED_FACTOR, 1.f) == doctest::Approx(WADE_SPEED_FACTOR));
	CHECK(wadeWithTerrain(WADE_SPEED_FACTOR, 1.5f) == doctest::Approx(WADE_SPEED_FACTOR * 1.5f)); // a bonus stacks
}

TEST_CASE("every boss has its kin's terrains; the swimmers are native to water; no kind native and hostile to one") {
	for (int type = 1; type <= MONSTER_TYPE_MAX; type++) {
		const MonsterKind& kind = *monsterKind(type);
		CAPTURE(kind.label);
		CHECK(validAffinity(kind.native, kind.hostile));
		CHECK((kind.wading == Wading::Swimmer) == kind.native.has(Terrain::Water));
		if (!kind.isBoss())
			continue;
		const MonsterKind& kin = *monsterKind(kind.kin);
		CHECK((kin.native & kind.native) == kin.native);
		CHECK((kin.hostile & kind.hostile) == kin.hostile);
	}
	CHECK(monsterKind(MonsterRerek)->native.has(Terrain::Sand));
	CHECK(monsterKind(MonsterSeth)->hostile.has(Terrain::Sand));
	CHECK(monsterKind(MonsterMotherOfWebs)->hostile.has(Terrain::Light));
}

TEST_CASE("a torch lights its cell but is no fire; a brazier is both; a fire vent is fire while its jet burns") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/camel_spider"));
	const auto r = static_cast<int>(std::floor(world.Y()));
	for (int col = 0; col < LEVEL_WIDTH; col++)
		world.dungeon.PlaceTorch(col, r, false);
	CHECK(world.dungeon.TerrainsAt(10, r) == Terrains{Terrain::Darkness});
	world.dungeon.PlaceTorch(10, r, true);
	CHECK(world.dungeon.TerrainsAt(10, r) == Terrains{Terrain::Light});
	CHECK(world.dungeon.TerrainsAt(11, r).has(Terrain::Light)); // its light reaches the next cell
	REQUIRE(world.dungeon.PlaceDecor(30, r, DECOR_BRAZIER));
	CHECK(world.dungeon.TerrainsAt(30, r) == (Terrains{Terrain::Light, Terrain::Fire}));
	CHECK(world.dungeon.TerrainsAt(31, r) == Terrains{Terrain::Light}); // its light, not its flame
	REQUIRE(world.dungeon.PlaceWeb(20, r));
	CHECK(world.dungeon.TerrainsAt(20, r) == (Terrains{Terrain::Darkness, Terrain::Web}));
	CHECK(world.dungeon.TerrainsAt(20, r, false) == Terrains{Terrain::Darkness}); // a flyer is over the web
}

TEST_CASE(
	"in its native sand the scorpion is fast and hits hard, quicksand too; on dry floor as usual; in the "
	"water it is slow and soft and still follows the player"
) {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/terrain"));
	const int r = static_cast<int>(std::floor(world.Y()));
	CHECK(world.dungeon.TerrainsAt(21, r).has(Terrain::Sand));
	CHECK(world.dungeon.TerrainsAt(24, r).has(Terrain::Sand)); // quicksand is sand
	world.WaitTicks(3);
	const Monster* s = find(world, MonsterScorpion);
	REQUIRE(s != nullptr);
	REQUIRE(cellOf(s->CentreX()) == 23);
	CHECK(s->Mood() == TerrainMood::Native);
	checkFactors(
		*s,
		row(Terrain::Sand).native.speed,
		row(Terrain::Sand).native.interval,
		row(Terrain::Sand).native.damage
	);

	// The player walks west: it follows out of the sand onto the dry floor, then through the water.
	bool dry = false, wet = false;
	for (int t = 0; t < 60000 && cellOf(s->CentreX()) >= 9; t += UPDATE_TICK_MS) {
		if (world.X() > 3.f)
			world.Step(-1);
		const int col = cellOf(s->CentreX()); // its terrain this tick: where it stands as the tick begins
		world.Tick();
		if (!dry && col >= 16 && col <= 19 && !s->Biting()) {
			dry = true;
			CHECK(s->Mood() == TerrainMood::Neutral);
			checkFactors(*s, 1.f, 1.f, 1.f);
		}
		if (!wet && col >= 11 && col <= 14 && s->InWater()) {
			wet = true;
			CHECK(s->Mood() == TerrainMood::Hostile);
			const TerrainFactors& hostile = row(Terrain::Water).hostile;
			checkFactors(*s, std::min(WADE_SPEED_FACTOR, hostile.speed), hostile.interval, hostile.damage);
		}
	}
	CHECK(dry);
	CHECK(wet); // chasing, it follows the player into the water
	CHECK(cellOf(s->CentreX()) < 9); // through it and on
}

TEST_CASE("a monster in a hostile cell, the player off its row, walks out to the nearer better cell and stays") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/terrain_idle"));
	world.WaitTicks(3);
	const Monster* s = find(world, MonsterScorpion);
	REQUIRE(s != nullptr);
	REQUIRE(s->InWater());
	world.Wait(8000); // a tile and a half at its slow wade
	CHECK_FALSE(s->InWater());
	CHECK(cellOf(s->CentreX()) == 23); // east: two cells, not five west
	CHECK(s->Mood() == TerrainMood::Neutral);
	CHECK_FALSE(s->Alerted()); // it did not notice the player by it
	CHECK(world.Saw(MonsterScorpion, CreatureMove::AvoidTerrain));
	for (int t = 0; t < 4000; t += UPDATE_TICK_MS) {
		world.Tick();
		REQUIRE(cellOf(s->CentreX()) == 23);
	}
}

TEST_CASE("a kiter backing off from the player does not step into a hostile cell") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/terrain_kite"));
	world.Tick();
	const Monster* b = find(world, MonsterBaboon);
	REQUIRE(b != nullptr);
	for (int t = 0; t < 10000; t += UPDATE_TICK_MS) {
		world.Step(1);
		world.Tick();
		REQUIRE_FALSE(b->InWater());
		REQUIRE(cellOf(b->CentreX()) < 15);
	}
	CHECK(world.Saw(MonsterBaboon, CreatureMove::Kite));
}

TEST_CASE("the checker: a native in its terrain scores higher, a monster spawned in a hostile cell is warned of") {
	LevelGrid grid;
	REQUIRE(loadLevelFile("tests/levels/terrain_idle", grid).empty());
	const LevelReport idle = checkLevel(grid);
	CHECK(std::any_of(idle.warnings.begin(), idle.warnings.end(), [](const std::string& w) {
		return w.find("scorpion") != std::string::npos && w.find("hostile terrain: water") != std::string::npos;
	}));

	REQUIRE(loadLevelFile("tests/levels/terrain", grid).empty());
	int scorpion = -1;
	for (int cell = 0; cell < LEVEL_CELL_COUNT; cell++)
		if (grid.cells[cell].type == MonsterSpawn && grid.cells[cell].attr == MonsterScorpion)
			scorpion = cell;
	REQUIRE(scorpion >= 0);
	const LevelReport sand = checkLevel(grid);
	CHECK(std::none_of(sand.warnings.begin(), sand.warnings.end(), [](const std::string& w) {
		return w.find("hostile terrain") != std::string::npos;
	}));
	grid.cells[scorpion].structure = Structure::Empty;
	const float onFloor = checkLevel(grid).difficulty;
	constexpr float SAND_CELL = 0.02f; // difficultyScore's for a sand cell on the path: one less on the floor
	CHECK(
		sand.difficulty - onFloor ==
		doctest::Approx(
			monsterThreat(MonsterScorpion) * (terrainThreatFactor(row(Terrain::Sand).native) - 1.f) + SAND_CELL
		)
	);
	CHECK(terrainThreatFactor(row(Terrain::Sand).native) > 1.f);
	CHECK(terrainThreatFactor(HOSTILE_TERRAIN) == 1.f); // the estimate stays on the safe side
}
