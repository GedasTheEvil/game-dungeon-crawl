// The lesser Anubis statue (plan lesser-anubis-guard): the guard at half size, no bolt, before the necropolis; the
// necropolis's guard and the Anubis boss 3x HP, 2x damage.
#include "../../external/doctest/doctest.h"
#include "../../src/level/campaign.h"
#include "../../src/level/level_check.h"
#include "../../src/level/monster_kinds.h"
#include <string>

TEST_CASE("the lesser Anubis statue: the guard's model at half size, 0.75x its old HP and blow, no bolt") {
	const MonsterKind& lesser = *monsterKind(MonsterLesserAnubis);
	const MonsterKind& guard = *monsterKind(MonsterAnubis);
	CHECK(std::string(lesser.model) == guard.model);
	CHECK(std::string(lesser.texture) == guard.texture);
	CHECK(lesser.scale == doctest::Approx(guard.scale / 2.f));
	CHECK(lesser.maxHealth == 975);
	CHECK(lesser.damage == 105);
	CHECK_FALSE(lesser.ranged.has_value());
	CHECK(lesser.rangedCurses[0].percent == 0);
	CHECK(lesser.melee.height == AttackHeight::Body); // from half the height: a crouch does not duck it
	CHECK(lesser.heightQuarters == 1); // through a crawl space
	CHECK(lesser.climbs);
	CHECK(lesser.bloodless);
	CHECK(lesser.poisonResistPercent == guard.poisonResistPercent); // immune
	CHECK(lesser.courage == Courage::Reckless);
	CHECK(lesser.xp < guard.xp);
	CHECK(monsterThreat(MonsterLesserAnubis) < monsterThreat(MonsterAnubis));
}

TEST_CASE("the necropolis's guard: 3x HP, 2x blow and bolt; the Anubis boss the same, still above it") {
	const MonsterKind& guard = *monsterKind(MonsterAnubis);
	const MonsterKind& boss = *monsterKind(MonsterAnubisBoss);
	CHECK(guard.maxHealth == 3900);
	CHECK(guard.damage == 280);
	REQUIRE(guard.ranged.has_value());
	CHECK(guard.ranged->damage == 140);
	CHECK(boss.maxHealth == 7200);
	CHECK(boss.damage == 380);
	REQUIRE(boss.ranged.has_value());
	CHECK(boss.ranged->damage == 250);
	CHECK(boss.maxHealth > guard.maxHealth);
	CHECK(boss.damage > guard.damage);
	CHECK(boss.ranged->damage > guard.ranged->damage);
	CHECK(boss.threat > guard.threat);
	CHECK(boss.xp > guard.xp);
}

TEST_CASE("the campaign: lesser statues before the necropolis, full guards only in it") {
	for (int n = 1; n <= CAMPAIGN_LEVELS; n++) {
		CAPTURE(n);
		LevelGrid grid;
		REQUIRE(loadLevelFile(campaignLevelFile(n).c_str(), grid).empty());
		int guards = 0;
		int lesser = 0;
		for (const Tile& t : grid.cells) {
			if (t.type != MonsterSpawn)
				continue;
			guards += t.attr == MonsterAnubis ? 1 : 0;
			lesser += t.attr == MonsterLesserAnubis ? 1 : 0;
		}
		const ZoneId zone = zoneOf(n)->id;
		if (zone == ZoneId::Necropolis || zone == ZoneId::HallOfJudgment)
			CHECK(lesser == 0);
		else
			CHECK(guards == 0);
		if (zone == ZoneId::PriestsQuarter)
			CHECK(lesser > 0);
	}
	CHECK(zoneFauna(ZoneId::PriestsQuarter).monsters[2].type == MonsterLesserAnubis);
}
