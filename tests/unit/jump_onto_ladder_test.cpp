// Jump onto a ladder (plan jump-onto-ladder): up or down pressed in a jump on its way down through a ladder cell
// catches the rungs; the jump ends there and the press climbs. No climb against a jump's rise or fall.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/world/climb_path.h"

namespace {
// tests/levels/climb_up: a ladder (col 6) from the lower floor up two rows to the upper one.
constexpr int LADDER_COL = 6;

int lowerRow(SimWorld& world) { return static_cast<int>(world.Y()) - 2; } // from the start on the upper floor

// The up key held for one tick, as stepHeldWalk does.
void holdUp(SimWorld& world) {
	world.dungeon.Move(0.f, PLAYER_FORWARD_MOVE_STEP);
	world.Tick();
}
} // namespace

TEST_CASE("jump onto a ladder: up held through a jump into the ladder's cell catches the rungs on the way down") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/climb_up"));
	REQUIRE(world.dungeon.PlacePlayer(LADDER_COL - 1, lowerRow(world)));
	REQUIRE(world.WalkTo(static_cast<float>(LADDER_COL) - 0.3f));
	const float floorY = world.Y();
	world.Jump();
	REQUIRE(world.player.jump.jumping);
	int ticks = 0;
	while (world.player.jump.jumping && ticks++ < 100) {
		const float velocity = world.player.jump.velocity;
		holdUp(world);
		if (velocity > 0.f) // rising: the jump's, no climb on top
			CHECK(world.player.jump.jumping);
	}
	REQUIRE_FALSE(world.player.jump.jumping);
	CHECK(world.Y() > floorY + 0.1f); // caught in the air, no snap back to the take-off
	CHECK(static_cast<int>(world.X()) == LADDER_COL);
	CHECK(world.dungeon.PlayerOnLadder());

	float last = world.Y();
	for (int i = 0; i < 40; i++) { // climbs on steadily: no drop, no shake
		holdUp(world);
		CHECK(world.Y() >= last);
		last = world.Y();
	}
	CHECK(last > floorY + 0.5f);
	CHECK_FALSE(world.player.jump.jumping);
	CHECK_FALSE(world.player.jump.falling);
}

TEST_CASE("jump onto a ladder: without up or down the jump goes on through the ladder's cell") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/climb_up"));
	REQUIRE(world.dungeon.PlacePlayer(LADDER_COL - 1, lowerRow(world)));
	REQUIRE(world.WalkTo(static_cast<float>(LADDER_COL) - 0.3f));
	const float floorY = world.Y();
	world.Jump();
	world.Wait(1200);
	CHECK_FALSE(world.player.jump.jumping);
	CHECK(world.Y() == doctest::Approx(floorY));
}

TEST_CASE("jump onto a ladder: a jump off the rungs with up held rises first, the hands catch it coming down") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/climb_up"));
	REQUIRE(world.dungeon.PlacePlayer(LADDER_COL, lowerRow(world)));
	REQUIRE(world.Climb(0.6f));
	REQUIRE(world.dungeon.PlayerOnLadder());
	const float from = world.Y();
	world.facing = 1;
	world.Jump();
	REQUIRE(world.player.jump.jumping);
	float top = from;
	int rising = 0;
	while (world.player.jump.velocity > 0.f && rising++ < 100) {
		holdUp(world);
		CHECK(world.player.jump.jumping); // no instant re-grab
		top = std::max(top, world.Y());
	}
	CHECK(top > from + 0.2f); // the jump's rise, not a climb
	for (int i = 0; i < 100 && world.player.jump.jumping; i++)
		holdUp(world);
	CHECK_FALSE(world.player.jump.jumping);
	CHECK(world.dungeon.PlayerOnLadder()); // still over the ladder's cell: caught
}

namespace {
BodyFit bodyOf(int type) { return {monsterKind(type)->heightQuarters, monsterKind(type)->stepQuarters}; }

const Monster* firstMonster(const SimWorld& world) {
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active())
			return &world.dungeon.Monsters()[i];
	return nullptr;
}

// The level's monster (col 3, lower floor) sees the player beside it and comes; then the player is up on the upper
// floor, right of the ladder. The ticks until the monster stands on the upper floor (-1: never in 20 s); caught: the
// first tick it climbed was in the air over the ladder's middle (no walk to the rungs, no pop to the floor).
struct Chase {
	int ticks = -1;
	bool climbed = false;
	float firstClimbLift = 0.f, firstClimbX = 0.f;
	int lastRow = -1;
};

Chase chaseUp(const char* level) {
	SimWorld world;
	REQUIRE(world.Load(level));
	world.player.god = true;
	const auto upper = static_cast<int>(world.Y());
	const int lower = upper - 2;
	REQUIRE(world.dungeon.PlacePlayer(7, lower));
	world.Wait(600); // it sees the player and comes
	const Monster* mon = firstMonster(world);
	REQUIRE(mon != nullptr);
	REQUIRE(mon->Alerted());
	REQUIRE(world.dungeon.PlacePlayer(16, upper));
	Chase chase;
	for (int t = 0; t < 20000 / UPDATE_TICK_MS; t++) {
		world.Tick();
		if (mon->Climbing() && !chase.climbed) {
			chase.climbed = true;
			chase.firstClimbLift = mon->DrawnLift() / RenderConfig::TILE_SIZE;
			chase.firstClimbX = mon->CentreX();
		}
		chase.lastRow = mon->Row();
		if (mon->Row() == upper && !mon->Climbing()) {
			chase.ticks = t;
			break;
		}
	}
	return chase;
}
} // namespace

TEST_CASE("jump onto a ladder: a giant rat leaps the spikes at the ladder's foot and catches the rungs in the air") {
	const Chase rat = chaseUp("tests/levels/leap_ladder");
	CHECK(rat.climbed);
	CHECK(rat.firstClimbLift > 0.1f); // from the arc's height, not the floor
	CHECK(rat.firstClimbX == doctest::Approx(11.5f)); // on the ladder's middle
	CHECK(rat.ticks > 0); // up on the player's floor
}

TEST_CASE("jump onto a ladder: a giant scarab leaps but does not climb, it stays below") {
	const Chase scarab = chaseUp("tests/levels/leap_ladder_scarab");
	CHECK_FALSE(scarab.climbed);
	CHECK(scarab.ticks < 0);
}

TEST_CASE("jump onto a ladder: a ladder over a pit three cells out, only the boss's leap catches it") {
	LevelGrid g{};
	REQUIRE(loadLevelFile("tests/levels/leap_ladder_pit", g).empty());
	int ladderCol = -1, foot = LEVEL_HEIGHT, top = -1;
	for (int row = 0; row < LEVEL_HEIGHT; row++)
		for (int col = 0; col < LEVEL_WIDTH; col++)
			if (g.at(col, row).type == Ladder) {
				ladderCol = col;
				foot = std::min(foot, row);
				top = std::max(top, row);
			}
	REQUIRE(ladderCol == 12);
	REQUIRE_FALSE(isFloor(g.at(ladderCol, foot - 1))); // hangs over the pit
	const int edge = 8; // the last floor cell before the spikes

	ClimbMap rat;
	rat.Build(g.cells, ladderCol + 3, top, ClimbAbility{false, true, bodyOf(MonsterGiantRat)}, CLIMB_PATH_MAX);
	CHECK(rat.Steps(edge, foot) == -1);

	ClimbMap boss;
	boss.Build(
		g.cells,
		ladderCol + 3,
		top,
		ClimbAbility{false, true, bodyOf(MonsterBabi), MONSTER_JUMP_MAX_GAP + BOSS_LEAP_CATCH_EXTRA},
		CLIMB_PATH_MAX
	);
	CHECK(boss.Steps(edge, foot) > 0);
	int col = 0, row = 0;
	REQUIRE(boss.Next(edge, foot, col, row));
	CHECK(col == ladderCol); // the leap at the rungs
	CHECK(row == foot);

	const Chase babi = chaseUp("tests/levels/leap_ladder_pit");
	CHECK(babi.climbed);
	CHECK(babi.firstClimbLift > 0.2f); // nearer the arc's top than the rat's catch
	CHECK(babi.firstClimbX == doctest::Approx(12.5f));
	CHECK(babi.ticks > 0);
}
