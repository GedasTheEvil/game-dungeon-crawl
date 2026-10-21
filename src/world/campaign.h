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
// A level's zone (LevelMeta::zone): the ZoneId, or NO_ZONE.
[[nodiscard]] constexpr bool isZone(int zone) { return zone >= 0 && zone < ZONE_ID_COUNT; }
// The ZoneId of a slug, NO_ZONE if none has it.
[[nodiscard]] int zoneBySlug(const std::string& slug);

constexpr int ZONE_MONSTER_MAX = 6;

// A run of campaign levels in one zone. Each level file names its zone too (LevelMeta): a test checks they agree.
struct Zone {
	ZoneId id;
	int first; // first and last level
	int last;
	MonsterTypeId monsters[ZONE_MONSTER_MAX]; // its main monsters, 0 after the last; the boss's kin among them
	MonsterTypeId boss; // on a level of the zone, 0: none
};

// Today's 35 levels. The rebuild (plan denser-levels) switches it to the target table of docs/levels.md.
constexpr Zone ZONES[] = {
	{ZoneId::TombEntrance,
	 1,
	 5,
	 {MonsterRat, MonsterScarab, MonsterBat, MonsterPlant, MonsterWorm, MonsterScorpion},
	 MonsterBossScarab},
	{ZoneId::BatGalleries,
	 6,
	 10,
	 {MonsterBat, MonsterGiantBat, MonsterGiantRat, MonsterGiantScarab, MonsterCrocodile, MonsterCamelSpider},
	 MonsterVampireBat},
	{ZoneId::ScorpionNest,
	 11,
	 15,
	 {MonsterMummy, MonsterUshabti, MonsterGiantScarab, MonsterGiantBat, MonsterScorpion, MonsterGiantScorpion},
	 MonsterScorpionQueen},
	{ZoneId::SerpentTemple,
	 16,
	 20,
	 {MonsterCobra, MonsterMummy, MonsterAnubis, MonsterCrocodile, MonsterGiantBat},
	 MonsterApep},
	{ZoneId::SethsDesert,
	 21,
	 25,
	 {MonsterWorm, MonsterCamelSpider, MonsterGiantScorpion, MonsterGiantCobra, MonsterGiantBat, MonsterHippo},
	 MonsterSeth},
	{ZoneId::CisternsOfSobek,
	 26,
	 30,
	 {MonsterCrocodile, MonsterGiantCobra, MonsterHippo, MonsterMummy, MonsterAnubis, MonsterGiantBat},
	 MonsterSobek},
	// The High Priest's temporary home until the rebuild moves him to the priests' quarter.
	{ZoneId::Necropolis,
	 31,
	 34,
	 {MonsterAnubis, MonsterMummy, MonsterGiantScarab, MonsterGiantCobra, MonsterGiantScorpion, MonsterCursedMimic},
	 MonsterMummyPriest},
	{ZoneId::HallOfJudgment,
	 35,
	 35,
	 {MonsterAnubis, MonsterMummy, MonsterGiantCobra, MonsterCursedMimic},
	 MonsterAnubisBoss},
};
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
[[nodiscard]] LevelBanner levelBanner(int number);

// The level file, also the seed of its decorations.
[[nodiscard]] std::string campaignLevelFile(int number);

#endif
