// Seth, the red hippo (plan hippo-boss): the hippo's boss. Lies in the water, charges farther and faster than the
// hippo, a wall stuns him, and as the stun ends he roars a sandstorm that hits, shoves and blinds the player near him;
// hippos dig out beside him; a spear's point goes in, blows sink into the fat.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
const Monster* seth(const SimWorld& world) {
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active() && world.dungeon.Monsters()[i].Type()->id == MonsterSeth)
			return &world.dungeon.Monsters()[i];
	return nullptr;
}

// tests/levels/seth: Seth in col 15 of a pool (cols 10-25), the player in col 9 by the west wall. A player who lives
// through a charge and a roar.
const Monster* loadPool(SimWorld& world) {
	REQUIRE(world.Load("tests/levels/seth"));
	world.player.stats.AddXP(1000000, world.events);
	REQUIRE(world.WalkTo(10.2f));
	world.Tick();
	return seth(world);
}

// Ticks until pred holds, at most ms.
template <typename Pred> bool waitFor(SimWorld& world, int ms, Pred pred) {
	for (int t = 0; t < ms / UPDATE_TICK_MS; t++) {
		if (pred())
			return true;
		world.Tick();
	}
	return pred();
}
} // namespace

TEST_CASE("Seth is the hippo's boss: a longer, faster charge, the roar, hippos, a spear's point goes in") {
	const MonsterKind& s = *monsterKind(MonsterSeth);
	const MonsterKind& hippo = *monsterKind(MonsterHippo);
	CHECK(s.isBoss());
	CHECK(s.kin == MonsterHippo);
	CHECK(s.boss.minion == MonsterHippo);
	CHECK(s.boss.summon == Summon::DigOut);
	CHECK(s.locomotion == Locomotion::Submerged);
	CHECK(s.wading == Wading::Swimmer);
	REQUIRE(s.charge);
	CHECK(s.charge->maxRange > hippo.charge->maxRange);
	CHECK(s.charge->maxRange > monsterKind(MonsterSobek)->charge->maxRange);
	CHECK(s.charge->speed > hippo.charge->speed);
	CHECK(s.charge->speed > monsterKind(MonsterSobek)->charge->speed);
	CHECK(s.roar);
	CHECK(s.maxHealth > monsterKind(MonsterSobek)->maxHealth);
	CHECK(s.resist[static_cast<size_t>(DamageType::Blunt)] == RESISTS);
	CHECK(s.resist[static_cast<size_t>(DamageType::Pierce)] == NORMAL);
}

TEST_CASE("Seth charges through the player into the wall, then roars: a hit, a shove away, sand in the eyes") {
	SimWorld world;
	const Monster* s = loadPool(world);
	REQUIRE(s != nullptr);
	REQUIRE(world.WalkTo(11.6f)); // in the water, in his wake range: no jump over the charge
	CHECK(s->Alerted());
	REQUIRE(waitFor(world, 4000, [&] { return s->Stunned(); }));
	CHECK(world.Saw(MonsterSeth, CreatureMove::Charge));
	CHECK(s->CentreX() < 12.f); // ran on past the player, into the west wall
	REQUIRE(world.WalkTo(s->Right() + 1.f)); // stand by him while he is stunned
	const int hp = world.Hp();
	const float x = world.X();
	REQUIRE(waitFor(world, CHARGE_STUN_MS + 200, [&] { return !s->Stunned(); }));
	CHECK(world.Saw(MonsterSeth, CreatureMove::Roar));
	CHECK(world.dungeon.SandHaze() > 0.5f);
	CHECK(world.Hp() < hp);
	world.Wait(500);
	CHECK(world.X() >= x + SETH_ROAR_SHOVE - 0.05f); // shoved away from him, east
	world.Wait(SETH_ROAR_HAZE_MS);
	CHECK(world.dungeon.SandHaze() == 0.f);
}

TEST_CASE("the roar does not reach a player out of its radius") {
	SimWorld world;
	const Monster* s = loadPool(world);
	REQUIRE(s != nullptr);
	REQUIRE(world.WalkTo(11.6f));
	REQUIRE(waitFor(world, 4000, [&] { return s->Stunned(); }));
	world.player.stats.Drink(potionGain(ItemKind::WaterBlessing)); // a faster wade
	world.HoldWalk(1, CHARGE_STUN_MS - 300); // away while he is stunned
	REQUIRE(world.X() - world.player.HalfWidth() > s->Right() + SETH_ROAR_RADIUS);
	const int hp = world.Hp();
	REQUIRE(waitFor(world, CHARGE_STUN_MS + 200, [&] { return !s->Stunned(); }));
	CHECK(world.dungeon.SandHaze() == 0.f);
	CHECK(world.Hp() == hp);
}

TEST_CASE("hippos dig out beside Seth") {
	SimWorld world;
	const Monster* s = loadPool(world);
	REQUIRE(s != nullptr);
	REQUIRE(world.WalkTo(11.6f));
	REQUIRE(s->Alerted());
	world.Wait(500);
	CHECK(world.dungeon.LivingMinions() >= 1);
}
