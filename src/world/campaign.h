#ifndef CAMPAIGN_H
#define CAMPAIGN_H

// Order of the levels in a game: content/levels/lvl1 .. lvl<CAMPAIGN_LEVELS>. Each exit loads the next level,
// the last one holds the ankh that wins the game. Level numbers start at 1 (Dungeon::LevelNumber).
// The levels are grouped into zones (plan campaign-zones): a run of levels with its own stone, monsters, main
// mechanic and, mostly, a boss on its last level. Each zone is one hour of Ra's night journey through the Duat.

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

// What the zone's walls and floors are made of. Only sandstone has its own surfaces so far (decor.h).
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

constexpr int ZONE_MONSTER_MAX = 6;

struct Zone {
	ZoneId id;
	const char* name;
	const char* gate; // the hour's gate: "The Fourth Hour: the Gate of the Priests" (zoneTitle)
	int first; // first and last level
	int last;
	ZoneStone stone;
	ZoneMechanic mechanic;
	MonsterTypeId monsters[ZONE_MONSTER_MAX]; // its main monsters, 0 after the last; the boss's kin among them
	MonsterTypeId boss; // on a level of the zone, 0: none
};

// Today's 35 levels. The rebuild (plan denser-levels) switches it to the target table of docs/levels.md.
constexpr Zone ZONES[] = {
	{ZoneId::TombEntrance,
	 "Tomb entrance",
	 "the Gate of the Tomb",
	 1,
	 5,
	 ZoneStone::Limestone,
	 ZoneMechanic::Spikes,
	 {MonsterRat, MonsterScarab, MonsterBat, MonsterPlant, MonsterWorm, MonsterScorpion},
	 MonsterBossScarab},
	{ZoneId::BatGalleries,
	 "Bat galleries",
	 "the Gate of the Bats",
	 6,
	 10,
	 ZoneStone::Limestone,
	 ZoneMechanic::Levers,
	 {MonsterBat, MonsterGiantBat, MonsterGiantRat, MonsterGiantScarab, MonsterCrocodile, MonsterCamelSpider},
	 MonsterVampireBat},
	{ZoneId::ScorpionNest,
	 "Scorpion nest",
	 "the Gate of Serqet",
	 11,
	 15,
	 ZoneStone::Plaster,
	 ZoneMechanic::Poison,
	 {MonsterMummy, MonsterUshabti, MonsterGiantScarab, MonsterGiantBat, MonsterScorpion, MonsterGiantScorpion},
	 MonsterScorpionQueen},
	{ZoneId::SerpentTemple,
	 "Serpent temple",
	 "the Gate of the Serpent",
	 16,
	 20,
	 ZoneStone::Plaster,
	 ZoneMechanic::Darts,
	 {MonsterCobra, MonsterMummy, MonsterAnubis, MonsterCrocodile, MonsterGiantBat},
	 MonsterApep},
	{ZoneId::SethsDesert,
	 "Seth's desert",
	 "the Gate of Seth",
	 21,
	 25,
	 ZoneStone::Sandstone,
	 ZoneMechanic::Sand,
	 {MonsterWorm, MonsterCamelSpider, MonsterGiantScorpion, MonsterGiantCobra, MonsterGiantBat, MonsterHippo},
	 MonsterSeth},
	{ZoneId::CisternsOfSobek,
	 "Cisterns of Sobek",
	 "the Gate of Sobek",
	 26,
	 30,
	 ZoneStone::WetStone,
	 ZoneMechanic::Water,
	 {MonsterCrocodile, MonsterGiantCobra, MonsterHippo, MonsterMummy, MonsterAnubis, MonsterGiantBat},
	 MonsterSobek},
	// The High Priest's temporary home until the rebuild moves him to the priests' quarter.
	{ZoneId::Necropolis,
	 "Necropolis",
	 "the Gate of the Dead",
	 31,
	 34,
	 ZoneStone::Granite,
	 ZoneMechanic::AllTraps,
	 {MonsterAnubis, MonsterMummy, MonsterGiantScarab, MonsterGiantCobra, MonsterGiantScorpion, MonsterCursedMimic},
	 MonsterMummyPriest},
	{ZoneId::HallOfJudgment,
	 "Hall of Judgment",
	 "the Gate of Judgment",
	 35,
	 35,
	 ZoneStone::Granite,
	 ZoneMechanic::Riddles,
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
[[nodiscard]] std::string zoneTitle(const Zone& zone);

// The level file, also the seed of its decorations.
[[nodiscard]] std::string campaignLevelFile(int number);

#endif
