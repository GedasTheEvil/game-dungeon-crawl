#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/world/climb_path.h"
#include <cmath>

// Climbers (docs/plan/monster-climbers.md): the path on the level's cells, and the climbers following it.

namespace {
const Monster* firstMonster(const SimWorld& world) {
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active())
			return &world.dungeon.Monsters()[i];
	return nullptr;
}

// The player from the entrance (left of the ladder at col 6) down to the lower floor, right until the monster has
// seen them, back up and left to x.
void showAndFlee(SimWorld& world, float seenAt, float fleeTo) {
	REQUIRE(world.WalkTo(6.45f));
	REQUIRE(world.Climb(-1.97f));
	REQUIRE(world.WalkTo(seenAt));
	world.Wait(500);
	REQUIRE(world.WalkTo(6.45f));
	REQUIRE(world.Climb(2.f));
	REQUIRE(world.WalkTo(fleeTo));
}

LevelGrid grid(const char* path) {
	LevelGrid g{};
	REQUIRE(loadLevelFile(path, g).empty());
	return g;
}
} // namespace

TEST_CASE("the climb path runs along the floors and up the ladder, a leaper's over the spikes") {
	const LevelGrid g = grid("tests/levels/climb_up");
	int ladderCol = -1, top = -1, foot = LEVEL_HEIGHT;
	for (int row = 0; row < LEVEL_HEIGHT; row++)
		for (int col = 0; col < LEVEL_WIDTH; col++)
			if (g.at(col, row).type == Ladder) {
				ladderCol = col;
				top = std::max(top, row);
				foot = std::min(foot, row);
			}
	REQUIRE(ladderCol >= 0);
	REQUIRE(top == foot + 2);

	ClimbMap walker;
	walker.Build(g.cells, ladderCol - 4, top, ClimbAbility{}, CLIMB_PATH_MAX);
	CHECK(walker.Steps(ladderCol + 6, foot) == 6 + 2 + 4); // along the lower floor, up two rungs, along the top
	int col = 0, row = 0;
	REQUIRE(walker.Next(ladderCol, foot, col, row));
	CHECK(col == ladderCol);
	CHECK(row == foot + 1);
	CHECK(walker.Steps(ladderCol + 8, top) == -1); // past the spikes: a coward does not step on them

	ClimbMap leaper;
	leaper.Build(g.cells, ladderCol + 8, top, ClimbAbility{false, true}, CLIMB_PATH_MAX);
	CHECK(leaper.Steps(ladderCol + 6, foot) > 0);
	REQUIRE(leaper.Next(ladderCol + 3, top, col, row));
	CHECK(col == ladderCol + 5); // over the spikes in one leap

	ClimbMap capped;
	capped.Build(g.cells, ladderCol - 4, top, ClimbAbility{}, 5);
	CHECK(capped.Steps(ladderCol + 6, foot) == -1);
}

TEST_CASE("a rat follows the player up the ladder") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/climb_up"));
	world.player.god = true;
	const auto upper = static_cast<int>(world.Y());
	showAndFlee(world, 9.f, 2.5f);
	const Monster* rat = firstMonster(world);
	REQUIRE(rat != nullptr);
	world.Wait(15000);
	CHECK(world.Saw(MonsterRat, CreatureMove::Climb));
	CHECK(rat->Row() == upper);
	CHECK(rat->CentreX() < 4.f);
	CHECK(!rat->OnRungs());
}

TEST_CASE("a rat follows the player down the ladder") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/climb_down"));
	world.player.god = true;
	const auto upper = static_cast<int>(world.Y());
	REQUIRE(world.WalkTo(6.45f));
	world.Wait(1000); // it has seen the player
	const Monster* rat = firstMonster(world);
	REQUIRE(rat != nullptr);
	REQUIRE(world.Climb(-1.97f));
	REQUIRE(world.WalkTo(15.f));
	world.Wait(15000);
	CHECK(rat->Row() == upper - 2);
	CHECK(rat->CentreX() > 12.f);
}

TEST_CASE("a scarab is no climber: it stays at the foot of the ladder") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/climb_scarab"));
	world.player.god = true;
	const int lower = static_cast<int>(world.Y()) - 2;
	showAndFlee(world, 9.f, 2.5f);
	const Monster* scarab = firstMonster(world);
	REQUIRE(scarab != nullptr);
	world.Wait(15000);
	CHECK(scarab->Row() == lower);
	CHECK(!world.Saw(MonsterScarab, CreatureMove::Climb));
}

TEST_CASE("a giant rat climbs after the player and leaps the spikes on the way") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/climb_leap"));
	world.player.god = true;
	const auto upper = static_cast<int>(world.Y());
	showAndFlee(world, 8.f, 14.5f); // past the spikes (col 10)
	const Monster* rat = firstMonster(world);
	REQUIRE(rat != nullptr);
	world.Wait(15000);
	CHECK(rat->Row() == upper);
	CHECK(rat->CentreX() > 11.f);
	CHECK(world.Saw(MonsterGiantRat, CreatureMove::Leap));
}

TEST_CASE("a climber gives up once the player is out of its path's reach") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/climb_far"));
	world.player.god = true;
	REQUIRE(world.WalkTo(2.45f));
	REQUIRE(world.Climb(-1.97f));
	const auto middle = static_cast<int>(world.Y() + STANDING_EPSILON);
	REQUIRE(world.WalkTo(36.45f));
	REQUIRE(world.Climb(-1.97f));
	world.Wait(1000); // it has seen the player
	const Monster* rat = firstMonster(world);
	REQUIRE(rat != nullptr);
	REQUIRE(world.Climb(2.f));
	REQUIRE(world.WalkTo(2.45f));
	REQUIRE(world.Climb(2.f));
	REQUIRE(world.WalkTo(12.f));
	world.Wait(CLIMB_GIVE_UP_MS + 1000);
	const float stopped = rat->CentreX();
	world.Wait(5000);
	CHECK(rat->Row() == middle); // it came up after the player
	CHECK(rat->CentreX() == stopped);
	CHECK(rat->CentreX() - world.X() > 8.f);
}
