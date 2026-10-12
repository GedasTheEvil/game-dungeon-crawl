// Curses (docs/plan/solved/curses.md): timed debuffs from monster hits, several at once, no stacking, no cure.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/world/curses.h"
#include "../../src/world/monster_kinds.h"
#include "../../src/ui/player_hud.h"
#include <sstream>
#include <string>

namespace {
PlayerStats freshStats() {
	GameClock::enableVirtual();
	return PlayerStats{};
}
} // namespace

TEST_CASE("a curse runs 2 minutes; laid again it restarts, no stacking; a greater grade raises a lesser one") {
	Curses curses;
	CHECK_FALSE(curses.Any());
	CHECK(curses.Lay(Curse::Weakness, CurseGrade::Lesser));
	CHECK(curses.LeftMs(Curse::Weakness) == CURSE_MS);
	CHECK(curses.Advance(60000) == 0);
	CHECK(curses.LeftMs(Curse::Weakness) == 60000);
	CHECK_FALSE(curses.Lay(Curse::Weakness, CurseGrade::Lesser)); // running already
	CHECK(curses.LeftMs(Curse::Weakness) == CURSE_MS);
	CHECK_FALSE(curses.Greater(Curse::Weakness));
	curses.Lay(Curse::Weakness, CurseGrade::Greater);
	CHECK(curses.Greater(Curse::Weakness));
	curses.Lay(Curse::Weakness, CurseGrade::Lesser); // only the time
	CHECK(curses.Greater(Curse::Weakness));

	curses.Lay(Curse::Slowness, CurseGrade::Lesser); // two at once
	CHECK(curses.Has(Curse::Weakness));
	CHECK(curses.Has(Curse::Slowness));
	const unsigned ended = curses.Advance(CURSE_MS);
	CHECK(ended == ((1U << static_cast<unsigned>(Curse::Weakness)) | (1U << static_cast<unsigned>(Curse::Slowness))));
	CHECK_FALSE(curses.Any());
	CHECK(curses.Advance(1000) == 0);
}

TEST_CASE("curses go through a save; an old save has none") {
	Curses curses;
	curses.Lay(Curse::Disease, CurseGrade::Greater);
	curses.Lay(Curse::Palsy, CurseGrade::Lesser);
	curses.Advance(5000);
	std::stringstream file;
	curses.Dump(file);
	file << "NEXT";
	Curses loaded;
	loaded.Lay(Curse::Weakness, CurseGrade::Lesser);
	loaded.LoadDump(file);
	CHECK_FALSE(loaded.Has(Curse::Weakness));
	CHECK(loaded.Greater(Curse::Disease));
	CHECK(loaded.LeftMs(Curse::Disease) == CURSE_MS - 5000);
	CHECK(loaded.Has(Curse::Palsy));
	std::string next;
	file >> next;
	CHECK(next == "NEXT");

	std::stringstream old("NEXT");
	loaded.LoadDump(old);
	CHECK_FALSE(loaded.Any());
	old >> next;
	CHECK(next == "NEXT");
}

TEST_CASE("weakness cuts Might, greater weakness past zero; clumsiness cuts the whole hit; a hit deals at least 1") {
	PlayerStats stats = freshStats();
	WorldEvents events;
	stats.AddMight(4);
	CHECK(stats.Damage(20) == 24);
	stats.LayCurse(Curse::Weakness, CurseGrade::Lesser, events);
	CHECK(stats.CurrentMight() == 2);
	CHECK(stats.Damage(20) == 22);
	stats.LayCurse(Curse::Weakness, CurseGrade::Greater, events);
	CHECK(stats.CurrentMight() == -1);
	CHECK(stats.Damage(20) == 19);
	CHECK(stats.Damage(0) == 1);
	stats.EndCurses();
	stats.LayCurse(Curse::Clumsiness, CurseGrade::Lesser, events);
	CHECK(stats.Damage(20) == 19); // 24 less a fifth
	stats.LayCurse(Curse::Clumsiness, CurseGrade::Greater, events);
	CHECK(stats.Damage(20) == 12);
	CHECK(stats.Damage(-4) == 1);
}

TEST_CASE("vulnerability cuts the armour; disease the max HP, the HP keeping its share both ways") {
	PlayerStats stats = freshStats();
	WorldEvents events;
	stats.AddArmor(10);
	stats.LayCurse(Curse::Vulnerability, CurseGrade::Lesser, events);
	CHECK(stats.CurrentArmor() == 4);

	stats.LoseHP(25); // 25 of 50
	stats.LayCurse(Curse::Disease, CurseGrade::Lesser, events);
	CHECK(stats.CurrentMaxHP() == 35);
	CHECK(stats.CurrentHP() == 18); // half, rounded
	stats.LayCurse(Curse::Disease, CurseGrade::Greater, events);
	CHECK(stats.CurrentMaxHP() == 15);
	stats.AdvanceCurses(CURSE_MS, events);
	CHECK(stats.CurrentMaxHP() == 50);
	CHECK(stats.CurrentArmor() == 10);
	CHECK(stats.CurrentHP() >= 24); // the share, give or take the rounding of three changes
	CHECK(stats.CurrentHP() <= 27);
}

TEST_CASE("fatigue drops the stamina to a fifth and stops it coming back; a potion still works") {
	PlayerStats stats = freshStats();
	WorldEvents events;
	stats.LayCurse(Curse::Fatigue, CurseGrade::Lesser, events);
	CHECK(stats.Stamina() == 20);
	for (int i = 0; i < 10; i++) {
		GameClock::advance(1000);
		stats.UpdateStamina(events);
	}
	CHECK(stats.Stamina() == 20);
	stats.AddStamina(30);
	CHECK(stats.Stamina() == 50);
	stats.AdvanceCurses(CURSE_MS, events);
	for (int i = 0; i < 3; i++) {
		GameClock::advance(1000);
		stats.UpdateStamina(events);
	}
	CHECK(stats.Stamina() > 50);

	stats.SetStamina(5); // under a fifth: not raised
	stats.LayCurse(Curse::Fatigue, CurseGrade::Lesser, events);
	CHECK(stats.Stamina() == 5);
}

TEST_CASE("slowness halves the walk and sprints at 1.25 tiles a second") {
	PlayerStats stats = freshStats();
	WorldEvents events;
	stats.SetSprintRequested(true);
	CHECK(stats.WalkCurseFactor() == 1.f);
	CHECK(stats.SprintMoveMultiplier() == 3.f);
	stats.LayCurse(Curse::Slowness, CurseGrade::Lesser, events);
	CHECK(stats.WalkCurseFactor() == 0.5f);
	CHECK(stats.WalkCurseFactor() * stats.SprintMoveMultiplier() == doctest::Approx(1.25f));
}

TEST_CASE("a curse tells the player on start, again and at the end, and writes its note once") {
	PlayerStats stats = freshStats();
	WorldEvents events;
	stats.LayCurse(Curse::Weakness, CurseGrade::Lesser, events);
	stats.LayCurse(Curse::Weakness, CurseGrade::Lesser, events);
	stats.AdvanceCurses(CURSE_MS, events);
	std::vector<std::string> lines;
	int notes = 0;
	for (const WorldEvent& e : events.Take()) {
		if (e.kind == WorldEvent::Kind::Status)
			lines.push_back(e.text);
		if (e.kind == WorldEvent::Kind::Note && e.note == FieldNote::CurseWeakness)
			notes++;
	}
	REQUIRE(lines.size() == 3);
	CHECK(lines[0] == curseDef(Curse::Weakness).startLine);
	CHECK(lines[1] == curseDef(Curse::Weakness).againLine);
	CHECK(lines[2] == curseDef(Curse::Weakness).endLine);
	CHECK(notes >= 1);
	CHECK(curseNote(Curse::Insanity) == FieldNote::CurseInsanity);
}

TEST_CASE("a hit rolls the entries in order, one curse a hit; death and a new game end them all") {
	GameClock::enableVirtual();
	Player player;
	WorldEvents events;
	Rng rng(1);
	const CurseEntries sure = {CurseEntry{Curse::Weakness, CurseGrade::Greater, 100},
							   CurseEntry{Curse::Slowness, CurseGrade::Lesser, 100}};
	CHECK(player.Curse(sure, events, rng));
	CHECK(player.stats.curses.Greater(Curse::Weakness));
	CHECK_FALSE(player.stats.curses.Has(Curse::Slowness));
	const CurseEntries none{};
	CHECK_FALSE(player.Curse(none, events, rng));

	player.stats.LayCurse(Curse::Disease, CurseGrade::Lesser, events);
	player.TakeHit(10000, DamageMix{100, 0, 0}, events, true);
	REQUIRE_FALSE(player.Alive());
	CHECK_FALSE(player.stats.curses.Any());
	CHECK_FALSE(player.Curse(sure, events, rng)); // the dead are not cursed
	player.Reanimate();
	player.stats.LayCurse(Curse::Disease, CurseGrade::Lesser, events);
	player.Reanimate();
	CHECK_FALSE(player.stats.curses.Any());
	CHECK(player.stats.CurrentHP() == player.stats.CurrentMaxHP());
	CHECK(player.stats.CurrentMaxHP() == 50);
}

TEST_CASE("the monsters' curse lists: the strongest first, a boss above its kin, each curse has text and an icon") {
	for (int t = 0; t < 64; t++) {
		const MonsterKind* kind = monsterKind(t);
		if (kind == nullptr)
			continue;
		for (const CurseEntries* list : {&kind->meleeCurses, &kind->rangedCurses}) {
			const CurseEntries& l = *list;
			if (l[1].percent > 0)
				CHECK(l[0].percent > 0);
			if (l[0].percent > 0 && l[1].percent > 0 && l[0].kind == l[1].kind)
				CHECK(l[0].grade == CurseGrade::Greater); // the greater, rarer one first
		}
		const MonsterKind* kin = kind->isBoss() ? monsterKind(kind->kin) : nullptr;
		if (kin != nullptr && kin->meleeCurses[0].percent > 0)
			CHECK(kind->meleeCurses.back().percent + kind->meleeCurses.front().percent >
				  kin->meleeCurses.back().percent + kin->meleeCurses.front().percent);
	}
	const MonsterKind* mummy = monsterKind(MonsterMummy);
	REQUIRE(mummy != nullptr);
	CHECK(mummy->meleeCurses[1].kind == Curse::Weakness);
	CHECK(mummy->meleeCurses[1].percent == 10);
	CHECK(monsterKind(MonsterAnubis)->rangedCurses[0].kind == Curse::Palsy);
	for (const CurseDef& def : CURSES) {
		CHECK(std::string(def.name).size() > 0);
		CHECK(std::string(def.startLine).size() > 0);
		CHECK(std::string(def.endLine).size() > 0);
	}
	CHECK(static_cast<int>(PlayerHud::Icon::FirstCurse) + CURSE_COUNT <= 8 * 5); // the atlas grid
}

TEST_CASE("a mummy's blows curse the player with weakness now and then, and the journal sees it") {
	SimWorld world; // tests/levels/mummy: the mummy at col 10, in its coffin
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/mummy"));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(9.1f));
	for (int t = 0; t < 6000 && !world.player.stats.curses.Has(Curse::Weakness); t++)
		world.Tick();
	CHECK(world.player.stats.curses.Has(Curse::Weakness));
	CHECK(world.Saw(MonsterMummy, CreatureMove::Curse));
}

TEST_CASE("the Anubis's bolt lays palsy now and then, and the journal sees it") {
	SimWorld
		world; // tests/levels/anubis_bolt: the Anubis shoots from afar, then comes on to hit; seeds until one lays it
	world.player.god = true;
	for (uint64_t seed = 1; seed < 40 && !world.player.stats.curses.Has(Curse::Palsy); seed++) {
		REQUIRE(world.Load("tests/levels/anubis_bolt", seed));
		for (int t = 0; t < 1500 && !world.player.stats.curses.Has(Curse::Palsy); t++)
			world.Tick();
	}
	CHECK(world.player.stats.curses.Has(Curse::Palsy));
	CHECK(world.Saw(MonsterAnubis, CreatureMove::Curse));
}
