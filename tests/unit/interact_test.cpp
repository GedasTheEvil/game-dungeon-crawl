// The interact prompt (plan interact-prompt): the verb of each cell, the key doing exactly what the verb says, the
// mimic waking on the key or a hit only, and the glint over an unread inscription.
#include "../../external/doctest/doctest.h"
#include "../../src/world/decor.h"
#include "sim_world.h"
#include <string>

namespace {
constexpr int ROW = 2; // tests/levels/interact: the corridor's row

// tests/levels/interact with inscription 1's text (inscription 2 has none: too worn to read).
void load(SimWorld& world) {
	world.player.god = true;
	world.inscriptions.Add({1, "Title", {"The text."}, "A note.", {}, "test"});
	REQUIRE(world.Load("tests/levels/interact"));
	world.WaitTicks(30);
}

// A press: what the world said, or did, after it.
size_t press(SimWorld& world) {
	const size_t before = world.said.size();
	world.Interact();
	world.WaitTicks(1);
	return world.said.size() - before;
}
} // namespace

TEST_CASE("the interact verb: Open, then Copy on the chest under the inscription, Pull once, Read a worn one") {
	SimWorld world;
	load(world);
	CHECK(world.dungeon.InteractVerb() == Interaction::None); // the entrance
	REQUIRE(world.WalkTo(3.5f));
	CHECK(world.dungeon.InteractVerb() == Interaction::Open);
	CHECK(press(world) > 0); // the lid creaks
	CHECK(world.dungeon.InteractVerb() == Interaction::None); // its clip plays
	CHECK(press(world) == 0);
	world.Wait(containerOpenMsMax());
	CHECK(world.dungeon.InteractVerb() == Interaction::Take); // the item shows
	CHECK(press(world) > 0);
	CHECK(world.dungeon.InteractVerb() == Interaction::Copy); // no hieroglyph skill
	world.player.stats.heka.Earn(1);
	REQUIRE(world.player.stats.heka.Learn(SkillId::Hieroglyphs, true));
	CHECK(world.dungeon.InteractVerb() == Interaction::Read);
	REQUIRE(world.WalkTo(4.5f));
	CHECK(world.dungeon.InteractVerb() == Interaction::None);
	CHECK(press(world) == 0);
	REQUIRE(world.WalkTo(5.5f));
	CHECK(world.dungeon.InteractVerb() == Interaction::Pull);
	press(world);
	CHECK(leverPulled(world.dungeon.Cell(5, ROW).fixture));
	CHECK(world.dungeon.InteractVerb() == Interaction::None); // pulled: nothing left to do
	CHECK(press(world) == 0);
	REQUIRE(world.WalkTo(7.5f));
	CHECK(world.dungeon.InteractVerb() == Interaction::Read); // too worn: read, whatever the skill
}

TEST_CASE("the interact verb: Enter a teleporter but not in the air, Light only with a bow, Take, Answer, Enter") {
	SimWorld world;
	load(world);
	REQUIRE(world.WalkTo(9.5f));
	CHECK(world.dungeon.InteractVerb() == Interaction::Enter);
	world.Jump();
	world.WaitTicks(3);
	REQUIRE(world.player.jump.jumping);
	CHECK(world.dungeon.InteractVerb() == Interaction::None);
	CHECK(press(world) == 0);
	world.Wait(1500);
	CHECK(world.X() < 12.f); // the press in the air did not teleport
	REQUIRE(world.dungeon.PlaceDecor(14, ROW, DECOR_BRAZIER));
	REQUIRE(world.WalkTo(14.5f));
	CHECK(world.dungeon.InteractVerb() == Interaction::None); // no bow
	CHECK(press(world) == 0);
	world.Give(ItemKind::SelfBow);
	CHECK(world.dungeon.InteractVerb() == Interaction::Light);
	press(world);
	CHECK(world.player.fireArrows == FIRE_ARROW_SHOTS);
	REQUIRE(world.WalkTo(16.5f));
	CHECK(world.dungeon.InteractVerb() == Interaction::Take);
	REQUIRE(world.WalkTo(17.5f));
	CHECK(world.dungeon.InteractVerb() == Interaction::None);
	REQUIRE(world.WalkTo(18.5f));
	CHECK(world.dungeon.InteractVerb() == Interaction::Answer);
	CHECK(press(world) > 0); // the riddle is asked
	CHECK(world.said.back().kind == WorldEvent::Kind::AskRiddle);
}

TEST_CASE("the interact key on a teleporter moves the player to its partner") {
	SimWorld world;
	load(world);
	REQUIRE(world.WalkTo(9.5f));
	world.Interact();
	world.WaitTicks(SimWorld::GATE_TRAVEL_TICKS);
	CHECK(world.X() > 12.f);
	CHECK(world.X() < 13.f);
	CHECK(world.dungeon.InteractVerb() == Interaction::Enter); // and back again
}

TEST_CASE("the verbs have their prompt words") {
	CHECK(std::string(interactionVerb(Interaction::None)).empty());
	CHECK(std::string(interactionVerb(Interaction::Open)) == "Open");
	CHECK(std::string(interactionVerb(Interaction::Light)) == "Light arrows");
	CHECK(std::string(interactionVerb(Interaction::Enter)) == "Enter");
}

TEST_CASE("a mimic does not wake when the player comes near, only on the interact key, shown as Open") {
	SimWorld world; // tests/levels/mimic: the mimic at col 8
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/mimic", 15));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(7.6f));
	world.Wait(2000);
	CHECK(world.dungeon.MonsterBarsShown() == 0); // still a chest
	CHECK(world.dungeon.InteractVerb() == Interaction::None); // not in its cell yet
	REQUIRE(world.WalkTo(8.5f));
	world.Wait(1000);
	CHECK(world.dungeon.MonsterBarsShown() == 0);
	CHECK(world.dungeon.InteractVerb() == Interaction::Open);
	world.Interact();
	world.WaitTicks(5);
	CHECK(world.dungeon.MonsterBarsShown() == 1);
	CHECK(world.dungeon.InteractVerb() == Interaction::None); // awake: no chest left to open
}

TEST_CASE("a hit wakes a mimic lying in wait") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/mimic", 15));
	world.WaitTicks(30);
	world.Equip(ItemKind::Spear);
	REQUIRE(world.WalkTo(7.9f));
	world.WaitTicks(5);
	REQUIRE(world.dungeon.MonsterBarsShown() == 0);
	REQUIRE(world.dungeon.InteractVerb() == Interaction::None); // not opened: struck
	REQUIRE(world.Attack());
	world.Wait(1000);
	CHECK(world.dungeon.MonsterBarsShown() == 1);
}

TEST_CASE("an inscription glints until it is read; a copy glints dimmer until translated, a worn one never") {
	SimWorld world;
	load(world);
	CHECK(world.dungeon.InscriptionGlint(3, ROW) == 1.f);
	CHECK(world.dungeon.InscriptionGlint(7, ROW) == 0.f); // too worn to read
	CHECK(world.dungeon.InscriptionGlint(5, ROW) == 0.f); // a lever
	REQUIRE(world.WalkTo(3.5f));
	world.OpenChest();
	world.WaitTicks(1);
	CHECK(world.dungeon.InscriptionGlint(3, ROW) == 1.f);
	press(world); // copied
	const float copy = world.dungeon.InscriptionGlint(3, ROW);
	CHECK(copy > 0.f);
	CHECK(copy < 1.f);
	world.player.stats.heka.Earn(1);
	REQUIRE(world.player.stats.heka.Learn(SkillId::Hieroglyphs, true));
	press(world); // read now
	CHECK(world.dungeon.InscriptionGlint(3, ROW) == 0.f);
}
