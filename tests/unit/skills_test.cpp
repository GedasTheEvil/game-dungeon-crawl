// Heka and the skills (docs/plan/skills.md): a boss gives one heka, once; learning spends it; saves keep both.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/ui/skills_layout.h"
#include "../../src/world/skills.h"
#include <algorithm>
#include <sstream>

namespace {
// The boss in play slain; the next update sees it dead.
void killBoss(SimWorld& world) {
	REQUIRE(world.dungeon.Boss() != nullptr);
	world.dungeon.HurtBoss(world.dungeon.BossHealth());
	world.Tick();
}
} // namespace

TEST_CASE("a boss level gives one heka, only the first time") {
	Heka heka;
	CHECK(heka.Earn(5));
	CHECK_FALSE(heka.Earn(5));
	CHECK(heka.Earn(10));
	CHECK(heka.Earned() == 2);
	CHECK(heka.Free() == 2);
}

TEST_CASE("learning spends a heka; no heka, learnt already or sealed: nothing") {
	Heka heka;
	CHECK_FALSE(heka.Learn(SkillId::SharpEye, true)); // no heka
	heka.Earn(5);
	CHECK(heka.Learn(SkillId::SharpEye, true));
	CHECK(heka.Has(SkillId::SharpEye));
	CHECK(heka.Free() == 0);
	CHECK(heka.Spent() == 1);
	heka.Earn(10);
	CHECK_FALSE(heka.Learn(SkillId::SharpEye, true)); // learnt already
	CHECK(heka.Free() == 1);
	for (int s = 0; s < SKILL_COUNT; s++) {
		const auto id = static_cast<SkillId>(s);
		if (SKILLS[static_cast<size_t>(s)].sealed && !heka.Has(id)) {
			CHECK_FALSE(heka.CanLearn(id));
			CHECK_FALSE(heka.Learn(id));
		}
	}
	CHECK(heka.Free() == 1);
}

TEST_CASE("heka and the skills learnt go through a save; an old save has none") {
	Heka heka;
	heka.Earn(5);
	heka.Earn(10);
	heka.Learn(SkillId::Dodge, true);
	std::stringstream file;
	heka.Dump(file);
	file << "NEXT";
	Heka loaded;
	loaded.Earn(15);
	loaded.LoadDump(file);
	CHECK(loaded.Earned() == 2);
	CHECK(loaded.Has(SkillId::Dodge));
	CHECK(loaded.Free() == 1);
	CHECK_FALSE(loaded.Earn(10));
	std::string next;
	file >> next;
	CHECK(next == "NEXT");

	std::stringstream old("NEXT");
	loaded.LoadDump(old);
	CHECK(loaded.Earned() == 0);
	old >> next;
	CHECK(next == "NEXT"); // the stream is where it was
}

TEST_CASE("every skill has a name, an effect and its lore; the heka mark and the icons fit the atlas") {
	for (const SkillDef& def : SKILLS) {
		CHECK(std::string(def.name).size() > 0);
		CHECK(std::string(def.effect).size() > 0);
		CHECK(std::string(def.lore).size() > 0);
	}
	CHECK(skillIcon(static_cast<SkillId>(SKILL_COUNT - 1)) < SKILL_ICON_COLUMNS * SKILL_ICON_ROWS);
	CHECK(HEKA_ICON < SKILL_ICON_FIRST);
}

TEST_CASE("the slain boss gives a heka and the journal's Heka note; after a load it gives none again") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/sobek"));
	REQUIRE(world.WalkTo(10.8f));
	for (int t = 0; t < 400 && world.dungeon.Boss() == nullptr; t++)
		world.Tick();
	world.SaveAndLoad(); // before the kill
	for (int t = 0; t < 400 && world.dungeon.Boss() == nullptr; t++)
		world.Tick();
	killBoss(world);
	CHECK(world.player.stats.heka.Free() == 1);
	CHECK(world.Told("heka"));
	CHECK(std::ranges::any_of(
		world.said, [](const WorldEvent& e) { return e.kind == WorldEvent::Kind::Note && e.note == FieldNote::Heka; }));
	REQUIRE(world.player.stats.heka.Learn(SkillId::Stun, true));
	world.SaveAndLoad();
	CHECK(world.player.stats.heka.Has(SkillId::Stun));
	CHECK(world.player.stats.heka.Free() == 0);
	CHECK(world.player.stats.heka.Earn(world.dungeon.LevelNumber()) == false); // this level's heka is taken
}

TEST_CASE("the Heka screen's cards and button fit their panels, apart, clear of the screen tabs") {
	using namespace SkillsLayout;
	const Rect canvas = {0, 0, ui::CANVAS_W, ui::CANVAS_H};
	CHECK(CARDS_PANEL.within(canvas));
	CHECK(DETAIL_PANEL.within(canvas));
	CHECK_FALSE(CARDS_PANEL.overlaps(DETAIL_PANEL));
	CHECK(LEARN_BUTTON.within(DETAIL_PANEL));
	for (int a = 0; a < SKILL_COUNT; a++) {
		const Rect r = cardRect(a);
		CHECK(r.inset(-1.f).within(CARDS_PANEL)); // the selection frame too
		CHECK(r.y + r.h < POINTS_BASELINE - 1.f);
		CHECK(cardAt(r.cx(), r.cy()) == a);
		for (int b = a + 1; b < SKILL_COUNT; b++)
			CHECK_FALSE(r.overlaps(cardRect(b)));
	}
	CHECK(cardAt(DETAIL_PANEL.cx(), DETAIL_PANEL.cy()) == -1);
	for (int t = 0; t < ui::SCREEN_TAB_COUNT; t++) {
		const Rect tab = ui::screenTabRect(t);
		CHECK_FALSE(tab.overlaps(CARDS_PANEL));
		CHECK_FALSE(tab.overlaps(DETAIL_PANEL));
		CHECK(tab.x > ui::CANVAS_W / 2 + ui::SCREEN_TABS_TITLE_REACH + 1.f); // the title rule ends before the strip
	}
}

TEST_CASE("Dodge: about 1 attack in 10 misses, none without the skill") {
	Player player;
	WorldEvents events;
	Rng rng(7);
	int dodged = 0;
	for (int i = 0; i < 10000; i++)
		dodged += player.Dodges(events, rng) ? 1 : 0;
	CHECK(dodged == 0);
	CHECK(events.Take().empty());

	player.stats.heka.Earn(5);
	REQUIRE(player.stats.heka.Learn(SkillId::Dodge, true));
	for (int i = 0; i < 10000; i++)
		dodged += player.Dodges(events, rng) ? 1 : 0;
	CHECK(dodged > 900);
	CHECK(dodged < 1100);
	const auto said = events.Take();
	CHECK(said.size() == static_cast<size_t>(dodged) * 2); // the line and the whoosh
}

TEST_CASE("Dodge: a plant's bites are sometimes dodged") {
	SimWorld world; // tests/levels/idle_monsters: the plant bites, poisoning weakly on 30% of its bites
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/idle_monsters"));
	world.player.stats.heka.Earn(5);
	REQUIRE(world.player.stats.heka.Learn(SkillId::Dodge, true));
	world.WaitTicks(30);
	REQUIRE(world.Walk(3.6f));
	REQUIRE(world.Walk(0.25f));
	REQUIRE(world.Walk(7.4f));
	world.Wait(40000); // some 50 bites
	CHECK(std::ranges::count_if(world.said, [](const WorldEvent& e) {
			  return e.kind == WorldEvent::Kind::Status && e.text == "Dodged";
		  }) > 0);
}
