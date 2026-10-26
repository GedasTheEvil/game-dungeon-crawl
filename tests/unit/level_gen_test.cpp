// Zone levels (plan campaign-levelgen): levels in the campaign's shape for a zone, passing the checker with no
// warnings over a range of seeds per zone.
#include "../../external/doctest/doctest.h"
#include "../../src/core/paths.h"
#include "../../src/world/campaign.h"
#include "../../src/world/decor.h"
#include "../../src/world/inscriptions.h"
#include "../../src/world/items.h"
#include "../../src/world/level.h"
#include "../../src/world/level_gen.h"
#include "../../src/world/monster_kinds.h"
#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <optional>
#include <set>
#include <sstream>
#include <string>

namespace {
GenOptions zoneOptions(uint32_t seed, int difficulty, int zone) {
	GenOptions options;
	options.seed = seed;
	options.difficulty = difficulty;
	options.zone = zone;
	return options;
}

std::string levelText(const LevelGrid& grid) {
	std::ostringstream out;
	writeLevel(out, grid.cells, grid.meta);
	return out.str();
}
} // namespace

TEST_CASE("zone levels fill the grid in the campaign's shape and pass the checker") {
	GenShape total;
	int gates = 0;
	for (int zone = 0; zone < ZONE_ID_COUNT; zone++) {
		for (uint32_t seed = 1; seed <= 2; seed++) {
			const int difficulty = 1 + (zone * 2 + static_cast<int>(seed)) % GEN_MAX_DIFFICULTY;
			CAPTURE(zone);
			CAPTURE(seed);
			CAPTURE(difficulty);
			const GenResult gen = generateLevel(zoneOptions(seed, difficulty, zone));
			REQUIRE(gen.ok);
			const LevelReport& r = gen.report;
			CHECK(r.valid);
			CHECK(r.warnings.empty());
			CHECK(gen.grid.meta.zone == zone);
			CHECK(std::abs(r.openCells - ZONE_OPEN_CELLS) < ZONE_OPEN_CELLS / 5);
			CHECK(r.boundsWidth >= LEVEL_WIDTH - 6); // the whole grid
			CHECK(r.boundsHeight >= LEVEL_HEIGHT - 8);
			CHECK(gen.shape.halls >= 10);
			total.tallHalls += gen.shape.tallHalls;
			total.loops += gen.shape.loops;
			total.hubs += gen.shape.hubs;
			total.ledges += gen.shape.ledges;
			total.shafts += gen.shape.shafts;
			gates += r.gates;
		}
	}
	CHECK(total.tallHalls > 0);
	CHECK(total.loops > 0);
	CHECK(total.hubs > 0);
	CHECK(total.ledges > 0);
	CHECK(total.shafts > 0);
	CHECK(gates > 0); // keys and levers in the wings
}

TEST_CASE("small zone levels (plan denser-levels): the first zones' ramp, fewer halls, keys before levers") {
	for (const int cells : {ZONE_SIZE[0].first, 200, ZONE_SIZE[0].last, ZONE_SIZE[1].first}) {
		for (uint32_t seed = 1; seed <= 3; seed++) {
			CAPTURE(cells);
			CAPTURE(seed);
			GenOptions options = zoneOptions(seed, 2, static_cast<int>(ZoneId::TombEntrance));
			options.openCells = cells;
			options.boss = seed == 3;
			const GenResult gen = generateLevel(options);
			REQUIRE(gen.ok);
			CHECK(gen.report.valid);
			CHECK(gen.report.warnings.empty());
			CHECK(std::abs(gen.report.openCells - cells) < cells / 3); // a boss room on top
			CHECK(gen.report.levers == 0); // the bat galleries bring them in (LEVERS_FROM)
		}
	}
}

TEST_CASE("a zone level's terrain and monsters follow the zone table") {
	for (int zone = 0; zone < ZONE_ID_COUNT; zone++) {
		const ZoneTerrain& terrain = zoneTerrain(static_cast<ZoneId>(zone));
		const ZoneFauna& fauna = zoneFauna(static_cast<ZoneId>(zone));
		int water = 0, deep = 0, sand = 0, quicksand = 0, sandFalls = 0, dark = 0, torches = 0, braziers = 0;
		int shafts = 0, discs = 0, turns = 0;
		std::set<int> kinds; // of the set's, met
		int strangers = 0; // monsters from outside the set (mimics aside)
		int spidersDark = 0, spidersLit = 0, wormsInSand = 0, coffins = 0, figures = 0;
		for (uint32_t seed = 1; seed <= 4; seed++) {
			CAPTURE(zone);
			CAPTURE(seed);
			const GenResult gen = generateLevel(zoneOptions(seed, 7, zone));
			REQUIRE(gen.ok);
			CHECK(gen.report.warnings.empty());
			turns += gen.report.pathTurns;
			for (const Tile& t : gen.grid.cells) {
				water += inHalfWater(t) ? 1 : 0;
				deep += t.structure == Structure::DeepWater ? 1 : 0;
				sand += t.structure == Structure::Sand ? 1 : 0;
				quicksand += inQuicksand(t) ? 1 : 0;
				sandFalls += isSandFall(t) ? 1 : 0;
				dark += t.dressing.torch == TorchPlacing::None ? 1 : 0;
				torches += t.dressing.torch == TorchPlacing::Torch ? 1 : 0;
				braziers += t.dressing.prop == DECOR_BRAZIER ? 1 : 0;
				shafts += t.type == LightShaft ? 1 : 0;
				discs += t.fixture.type == SunDisc ? 1 : 0;
				coffins += t.dressing.prop == DECOR_COFFIN_MUMMY ? 1 : 0;
				figures += t.dressing.prop == DECOR_USHABTI ? 1 : 0;
				if (t.type != MonsterSpawn || t.attr == MonsterMimic || t.attr == MonsterCursedMimic)
					continue;
				bool inSet = false;
				for (const ZoneMonster& m : fauna.monsters)
					inSet = inSet || (m.weight > 0 && m.type == t.attr);
				strangers += inSet ? 0 : 1;
				kinds.insert(t.attr);
				if (t.attr == MonsterCamelSpider)
					(t.dressing.torch == TorchPlacing::None ? spidersDark : spidersLit)++;
				wormsInSand += t.attr == MonsterWorm && inSand(t) ? 1 : 0;
			}
		}
		CAPTURE(zone);
		CHECK(strangers == 0);
		CHECK(kinds.size() >= 3);
		if (zone == static_cast<int>(ZoneId::WebHalls))
			CHECK(spidersDark > spidersLit); // the spiders run in the dark halls
		if (terrain.sand == TerrainAmount::Much)
			CHECK(wormsInSand > 0); // the worms lie under the drifts
		if (zone == static_cast<int>(ZoneId::Necropolis))
			CHECK(coffins > 0); // mummies among decoy coffins
		if (zone == static_cast<int>(ZoneId::PriestsQuarter))
			CHECK(figures > 0); // living ushabti among decor ones
		CHECK((water > 0) == (terrain.water != TerrainAmount::None));
		CHECK((deep > 0) == (terrain.water >= TerrainAmount::Some));
		CHECK((sand > 0) == (terrain.sand != TerrainAmount::None));
		CHECK((quicksand > 0) == (terrain.sand >= TerrainAmount::Some));
		CHECK((sandFalls > 0) == (terrain.sand == TerrainAmount::Much));
		CHECK((dark > 0) == (terrain.dark != TerrainAmount::None));
		CHECK((torches > 0) == (terrain.dark != TerrainAmount::None));
		CHECK((braziers > 0) == terrain.braziers);
		CHECK((shafts > 0) == (terrain.lightShafts || terrain.mirrors));
		CHECK((discs > 0) == terrain.mirrors);
		CHECK((turns > 0) == terrain.mirrors); // a disc opens a gate on the way: the mirror turned
		if (terrain.water == TerrainAmount::Much)
			CHECK(water > 4 * 60); // flooded halls
	}
}

TEST_CASE("a zone level's traps follow the zone table; test levels keep theirs") {
	for (int zone = 0; zone < ZONE_ID_COUNT; zone++) {
		const ZoneTraps& traps = zoneTraps(static_cast<ZoneId>(zone));
		int darts = 0, wallSpikes = 0, fireVents = 0, drips = 0, gauntlets = 0, pairs = 0;
		for (uint32_t seed = 1; seed <= 3; seed++) {
			CAPTURE(zone);
			CAPTURE(seed);
			const GenResult gen = generateLevel(zoneOptions(seed, 8, zone));
			REQUIRE(gen.ok);
			const LevelReport& r = gen.report;
			CHECK(r.warnings.empty()); // an antidote with the plates and drips, every run crossable
			darts += r.dartPlates;
			wallSpikes += r.wallSpikes;
			fireVents += r.fireVents;
			drips += r.poisonDrips;
			gauntlets += gen.shape.gauntlets;
			for (int cell = 0; cell + 1 < LEVEL_CELL_COUNT; cell++)
				pairs += isRhythmTrap(gen.grid.cells[cell].fixture.type) &&
								 isRhythmTrap(gen.grid.cells[cell + 1].fixture.type)
							 ? 1
							 : 0;
		}
		CAPTURE(zone);
		// None without a weight, some with the full one (a smaller one may miss three levels).
		auto follows = [](int count, int weight) { return weight == 0 ? count == 0 : weight < 100 || count > 0; };
		CHECK(follows(darts, traps.darts));
		CHECK(follows(wallSpikes, traps.wallSpikes));
		CHECK(follows(fireVents, traps.fireVents));
		CHECK(follows(drips, traps.poisonDrips));
		CHECK((gauntlets > 0) == (traps.gauntlets > 0));
		CHECK(gauntlets <= 3 * traps.gauntlets);
		if (zone == static_cast<int>(ZoneId::SerpentTemple))
			CHECK(pairs > 0); // two in step, from difficulty 6
	}
	for (int difficulty = GEN_MIN_DIFFICULTY; difficulty <= GEN_MAX_DIFFICULTY; difficulty++) {
		GenOptions options;
		options.seed = 5;
		options.difficulty = difficulty;
		const GenResult gen = generateLevel(options);
		REQUIRE(gen.ok);
		CHECK(gen.report.dartPlates + gen.report.wallSpikes + gen.report.fireVents + gen.report.poisonDrips == 0);
		CHECK(gen.shape.gauntlets == 0);
	}
}

TEST_CASE("a zone level's gates follow the zone table: riddles, name gates, hidden rooms, treasure tombs, chains") {
	int chains = 0;
	for (int zone = 0; zone < ZONE_ID_COUNT; zone++) {
		const ZoneGates& gates = zoneGates(static_cast<ZoneId>(zone));
		int riddles = 0, nameGates = 0, hidden = 0, tombs = 0;
		for (uint32_t seed = 1; seed <= 3; seed++) {
			CAPTURE(zone);
			CAPTURE(seed);
			const GenResult gen = generateLevel(zoneOptions(seed, 7, zone));
			REQUIRE(gen.ok);
			const LevelReport& r = gen.report;
			CHECK(r.warnings.empty()); // the cartouche apart from its gate, every room's chest in reach
			CHECK(r.riddles == gen.shape.riddleGates);
			CHECK(r.nameGates == gen.shape.nameGates);
			CHECK(r.nameCartouches == r.nameGates);
			CHECK(r.teleporters == 2 * (gen.shape.nameGates + gen.shape.hiddenRooms));
			CHECK(r.pathTeleports == 0); // the rooms are off the way to the exit
			riddles += r.riddles;
			nameGates += r.nameGates;
			hidden += gen.shape.hiddenRooms;
			tombs += gen.shape.treasureTombs;
			chains += gen.shape.chains;
		}
		CAPTURE(zone);
		CHECK(riddles >= 3 * gates.riddles - 1);
		if (gates.nameGatePercent == 0)
			CHECK(nameGates == 0);
		if (gates.nameGatePercent >= 80)
			CHECK(nameGates > 0);
		CHECK((hidden > 0) == (gates.hiddenRooms > 0));
		CHECK((tombs > 0) == (gates.treasureTombs > 0));
	}
	CHECK(chains > 0); // a key behind an earlier gate
}

TEST_CASE("a zone level's supplies: trapped chests of plain potions, Hapi before the water, resistance with poison") {
	for (int zone = 0; zone < ZONE_ID_COUNT; zone++) {
		const int trappedWanted = ZONE_TRAPPED_CHESTS[zone];
		int trapped = 0, hapiByWater = 0, hapi = 0;
		for (uint32_t seed = 1; seed <= 3; seed++) {
			CAPTURE(zone);
			CAPTURE(seed);
			const GenResult gen = generateLevel(zoneOptions(seed, 6, zone));
			REQUIRE(gen.ok);
			const LevelReport& r = gen.report;
			CHECK(r.warnings.empty()); // an antidote in reach with any poison, none of it in a trapped chest
			CHECK(r.trappedTreasures == gen.shape.trappedChests);
			CHECK(r.trappedTreasures <= trappedWanted);
			trapped += r.trappedTreasures;
			bool poison = false, lesser = false, greater = false;
			for (int cell = 0; cell < LEVEL_CELL_COUNT; cell++) {
				const Tile& t = gen.grid.cells[cell];
				poison = poison || (t.type == MonsterSpawn && isPoisoner(t.attr)) || t.type == DartPlate ||
						 t.fixture.type == PoisonDrip;
				const std::optional<ItemKind> item = isTreasure(t) ? itemFromFile(t.attr, t.value) : std::nullopt;
				if (t.type == TrappedTreasure) // only what the level can do without
					CHECK(
						(item == ItemKind::SmallHealth || item == ItemKind::LargeHealth ||
						 item == ItemKind::SmallStamina || item == ItemKind::LargeStamina || item == ItemKind::Might ||
						 item == ItemKind::Armor)
					);
				lesser = lesser || (t.type == Treasure && item == ItemKind::LesserResistance);
				greater = greater || (t.type == Treasure && item == ItemKind::GreaterResistance);
				if (t.type != Treasure || item != ItemKind::WaterBlessing)
					continue;
				hapi++;
				const int col = cell % LEVEL_WIDTH, row = cell / LEVEL_WIDTH;
				for (int dx = -6; dx <= 6; dx++)
					if (inHalfWater(gen.grid.at(col + dx, row))) {
						hapiByWater++;
						break;
					}
			}
			if (poison)
				CHECK(lesser);
			if (poison && zone >= static_cast<int>(GREATER_RESISTANCE_FROM))
				CHECK(greater);
		}
		CAPTURE(zone);
		CHECK((trapped > 0) == (trappedWanted > 0));
		if (zoneTerrain(static_cast<ZoneId>(zone)).water == TerrainAmount::Much)
			CHECK(hapiByWater > 0); // a few cells before the first wade
		if (zoneTerrain(static_cast<ZoneId>(zone)).water == TerrainAmount::None)
			CHECK(hapi == 0);
	}
}

TEST_CASE("a zone level's carried content: its zone's inscriptions, a page, a title, the Stargate's stand-in") {
	InscriptionBook book;
	REQUIRE(book.Load(CONTENT_DIR "inscriptions").empty());
	// The table of the zones' texts follows content/inscriptions/rebuild.txt: every id of a range has a text.
	for (int zone = 0; zone < ZONE_ID_COUNT; zone++)
		for (int id = ZONE_TEXT_FIRST[zone]; id < ZONE_TEXT_FIRST[zone + 1]; id++)
			CHECK_MESSAGE((!isInscriptionId(id) || book.Find(id) != nullptr), id);
	for (int id : SECRET_ROOM_TEXTS)
		CHECK(book.Find(id) != nullptr);

	for (int zone = 0; zone < ZONE_ID_COUNT; zone++)
		for (uint32_t seed = 1; seed <= 2; seed++) {
			CAPTURE(zone);
			CAPTURE(seed);
			GenOptions options = zoneOptions(seed, 6, zone);
			options.texts = &book;
			const GenResult gen = generateLevel(options);
			REQUIRE(gen.ok);
			CHECK(gen.report.warnings.empty()); // each inscription where the player can stand in front of it
			std::set<int> ids;
			int onHalls = 0, boons = 0;
			for (const Tile& t : gen.grid.cells) {
				if (t.fixture.type != Inscription || t.fixture.attr == NAME_CARTOUCHE_ID)
					continue;
				const int id = t.fixture.attr;
				CHECK(ids.insert(id).second); // once a level
				REQUIRE(book.Find(id) != nullptr);
				boons += book.Find(id)->boon.Any() ? 1 : 0;
				const bool lore = std::find(std::begin(SECRET_ROOM_TEXTS), std::end(SECRET_ROOM_TEXTS), id) !=
								  std::end(SECRET_ROOM_TEXTS);
				CHECK((lore || (id >= ZONE_TEXT_FIRST[zone] && id < ZONE_TEXT_FIRST[zone + 1])));
				CHECK(id != STARGATE_TEXT);
				onHalls += lore ? 0 : 1;
			}
			CHECK(onHalls == gen.shape.inscriptions);
			CHECK(onHalls >= (zone == 0 ? 1 : 2));
			CHECK(onHalls <= (zone == 0 ? 2 : 3));
			CHECK(boons <= 1);
			CHECK(gen.report.inscriptions == onHalls + 2 * gen.shape.nameGates); // a lore text and a cartouche each
		}

	GenOptions options = zoneOptions(4, 6, static_cast<int>(ZoneId::Necropolis));
	options.texts = &book;
	options.inscriptions = {123, 126};
	options.page = 22;
	options.title = "The Hall of the Test";
	options.stargate = true;
	const GenResult gen = generateLevel(options);
	REQUIRE(gen.ok);
	CHECK(gen.report.warnings.empty());
	CHECK(gen.grid.meta.title == "The Hall of the Test");
	CHECK(gen.report.nameGates == 1); // the password to the ring's chamber
	int pages = 0, given = 0, ring = 0;
	for (const Tile& t : gen.grid.cells) {
		pages += t.type == Page && t.attr == 22 ? 1 : 0;
		given += t.fixture.type == Inscription && (t.fixture.attr == 123 || t.fixture.attr == 126) ? 1 : 0;
		ring += t.fixture.type == Inscription && t.fixture.attr == STARGATE_TEXT ? 1 : 0;
	}
	CHECK(pages == 1);
	CHECK(given == 2);
	CHECK(ring == 1);
	CHECK(gen.shape.inscriptions == 2);
}

TEST_CASE("a route fork: two one-way routes to the exit, the warning between the drops, the page on the way") {
	for (int zone : {static_cast<int>(ZoneId::Necropolis), static_cast<int>(ZoneId::SerpentTemple)})
		for (uint32_t seed = 1; seed <= 2; seed++) {
			CAPTURE(zone);
			CAPTURE(seed);
			GenOptions options = zoneOptions(seed, 6, zone);
			options.fork = 2;
			options.page = 24;
			const GenResult gen = generateLevel(options);
			REQUIRE(gen.ok);
			CHECK(gen.report.warnings.empty()); // no softlock: each route alone reaches the exit
			REQUIRE(gen.shape.forks == 1);
			int warning = -1, page = -1;
			for (int cell = 0; cell < LEVEL_CELL_COUNT; cell++) {
				warning = gen.grid.cells[cell].fixture.type == Inscription &&
								  gen.grid.cells[cell].fixture.attr == FORK_TEXT
							  ? cell
							  : warning;
				page = gen.grid.cells[cell].type == Page && gen.grid.cells[cell].attr == 24 ? cell : page;
			}
			REQUIRE(warning >= 0);
			REQUIRE(page >= 0);
			const int col = warning % LEVEL_WIDTH, row = warning / LEVEL_WIDTH;
			// The two drops either side of it, the pillar of rock under it.
			CHECK_FALSE(isSolidStructure(gen.grid.at(col - 2, row - 1).structure));
			CHECK_FALSE(isSolidStructure(gen.grid.at(col + 2, row - 1).structure));
			CHECK(isWall(gen.grid.at(col, row - 1)));
			bool onPath = false;
			for (const CellPos& p : gen.report.path)
				onPath = onPath || p.row * LEVEL_WIDTH + p.col == page;
			CHECK(onPath); // the page lies on the way to the fork

			// A copy whose right route ends short of its drop: a softlock.
			LevelGrid blocked = gen.grid;
			int landing = row - 1;
			while (!isWall(blocked.at(col + 2, landing - 1)))
				landing--;
			blocked.set(col + 3, landing, wallTile());
			CHECK_FALSE(checkLevel(blocked).warnings.empty());
		}
}

TEST_CASE("a zone level's decor layer: tombs and sealed rooms dressed, the boss gate's guardian, webs in the dark") {
	int tombProps = 0, guardians = 0, suns = 0, webs = 0;
	for (int zone : {static_cast<int>(ZoneId::WebHalls), static_cast<int>(ZoneId::HallOfJudgment)})
		for (uint32_t seed = 1; seed <= 2; seed++) {
			CAPTURE(zone);
			CAPTURE(seed);
			GenOptions options = zoneOptions(seed, 6, zone);
			options.boss = true;
			const GenResult gen = generateLevel(options);
			REQUIRE(gen.ok);
			CHECK(gen.report.warnings.empty()); // every placed prop and decal fits its cell
			int placed = 0;
			for (const Tile& t : gen.grid.cells) {
				const int prop = t.dressing.prop;
				placed += (prop >= 0 ? 1 : 0) + (t.dressing.decal >= 0 ? 1 : 0);
				tombProps += prop == DECOR_SARCOPHAGUS || prop == DECOR_CANOPIC || prop == DECOR_SCROLLS ? 1 : 0;
				guardians += t.dressing.prop == DECOR_CAT || t.dressing.prop == DECOR_JACKAL ? 1 : 0;
				suns += t.dressing.decal == DECAL_WINGED_SUN ? 1 : 0;
				webs += prop == DECOR_WEB ? 1 : 0;
			}
			CHECK(placed >= gen.shape.placedDecor); // the braziers, coffins and figures besides
		}
	CHECK(tombProps > 0);
	CHECK(guardians == 4); // one before each boss gate
	CHECK(suns == 4);
	CHECK(webs > 0); // the web halls' dark halls
}

TEST_CASE("a zone's boss level: the boss in a sealed room, its key for the last gate behind the boss gate") {
	for (int zone = 0; zone < ZONE_ID_COUNT; zone++) {
		const int boss = zoneBoss(static_cast<ZoneId>(zone));
		if (boss == 0)
			continue;
		for (uint32_t seed = 1; seed <= 2; seed++) {
			CAPTURE(zone);
			CAPTURE(seed);
			GenOptions options = zoneOptions(seed, 6, zone);
			options.boss = true;
			const GenResult gen = generateLevel(options);
			REQUIRE(gen.ok);
			const LevelReport& r = gen.report;
			CHECK(r.warnings.empty()); // only a teleporter leads to the boss, the boss gate opens with it
			CHECK(gen.shape.bossRoom);
			CHECK(r.bosses == 1);
			CHECK(r.monsters[boss] == 1);
			CHECK(r.pathBoss); // the way to the exit goes through the boss gate
			CHECK(r.pathTeleports == 2); // there and back
			int eggs = 0;
			for (const Tile& t : gen.grid.cells) {
				eggs += t.type == MonsterSpawn && t.attr == MonsterEggCluster ? 1 : 0;
				if (t.type != MonsterSpawn || t.attr != boss)
					continue;
				if (boss == MonsterSobek || boss == MonsterSeth)
					CHECK(inHalfWater(t));
				if (boss == MonsterRerek)
					CHECK(inSand(t));
			}
			CHECK((eggs >= 2) == (boss == MonsterScorpionQueen));
		}
	}
	GenOptions options = zoneOptions(1, 6, static_cast<int>(ZoneId::CisternsOfSobek));
	const GenResult plain = generateLevel(options);
	CHECK(plain.report.bosses == 0);
	CHECK_FALSE(plain.shape.bossRoom);
}

TEST_CASE("a zone level comes out the same for the same seed, and its size follows the target") {
	const int zone = static_cast<int>(ZoneId::CisternsOfSobek);
	const GenResult a = generateLevel(zoneOptions(9, 6, zone));
	const GenResult b = generateLevel(zoneOptions(9, 6, zone));
	REQUIRE(a.ok);
	CHECK(levelText(a.grid) == levelText(b.grid));
	CHECK(levelText(a.grid) != levelText(generateLevel(zoneOptions(10, 6, zone)).grid));

	GenOptions small = zoneOptions(9, 6, zone);
	small.openCells = 450;
	const GenResult s = generateLevel(small);
	REQUIRE(s.ok);
	CHECK(s.report.warnings.empty());
	CHECK(std::abs(s.report.openCells - 450) < 450 / 5);
}

TEST_CASE("gen specs: a test level, or a zone level by the zone's slug") {
	GenOptions options;
	REQUIRE(parseGenSpec("gen:7:3", options));
	CHECK(options.seed == 7);
	CHECK(options.difficulty == 3);
	CHECK(options.zone == NO_ZONE);
	REQUIRE(parseGenSpec("gen:7:4:necropolis", options));
	CHECK(options.difficulty == 4);
	CHECK(options.zone == static_cast<int>(ZoneId::Necropolis));
	CHECK_FALSE(options.boss);
	REQUIRE(parseGenSpec("gen:7:4:cisterns_of_sobek:boss", options));
	CHECK(options.boss);
	CHECK(options.zone == static_cast<int>(ZoneId::CisternsOfSobek));
	CHECK_FALSE(parseGenSpec("gen:7:4:necropolis:boss", options)); // no boss there
	CHECK_FALSE(parseGenSpec("gen:7:4:necropolis:first", options));
	CHECK_FALSE(parseGenSpec("gen:7:4:nowhere", options));
	CHECK_FALSE(parseGenSpec("gen:7:4x", options));
	CHECK_FALSE(parseGenSpec("gen:7", options));
	CHECK_FALSE(parseGenSpec("lvl3", options));
}
