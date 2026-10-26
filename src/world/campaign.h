#ifndef CAMPAIGN_H
#define CAMPAIGN_H

// Order of the levels in a game: content/levels/lvl1 .. lvl<CAMPAIGN_LEVELS>. Each exit loads the next level,
// the last one holds the ankh that wins the game. Level numbers start at 1 (Dungeon::LevelNumber).
// The levels are grouped into zones (plan campaign-zones): a run of levels with its own stone, monsters, main
// mechanic and, mostly, a boss on its last level. Each zone is one hour of Ra's night journey through the Duat.
// A level file names its zone (LevelMeta, level.h): what it looks like (zone_look.h) follows the file.

#include "level.h"
#include <string>

// The twelve zones of the target campaign (docs/levels.md, Zones). Today's campaign has eight of them.
enum class ZoneId : unsigned char {
	TombEntrance,
	BatGalleries,
	WebHalls,
	PriestsQuarter,
	ScorpionNest,
	SerpentTemple,
	CisternsOfSobek,
	SethsDesert,
	LandOfSokar,
	LakeOfFire,
	Necropolis,
	HallOfJudgment,
};
constexpr int ZONE_ID_COUNT = static_cast<int>(ZoneId::HallOfJudgment) + 1;

// What the zone's dressed walls, floors and ceilings are made of (scatterSurfaces, decor.h); plaster is limestone
// painted more often.
enum class ZoneStone : unsigned char { Limestone, Plaster, WetStone, Sandstone, Basalt, Granite };

// The new thing the zone teaches.
enum class ZoneMechanic : unsigned char {
	Spikes, // spikes, pits, the first key and riddle
	Levers, // levers, rock falls
	Dark, // darkness and light
	Inscriptions, // name gates, inscriptions
	Poison,
	Darts, // dart plates, rhythm traps
	Water, // deep water, swimming
	Sand, // sand, mirrors and sun discs
	Quicksand,
	Fire,
	AllTraps, // every trap mixed
	Riddles,
};

// What a zone is, by ZoneId: its name in the level files and the editor, in the game, its gate, stone and mechanic.
struct ZoneInfo {
	const char* slug; // the level file's "zone" line (level.h), the editor
	const char* name;
	const char* gate; // the hour's gate: "The Fourth Hour: the Gate of the Priests" (zoneTitle)
	ZoneStone stone;
	ZoneMechanic mechanic;
};
constexpr ZoneInfo ZONE_INFO[ZONE_ID_COUNT] = {
	{"tomb_entrance", "Tomb entrance", "the Gate of the Tomb", ZoneStone::Limestone, ZoneMechanic::Spikes},
	{"bat_galleries", "Bat galleries", "the Gate of the Bats", ZoneStone::Limestone, ZoneMechanic::Levers},
	{"web_halls", "Web halls", "the Gate of the Webs", ZoneStone::Limestone, ZoneMechanic::Dark},
	{"priests_quarter", "Priests' quarter", "the Gate of the Priests", ZoneStone::Plaster, ZoneMechanic::Inscriptions},
	{"scorpion_nest", "Scorpion nest", "the Gate of Serqet", ZoneStone::Plaster, ZoneMechanic::Poison},
	{"serpent_temple", "Serpent temple", "the Gate of the Serpent", ZoneStone::Plaster, ZoneMechanic::Darts},
	{"cisterns_of_sobek", "Cisterns of Sobek", "the Gate of Sobek", ZoneStone::WetStone, ZoneMechanic::Water},
	{"seths_desert", "Seth's desert", "the Gate of Seth", ZoneStone::Sandstone, ZoneMechanic::Sand},
	{"land_of_sokar", "Land of Sokar", "the Gate of Sokar", ZoneStone::Sandstone, ZoneMechanic::Quicksand},
	{"lake_of_fire", "Lake of Fire", "the Gate of Fire", ZoneStone::Basalt, ZoneMechanic::Fire},
	{"necropolis", "Necropolis", "the Gate of the Dead", ZoneStone::Granite, ZoneMechanic::AllTraps},
	{"hall_of_judgment", "Hall of Judgment", "the Gate of Judgment", ZoneStone::Granite, ZoneMechanic::Riddles},
};
[[nodiscard]] constexpr const ZoneInfo& zoneInfo(ZoneId id) { return ZONE_INFO[static_cast<int>(id)]; }

// How much of a terrain a zone's levels have (ZoneTerrain).
enum class TerrainAmount : unsigned char { None, Few, Some, Much };
// What a zone level's halls are made of besides rock (plan campaign-levelgen, level_gen.h): its water, sand and light.
struct ZoneTerrain {
	TerrainAmount water; // Few: a short wade; Some: wades and a pool; Much: flooded halls over deep pools
	TerrainAmount sand; // Few: a few drifts; Some: drifts and quicksand; Much: and sand falls
	TerrainAmount dark; // halls without a wall torch, a torch or two in the dark
	bool braziers;
	bool lightShafts; // daylight through cracks in the ceiling
	bool mirrors; // sun discs open gates: a mirror under a light shaft sends the beam to the disc
};
// By ZoneId (docs/levels.md, Zones).
constexpr ZoneTerrain ZONE_TERRAIN[ZONE_ID_COUNT] = {
	{TerrainAmount::None, TerrainAmount::None, TerrainAmount::None, false, false, false}, // tomb entrance
	{TerrainAmount::Few, TerrainAmount::None, TerrainAmount::Few, false, false, false}, // bat galleries: the first wade
	{TerrainAmount::None, TerrainAmount::None, TerrainAmount::Much, true, false, false}, // web halls: dark and light
	{TerrainAmount::None, TerrainAmount::None, TerrainAmount::None, true, false, false}, // priests' quarter
	{TerrainAmount::None, TerrainAmount::Few, TerrainAmount::Few, false, false, false}, // scorpion nest
	{TerrainAmount::Some, TerrainAmount::None, TerrainAmount::Few, true, false, false}, // serpent temple
	{TerrainAmount::Much, TerrainAmount::None, TerrainAmount::Few, false, false, false}, // cisterns of Sobek
	{TerrainAmount::Few, TerrainAmount::Some, TerrainAmount::None, false, true, true}, // Seth's desert: the oasis
	{TerrainAmount::None, TerrainAmount::Much, TerrainAmount::Some, false, true, false}, // land of Sokar
	{TerrainAmount::Some, TerrainAmount::None, TerrainAmount::None, true, false, false}, // lake of fire: a refuge
	{TerrainAmount::Few, TerrainAmount::None, TerrainAmount::Some, true, false, false}, // necropolis
	{TerrainAmount::Few, TerrainAmount::None, TerrainAmount::Few, true, false, false}, // hall of judgment
};
[[nodiscard]] constexpr const ZoneTerrain& zoneTerrain(ZoneId id) { return ZONE_TERRAIN[static_cast<int>(id)]; }

constexpr int ZONE_MONSTER_MAX = 6;

// A zone level's monsters (plan campaign-levelgen): the target campaign's set for the zone (docs/levels.md, Zones;
// not today's ZONES), each by its weight, and the share of its chests that are mimics. The swimmers (crocodile,
// hippo) and the lotus go only in half water.
struct ZoneMonster {
	MonsterTypeId type;
	int weight;
};
struct ZoneFauna {
	ZoneMonster monsters[ZONE_MONSTER_MAX]; // type 0 after the last
	int mimicPercent; // of the chests
	int cursedPercent; // of the mimics
};
// By ZoneId.
constexpr ZoneFauna ZONE_FAUNA[ZONE_ID_COUNT] = {
	{{{MonsterRat, 4}, {MonsterScarab, 4}, {MonsterBat, 3}, {MonsterPlant, 2}}, 10, 0},
	{{{MonsterBat, 4}, {MonsterGiantBat, 3}, {MonsterWorm, 2}, {MonsterGiantRat, 3}, {MonsterCrocodile, 2}}, 15, 0},
	{{{MonsterCamelSpider, 4}, {MonsterGiantScarab, 3}, {MonsterGiantRat, 1}}, 30, 0},
	{{{MonsterMummy, 4}, {MonsterUshabti, 2}, {MonsterAnubis, 1}, {MonsterGiantScarab, 1}}, 15, 0},
	{{{MonsterScorpion, 4}, {MonsterGiantScorpion, 3}, {MonsterGiantScarab, 1}}, 15, 0},
	{{{MonsterCobra, 4}, {MonsterGiantCobra, 2}, {MonsterCrocodile, 2}, {MonsterGiantBat, 1}}, 15, 0},
	{{{MonsterCrocodile, 4}, {MonsterHippo, 1}, {MonsterGiantCobra, 3}, {MonsterLotus, 2}, {MonsterGiantBat, 1}},
	 15,
	 10},
	{{{MonsterCamelSpider, 3}, {MonsterGiantScorpion, 3}, {MonsterHippo, 2}, {MonsterWorm, 1}}, 15, 10},
	{{{MonsterWorm, 4}, {MonsterCamelSpider, 3}, {MonsterGiantScorpion, 1}}, 15, 10},
	{{{MonsterBaboon, 3}, {MonsterGiantCobra, 3}, {MonsterGiantBat, 2}}, 15, 20},
	{{{MonsterAnubis, 3}, {MonsterMummy, 4}, {MonsterBaboon, 1}, {MonsterGiantScorpion, 1}}, 25, 60},
	{{{MonsterAnubis, 4}, {MonsterMummy, 2}, {MonsterGiantCobra, 2}, {MonsterBaboon, 1}, {MonsterCamelSpider, 1}},
	 20,
	 50},
};
[[nodiscard]] constexpr const ZoneFauna& zoneFauna(ZoneId id) { return ZONE_FAUNA[static_cast<int>(id)]; }

// The boss on a zone's boss level in the target campaign (docs/levels.md, Zones), 0 for none (the necropolis): a
// generated boss level's (plan campaign-levelgen). Today's ZONES name the bosses of today's campaign.
constexpr int ZONE_BOSS[ZONE_ID_COUNT] = {
	MonsterBossScarab,
	MonsterVampireBat,
	MonsterMotherOfWebs,
	MonsterMummyPriest,
	MonsterScorpionQueen,
	MonsterApep,
	MonsterSobek,
	MonsterSeth,
	MonsterRerek,
	MonsterBabi,
	0,
	MonsterAnubisBoss,
};
[[nodiscard]] constexpr int zoneBoss(ZoneId id) { return ZONE_BOSS[static_cast<int>(id)]; }

// A zone level's traps (plan campaign-levelgen): the weight of each kind along its halls, in percent of a test
// level's (100: as on a test level of the difficulty; the new kinds have a weight of their own, 0 on a test level),
// and its trap gauntlets: a stretch of a hall on the route with a trap on every other cell.
struct ZoneTraps {
	int spikes;
	int pits; // death pits: a hole with a death trap in it, jumped
	int rockFalls; // sand falls among them in a zone with much sand
	int darts; // dart plates
	int wallSpikes; // the rhythm traps
	int fireVents;
	int poisonDrips;
	int gauntlets; // a level
};
// By ZoneId (docs/levels.md, Zones: the zone's new mechanic).
constexpr ZoneTraps ZONE_TRAPS[ZONE_ID_COUNT] = {
	{150, 150, 50, 0, 0, 0, 0, 0}, // tomb entrance: spikes and pits
	{100, 100, 200, 0, 0, 0, 0, 1}, // bat galleries: rock falls
	{100, 100, 100, 0, 0, 0, 0, 0}, // web halls
	{80, 80, 80, 0, 0, 100, 0, 1}, // priests' quarter: fire jets
	{80, 80, 80, 0, 0, 0, 200, 1}, // scorpion nest: poison drips
	{60, 60, 60, 150, 200, 0, 0, 2}, // serpent temple: dart plates, rhythm traps
	{60, 40, 60, 50, 50, 0, 0, 1}, // cisterns of Sobek
	{60, 60, 100, 50, 0, 0, 0, 1}, // Seth's desert
	{60, 60, 150, 0, 0, 0, 0, 1}, // land of Sokar: sand falls
	{50, 50, 50, 50, 0, 150, 0, 2}, // lake of fire: fire-jet runs
	{100, 100, 150, 100, 100, 100, 100, 2}, // necropolis: every trap mixed
	{80, 80, 80, 80, 80, 80, 80, 1}, // hall of judgment
};
[[nodiscard]] constexpr const ZoneTraps& zoneTraps(ZoneId id) { return ZONE_TRAPS[static_cast<int>(id)]; }

// A zone level's gates besides its locks (plan campaign-levelgen): riddle gates, the chance of a name gate to a secret
// room (its cartouche elsewhere on the level), hidden rooms a plain teleporter leads to, and treasure tombs: a wing
// behind a second gate of a colour the route already opened.
struct ZoneGates {
	int riddles; // a level
	int nameGatePercent;
	int hiddenRooms; // a level
	int treasureTombs; // a level
};
// By ZoneId (docs/levels.md, Zones). The first name gate comes in the bat galleries.
constexpr ZoneGates ZONE_GATES[ZONE_ID_COUNT] = {
	{1, 0, 0, 0}, // tomb entrance: the first riddle
	{1, 30, 0, 0}, // bat galleries
	{1, 40, 0, 1}, // web halls
	{1, 80, 0, 1}, // priests' quarter: name gates
	{1, 40, 0, 1}, // scorpion nest
	{1, 40, 0, 1}, // serpent temple
	{1, 40, 1, 1}, // cisterns of Sobek: a hidden treasure cistern
	{1, 40, 0, 1}, // Seth's desert
	{1, 40, 1, 1}, // land of Sokar
	{1, 40, 0, 1}, // lake of fire
	{1, 50, 1, 2}, // necropolis: hidden tombs
	{3, 50, 0, 1}, // hall of judgment: riddles
};
[[nodiscard]] constexpr const ZoneGates& zoneGates(ZoneId id) { return ZONE_GATES[static_cast<int>(id)]; }

// A zone level's trapped chests (plan campaign-levelgen, plan trapped-chests): from the web halls on, where the target
// campaign brings them in, more deeper down. By ZoneId.
constexpr int ZONE_TRAPPED_CHESTS[ZONE_ID_COUNT] = {0, 0, 2, 1, 2, 2, 2, 2, 3, 3, 3, 3};
// From this zone on a level with poison gets a greater resistance potion besides the lesser one.
constexpr ZoneId GREATER_RESISTANCE_FROM = ZoneId::ScorpionNest;
// From this zone on a gate on a zone level may have a lever instead of a key: the bat galleries bring the levers in.
constexpr ZoneId LEVERS_FROM = ZoneId::BatGalleries;

// A zone level's size in the target campaign (plan denser-levels): the open cells of its first and its last level,
// the levels between on the line. A ramp from the way in to the whole grid by the web halls, no jump.
struct ZoneSize {
	int first;
	int last;
};
// By ZoneId.
constexpr ZoneSize ZONE_SIZE[ZONE_ID_COUNT] = {
	{120, 280}, // tomb entrance: the way in
	{320, 500}, // bat galleries
	{540, 650}, // web halls: the whole grid
	{650, 650},
	{650, 650},
	{650, 650},
	{650, 650},
	{650, 650},
	{650, 650},
	{650, 650},
	{650, 650},
	{650, 650},
};
// The open cells of level `place` (0-based) of a zone of `levels` levels.
[[nodiscard]] constexpr int zoneLevelCells(ZoneId id, int place, int levels) {
	const ZoneSize& size = ZONE_SIZE[static_cast<int>(id)];
	return levels <= 1 ? size.last : size.first + (size.last - size.first) * place / (levels - 1);
}
// A level's zone (LevelMeta::zone): the ZoneId, or NO_ZONE.
[[nodiscard]] constexpr bool isZone(int zone) { return zone >= 0 && zone < ZONE_ID_COUNT; }
// The ZoneId of a slug, NO_ZONE if none has it.
[[nodiscard]] int zoneBySlug(const std::string& slug);

// A run of campaign levels in one zone. Each level file names its zone too (LevelMeta): a test checks they agree.
struct Zone {
	ZoneId id;
	int first; // first and last level
	int last;
	MonsterTypeId monsters[ZONE_MONSTER_MAX]; // its main monsters, 0 after the last; the boss's kin among them
	MonsterTypeId boss; // on a level of the zone, 0: none
};

// The campaign while the rebuild (plan denser-levels) goes zone by zone: the rebuilt zones on the target table of
// docs/levels.md first (through REBUILT_LEVELS), then today's levels after them, one number earlier per level the
// rebuilt zones are shorter.
constexpr Zone ZONES[] = {
	{ZoneId::TombEntrance, 1, 4, {MonsterRat, MonsterScarab, MonsterBat, MonsterPlant}, MonsterBossScarab},
	{ZoneId::BatGalleries,
	 5,
	 9,
	 {MonsterBat, MonsterGiantBat, MonsterWorm, MonsterGiantRat, MonsterCrocodile},
	 MonsterVampireBat},
	{ZoneId::WebHalls,
	 10,
	 13,
	 {MonsterCamelSpider, MonsterGiantScarab, MonsterGiantRat, MonsterMimic},
	 MonsterMotherOfWebs},
	{ZoneId::PriestsQuarter,
	 14,
	 18,
	 {MonsterMummy, MonsterUshabti, MonsterAnubis, MonsterGiantScarab, MonsterMimic},
	 MonsterMummyPriest},
	{ZoneId::ScorpionNest,
	 19,
	 22,
	 {MonsterScorpion, MonsterGiantScorpion, MonsterGiantScarab, MonsterMimic},
	 MonsterScorpionQueen},
	{ZoneId::SerpentTemple,
	 23,
	 27,
	 {MonsterCobra, MonsterGiantCobra, MonsterCrocodile, MonsterGiantBat, MonsterMimic},
	 MonsterApep},
	{ZoneId::CisternsOfSobek,
	 28,
	 32,
	 {MonsterCrocodile, MonsterHippo, MonsterGiantCobra, MonsterLotus, MonsterGiantBat, MonsterMimic},
	 MonsterSobek},
	{ZoneId::SethsDesert,
	 33,
	 37,
	 {MonsterCamelSpider, MonsterGiantScorpion, MonsterHippo, MonsterWorm, MonsterMimic},
	 MonsterSeth},
	{ZoneId::LandOfSokar, 38, 41, {MonsterWorm, MonsterCamelSpider, MonsterGiantScorpion, MonsterMimic}, MonsterRerek},
	{ZoneId::Necropolis,
	 42,
	 45,
	 {MonsterAnubis, MonsterMummy, MonsterGiantScarab, MonsterGiantCobra, MonsterGiantScorpion, MonsterCursedMimic},
	 MonsterTypeId{}}, // no boss
	{ZoneId::HallOfJudgment,
	 46,
	 46,
	 {MonsterAnubis, MonsterMummy, MonsterGiantCobra, MonsterCursedMimic},
	 MonsterAnubisBoss},
};
// The last level the rebuild has made (plan denser-levels): its levels follow the target table, their monsters the
// zone's set (ZONE_FAUNA), a test checks them.
constexpr int REBUILT_LEVELS = 41;
constexpr int ZONE_COUNT = static_cast<int>(sizeof(ZONES) / sizeof(ZONES[0]));

constexpr int CAMPAIGN_LEVELS = ZONES[ZONE_COUNT - 1].last;

// The zone of a campaign level, nullptr outside the campaign (a generated or test level).
[[nodiscard]] constexpr const Zone* zoneOf(int number) {
	for (const Zone& zone : ZONES)
		if (number >= zone.first && number <= zone.last)
			return &zone;
	return nullptr;
}

// "The Fourth Hour: the Gate of the Priests": the zone's hour is its place in the target campaign (ZoneId).
[[nodiscard]] std::string zoneTitle(ZoneId id);

// The banner over the game when a campaign level starts (plan zone-looks, ZoneBanner): a zone's first level names
// the hour and its gate, the zone and its guardian (none on a bossless zone); any other level its number and zone.
struct LevelBanner {
	bool zone = false; // the zone's banner, else the level's smaller one
	std::string title;
	std::string subtitle;
	std::string guardian; // empty: none
};
// title: the level's own title (LevelMeta::title), empty for none; it follows the level or the zone name.
[[nodiscard]] LevelBanner levelBanner(int number, const std::string& title = "");
// The title in a campaign level's file (cached), empty for none or a level outside the campaign: for the save slots.
[[nodiscard]] const std::string& campaignLevelTitle(int number);

// The level file, also the seed of its decorations.
[[nodiscard]] std::string campaignLevelFile(int number);

#endif
