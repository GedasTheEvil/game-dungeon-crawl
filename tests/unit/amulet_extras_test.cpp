// The crushing, cleaving and piercing amulets, and the journal note of every amulet type (plan amulet-extras).
#include "../../external/doctest/doctest.h"
#include "../../src/entities/player_stats.h"
#include "../../src/world/item_bag.h"
#include "../../src/world/journal.h"
#include "../../src/world/journal_text.h"
#include "sim_world.h"
#include <string>

namespace {
constexpr size_t BLUNT = static_cast<size_t>(DamageType::Blunt);
constexpr size_t SLASH = static_cast<size_t>(DamageType::Slash);
constexpr size_t PIERCE = static_cast<size_t>(DamageType::Pierce);
} // namespace

TEST_CASE("the typed might amulets: four tiers, +10 / 20 / 30 / 50% of one damage type") {
	const ItemKind crushing[4] = {ItemKind::CrushingLesser, ItemKind::CrushingMinor, ItemKind::CrushingNormal,
								  ItemKind::CrushingGrand};
	const int percent[4] = {10, 20, 30, 50};
	for (int tier = 0; tier < 4; tier++) {
		const AmuletBonus bonus = amuletBonus(crushing[tier]);
		CHECK(bonus.typeMightPercent[BLUNT] == percent[tier]);
		CHECK(bonus.typeMightPercent[SLASH] == 0);
		CHECK(bonus.typeMightPercent[PIERCE] == 0);
		CHECK(bonus.might == 0);
	}
	CHECK(amuletBonus(ItemKind::CleavingGrand).typeMightPercent[SLASH] == 50);
	CHECK(amuletBonus(ItemKind::PiercingNormal).typeMightPercent[PIERCE] == 30);
	CHECK(amuletBonus(ItemKind::StrengthGrand).typeMightPercent[BLUNT] == 0);
	CHECK(*amuletKind(AmuletType::Crushing, AmuletTier::Lesser) == ItemKind::CrushingLesser);
	CHECK(*amuletKind(AmuletType::Piercing, AmuletTier::Grand) == ItemKind::PiercingGrand);
	CHECK(static_cast<int>(ItemKind::CrushingLesser) == static_cast<int>(ItemKind::FireWardGrand) + 1); // after fire
}

TEST_CASE("typed might amulet names and effects") {
	CHECK(std::string(itemText(ItemKind::CrushingLesser).name) == "Lesser Seth's Mace");
	CHECK(std::string(itemText(ItemKind::CleavingGrand).name) == "Grand Montu's Khopesh");
	CHECK(std::string(itemText(ItemKind::PiercingNormal).name) == "Neith's Arrowhead");
	CHECK(std::string(itemText(ItemKind::CrushingMinor).label) == "minor amulet of crushing");
	CHECK(std::string(itemText(ItemKind::CrushingGrand).effect) == "Smash damage +50%");
	CHECK(std::string(itemText(ItemKind::CleavingMinor).effect) == "Cut damage +20%");
	CHECK(std::string(itemText(ItemKind::PiercingLesser).effect) == "Stab damage +10%");
	CHECK(std::string(amuletModel(AmuletType::Cleaving)) == "amulet_cleaving");
}

TEST_CASE("only the matching share of the weapon's damage mix grows") {
	PlayerStats stats;
	const DamageMix mace = weaponMix(ItemKind::Mace); // 100% blunt
	const DamageMix sword = weaponMix(ItemKind::ShortSword); // 85% slash
	const DamageMix club = weaponMix(ItemKind::Club); // 85% blunt, 15% slash
	CHECK(stats.TypeMightPercent(mace) == 0); // no amulet
	stats.Wear(amuletBonus(ItemKind::CrushingGrand), true);
	CHECK(stats.TypeMightPercent(mace) == 50);
	CHECK(stats.TypeMightPercent(sword) == 50 * sword[BLUNT] / 100);
	stats.Wear(amuletBonus(ItemKind::CleavingGrand), true);
	CHECK(stats.TypeMightPercent(sword) == 50 * sword[SLASH] / 100);
	CHECK(stats.TypeMightPercent(sword) == 42); // the plan's "about +42%"
	CHECK(stats.TypeMightPercent(club) == 7); // 15% slash of +50%: about +8%
	CHECK(stats.TypeMightPercent(mace) == 0);
	stats.Wear(amuletBonus(ItemKind::PiercingGrand), true);
	CHECK(stats.TypeMightPercent(weaponMix(ItemKind::Spear)) > 0);
	CHECK(stats.TypeMightPercent(mace) == 0);
}

TEST_CASE("a weapon hit lands harder with the matching amulet on, a boss hit without a mix does not") {
	int lost[2] = {};
	for (int worn = 0; worn < 2; worn++) {
		SimWorld world;
		world.player.god = true;
		REQUIRE(world.Load("tests/levels/hitbox_rat"));
		REQUIRE(world.Walk(1.f));
		world.Wait(8000);
		if (worn != 0)
			world.player.stats.Wear(amuletBonus(ItemKind::CrushingGrand), true);
		const Monster* rat = world.dungeon.NearestMonster();
		REQUIRE(rat != nullptr);
		const int before = rat->Health();
		REQUIRE(world.dungeon.AttackNearest(2, {100, 0, 0}, 1.f, 1));
		lost[worn] = before - rat->Health();
	}
	CHECK(lost[0] > 0);
	CHECK(lost[1] > lost[0]); // 2 -> 3 before the rat's own resistance
}

TEST_CASE("every amulet type has a journal note, written the first time one of its tiers is found") {
	CHECK(FIELD_NOTE_COUNT == static_cast<int>(FieldNote::AmuletLast) + 1);
	for (int t = 0; t < AMULET_TYPE_COUNT; t++) {
		CAPTURE(t);
		const FieldNoteText& note = fieldNoteText(amuletNote(static_cast<AmuletType>(t)));
		CHECK(std::string(note.title).size() > 3);
		CHECK(std::string(note.text).size() > 40);
	}
	ItemBag bag;
	Journal journal;
	bag.Find(ItemKind::CleavingMinor, journal);
	REQUIRE(journal.Notes().size() == 1);
	CHECK(journal.Notes()[0] == amuletNote(AmuletType::Cleaving));
	bag.Find(ItemKind::CleavingGrand, journal); // the same type: nothing new
	CHECK(journal.Notes().size() == 1);
	bag.Find(ItemKind::RegenerationNormal, journal);
	REQUIRE(journal.Notes().size() == 2);
	CHECK(journal.Notes()[1] == amuletNote(AmuletType::Regeneration));
	CHECK(bag.Count(ItemKind::CleavingGrand) == 1);
}
