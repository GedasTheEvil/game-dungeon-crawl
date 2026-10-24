// The bite's reach (plan monster-attack-reach): a cone from the mouth along the way the monster bites, checked when the
// bite lands. A jump over a low monster clears it, over a tall one it does not; behind its back the player is out of
// reach until it turns; a boss reaches farther than its kin.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include <algorithm>

namespace {
constexpr int ROW = 1; // tests/levels/empty_row: the player's row
constexpr float JUMP_HIGH = 0.3f; // tiles up: most of a jump (it peaks at ~0.4)

// A monster of the kind standing in col 3 of tests/levels/empty_row, linked to the world's player; not on the level.
struct Placed {
	Monster mon;
	Placed(SimWorld& world, int kind) {
		REQUIRE(world.Load("tests/levels/empty_row"));
		const MonsterLinks links{&world.player, &world.journal, &world.events, 1, &world.random.gameplay};
		mon.Spawn(SimWorld::monsterTypes()[static_cast<size_t>(kind)], 3, ROW, links, world.random.effects);
	}
};

// The player's x with their box gap tiles in front of the monster's mouth, the way dir.
float inFront(const SimWorld& world, const Monster& mon, int dir, float gap) {
	const float half = world.player.HalfWidth();
	return dir > 0 ? mon.Right() + gap + half : mon.Left() - gap - half;
}

const Monster* find(const SimWorld& world, int type) {
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active() && world.dungeon.Monsters()[i].Type()->id == type)
			return &world.dungeon.Monsters()[i];
	return nullptr;
}

bool noted(const SimWorld& world, FieldNote note) {
	return std::any_of(world.said.begin(), world.said.end(), [note](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Note && e.note == note;
	});
}
} // namespace

TEST_CASE("a rat bites the standing player in front of it, not one jumping over it or behind its back") {
	SimWorld world;
	Placed rat(world, MonsterRat);
	const Monster& mon = rat.mon;
	const auto row = static_cast<float>(ROW);
	for (int dir : {-1, 1}) {
		CHECK(mon.Reaches(inFront(world, mon, dir, MONSTER_BITE_REACH), row, dir));
		CHECK(mon.Reaches(inFront(world, mon, dir, mon.Reach().length - 0.01f), row, dir));
		CHECK_FALSE(mon.Reaches(inFront(world, mon, dir, mon.Reach().length + 0.01f), row, dir));
		CHECK_FALSE(mon.Reaches(inFront(world, mon, dir, MONSTER_BITE_REACH), row + JUMP_HIGH, dir));
		CHECK_FALSE(mon.Reaches(mon.CentreX(), row + JUMP_HIGH, dir)); // right above it
		CHECK_FALSE(mon.Reaches(inFront(world, mon, -dir, MONSTER_BITE_REACH), row, dir)); // behind its back
	}
	CHECK(mon.Reaches(mon.CentreX(), row, 0)); // inside its body: no safe spot
}

TEST_CASE("a jump over a tall monster does not clear its blow") {
	SimWorld world;
	for (int kind : {MonsterMummy, MonsterAnubis, MonsterHippo}) {
		CAPTURE(kind);
		Placed tall(world, kind);
		const Monster& mon = tall.mon;
		const float high = static_cast<float>(ROW) + JUMP_HIGH;
		CHECK(mon.Reaches(mon.CentreX(), high, 1));
		CHECK(mon.Reaches(inFront(world, mon, 1, MONSTER_BITE_REACH), high, 1));
	}
}

TEST_CASE("a boss's bite reaches farther and wider than its kin's") {
	SimWorld world;
	for (int kind = 1; kind <= MONSTER_TYPE_MAX; kind++) {
		const MonsterType& type = SimWorld::monsterTypes()[static_cast<size_t>(kind)];
		if (!type.isBoss())
			continue;
		CAPTURE(type.label);
		Placed boss(world, kind);
		Placed kin(world, type.kin);
		CHECK(boss.mon.Reach().length > kin.mon.Reach().length);
		CHECK(boss.mon.Reach().halfAngleDeg >= kin.mon.Reach().halfAngleDeg);
		CHECK(boss.mon.Reach().halfAngleDeg >= BOSS_MELEE_REACH_HALF_ANGLE);
	}
}

TEST_CASE("a jump as the rat starts its bite: it snaps at the air, then turns and bites") {
	SimWorld world; // tests/levels/hitbox_rat: the rat walks up to the player from the right
	REQUIRE(world.Load("tests/levels/hitbox_rat"));
	world.Tick();
	const Monster* rat = find(world, MonsterRat);
	REQUIRE(rat != nullptr);
	for (int t = 0; t < 10000 && !rat->Biting(); t += UPDATE_TICK_MS)
		world.Tick();
	REQUIRE(rat->Biting());
	const int hp = world.Hp();
	world.facing = 1;
	world.Jump();
	while (rat->Biting())
		world.Tick();
	CHECK(world.Hp() == hp);
	CHECK(noted(world, FieldNote::JumpedBite));
	while (world.player.jump.jumping || world.player.jump.falling)
		world.Tick();
	CHECK(world.X() > rat->CentreX()); // over it, behind its back
	world.Wait(3000); // no immunity: it turns
	CHECK(world.Hp() < hp);
}
