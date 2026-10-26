// The giant rat's pack call (plan giant-rat-pack-call): hurt below half its HP, it squeals once and the rats within
// 8 tiles come; the farther ones do not.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include <vector>

namespace {
// tests/levels/giant_rat_call: the giant rat on the lower floor, the rats on the upper one (no ladder, never on the
// player's row, so only the call alerts them). The monsters of a type, in level order.
std::vector<const Monster*> monstersOf(const SimWorld& world, int type) {
	std::vector<const Monster*> found;
	for (int i = 0; i < MAX_MONSTERS; i++) {
		const Monster& mon = world.dungeon.Monsters()[i];
		if (mon.Active() && mon.Type()->id == type)
			found.push_back(&mon);
	}
	return found;
}

// The giant rat's squeals since the level loaded.
int squeals(SimWorld& world) {
	int n = 0;
	for (const WorldEvent& event : world.said)
		if (event.kind == WorldEvent::Kind::CharacterSound && event.character == CharacterSound::Roar &&
			event.who == MonsterGiantRat)
			n++;
	return n;
}
} // namespace

TEST_CASE("the giant rat's call: it squeals once below half its HP, the rats within 8 tiles come, the others do not") {
	const MonsterKind& kind = *monsterKind(MonsterGiantRat);
	REQUIRE(kind.call.has_value());
	CHECK(kind.call->radius == 8.f);
	CHECK(kind.call->belowPct == 50);
	CHECK(monsterKind(MonsterRat)->answersCall);
	CHECK_FALSE(monsterKind(MonsterRat)->call.has_value());

	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/giant_rat_call"));
	world.Wait(500);
	const std::vector<const Monster*> rats = monstersOf(world, MonsterRat); // cols 4, 7 (near), 11 (far)
	const std::vector<const Monster*> giants = monstersOf(world, MonsterGiantRat); // walled in at col 2
	REQUIRE(rats.size() == 3);
	REQUIRE(giants.size() == 1);
	Monster& giant = const_cast<Monster&>(*giants[0]);
	for (const Monster* rat : rats)
		CHECK_FALSE(rat->Alerted());
	CHECK(squeals(world) == 0);

	const DamageMix pierce = {0, 0, 100};
	const auto hit = [&](int dmg) {
		giant.TakeWeaponHit(dmg, pierce);
		world.Wait(100);
	};
	hit(kind.maxHealth / 4); // 75%
	CHECK(squeals(world) == 0);
	for (const Monster* rat : rats)
		CHECK_FALSE(rat->Alerted()); // not hurt enough yet
	hit(kind.maxHealth / 4 + 2); // below 50%
	REQUIRE(giant.Alive());
	CHECK(giant.Health() * 2 < kind.maxHealth);
	CHECK(squeals(world) == 1);
	CHECK(rats[0]->Alerted()); // 2.8 tiles away
	CHECK(rats[1]->Alerted()); // 5.4
	CHECK_FALSE(rats[2]->Alerted()); // 9.0 and more
	CHECK(world.Saw(MonsterGiantRat, CreatureMove::CallPack));

	hit(1); // once only
	CHECK(squeals(world) == 1);
}
