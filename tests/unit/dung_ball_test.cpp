// The dung ball (plan scarab-dung-ball): the giant scarab turns its back on the player and kicks a ball at them along
// the row; it hits a player on the floor, a jump clears it, it falls into a pit, sinks in water, breaks at the end of
// its roll, rolls over traps; one ball per cooldown. Khepri's is bigger, faster, from farther, and burns.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
const Monster* roller(const SimWorld& world) {
	for (int i = 0; i < MAX_MONSTERS; i++) {
		const Monster& mon = world.dungeon.Monsters()[i];
		if (mon.Active() && mon.Type()->roll && !mon.Minion())
			return &mon;
	}
	return nullptr;
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

// A level of tests/levels with a player who lives through a few balls; the roller on it.
const Monster* load(SimWorld& world, const char* level) {
	REQUIRE(world.Load(level));
	world.player.stats.AddXP(1000000, world.events);
	world.Tick();
	return roller(world);
}

// Steps up to gap tiles from the roller's box (by default just out of the giant scarab's range: it comes into it).
void approach(SimWorld& world, const Monster& mon, float gap = 1.15f) {
	for (int t = 0; t < 400 && mon.PlayerGap(world.X()) > gap; t++) { // it comes too
		world.Step(1);
		world.Tick();
	}
	REQUIRE(mon.PlayerGap(world.X()) <= gap);
}

bool rolling(const SimWorld& world) {
	const auto& balls = world.dungeon.Balls();
	return std::any_of(balls.begin(), balls.end(), [](const auto& b) { return b.breakMs < 0; });
}
bool broken(const SimWorld& world) {
	const auto& balls = world.dungeon.Balls();
	return !balls.empty() && balls.front().breakMs >= 0;
}
} // namespace

TEST_CASE("the giant scarab and Khepri roll dung balls, the small scarab none; Khepri's is bigger, faster, farther") {
	const MonsterKind& giant = *monsterKind(MonsterGiantScarab);
	const MonsterKind& khepri = *monsterKind(MonsterBossScarab);
	CHECK_FALSE(monsterKind(MonsterScarab)->roll);
	REQUIRE(giant.roll);
	REQUIRE(khepri.roll);
	CHECK(giant.roll->minRange > MONSTER_BITE_REACH);
	CHECK(giant.roll->maxRange > giant.roll->minRange);
	CHECK(khepri.roll->radius > giant.roll->radius);
	CHECK(khepri.roll->speed > giant.roll->speed);
	CHECK(khepri.roll->maxRange > giant.roll->maxRange);
	CHECK(khepri.roll->damage > giant.roll->damage);
	CHECK(khepri.roll->burns());
	CHECK_FALSE(giant.roll->burns());
	CHECK(mainType(giant.roll->mix) == DamageType::Blunt);
	for (const MonsterKind* kind : {&giant, &khepri}) {
		CHECK(&ClipFilesOf(*kind) == &ROLLER_CLIPS);
		const MonsterType& type = SimWorld::monsterTypes()[static_cast<size_t>(kind->id)];
		CHECK(type.model.Clip(ModelState::AttackRanged).present);
		CHECK(type.model.Clip(ModelState::Jump).present);
	}
}

TEST_CASE("the giant scarab turns its back, shapes a ball and kicks it at the player on the floor: a blunt hit") {
	SimWorld world;
	const Monster* k = load(world, "tests/levels/dung_ball");
	REQUIRE(k != nullptr);
	approach(world, *k);
	CHECK_FALSE(k->RollingBall()); // out of its range
	const int hp = world.Hp();
	REQUIRE(waitFor(world, 3000, [&] { return k->RollingBall(); }));
	CHECK(world.Saw(MonsterGiantScarab, CreatureMove::RollBall));
	CHECK(k->BallDir() == -1); // to the player, west
	world.Tick();
	CHECK(k->Facing() == 1); // its back to them
	CHECK(k->State() == ModelState::AttackRanged);
	world.Wait(400);
	CHECK(k->BallShaped() > 0.3f);
	CHECK(world.dungeon.Balls().empty()); // not kicked yet
	REQUIRE(waitFor(world, 600, [&] { return rolling(world); }));
	CHECK(world.dungeon.Balls().front().x == doctest::Approx(k->Left()).epsilon(0.02));
	REQUIRE(waitFor(world, 1500, [&] { return broken(world); }));
	CHECK(world.Hp() < hp);
	CHECK(std::fabs(world.dungeon.Balls().front().x - world.X()) < 0.3f); // on the player
	CHECK(world.Creature(MonsterGiantScarab)->hitBy);
	CHECK(world.said.end() != std::find_if(world.said.begin(), world.said.end(), [](const WorldEvent& e) {
			  return e.kind == WorldEvent::Kind::Sound && e.sound == WorldSound::DungThud;
		  }));
}

TEST_CASE(
	"a jump clears the dung ball: it rolls on past the player, over the spikes, and falls apart at the end of its "
	"roll") {
	SimWorld world;
	const Monster* k = load(world, "tests/levels/dung_ball");
	REQUIRE(k != nullptr);
	approach(world, *k);
	REQUIRE(waitFor(world, 4000, [&] { return rolling(world); }));
	const float from = world.dungeon.Balls().front().x;
	REQUIRE(waitFor(world, 1000, [&] { return world.dungeon.Balls().front().x - world.X() < 0.8f; }));
	const int hp = world.Hp();
	world.facing = 0; // straight up
	world.Jump();
	REQUIRE(waitFor(world, 3000, [&] { return broken(world); }));
	CHECK(world.Hp() == hp);
	CHECK(world.dungeon.Balls().front().x == doctest::Approx(from - DUNG_BALL_MAX_ROLL).epsilon(0.01));
	CHECK(world.dungeon.Balls().front().x < 9.f); // past the spikes in col 9
}

TEST_CASE("one ball per cooldown") {
	SimWorld world;
	const Monster* k = load(world, "tests/levels/dung_ball");
	REQUIRE(k != nullptr);
	approach(world, *k);
	REQUIRE(waitFor(world, 4000, [&] { return rolling(world); }));
	const int kicked = GameClock::now();
	REQUIRE(waitFor(world, 1500, [&] { return !k->RollingBall(); }));
	// It keeps coming; the player keeps in its range, a step back now and then.
	const int cooldown = monsterKind(MonsterGiantScarab)->roll->cooldownMs;
	CHECK_FALSE(waitFor(world, cooldown - (GameClock::now() - kicked) - 100, [&] {
		if (k->PlayerGap(world.X()) < 1.f)
			world.Step(-1);
		return k->RollingBall();
	}));
	REQUIRE(waitFor(world, 1000, [&] { return k->RollingBall(); }));
	CHECK(GameClock::now() - kicked >= cooldown);
}

TEST_CASE("the dung ball falls into a pit and breaks at its bottom; in half water it sinks") {
	SimWorld world;
	SUBCASE("a pit") {
		const Monster* k = load(world, "tests/levels/dung_ball_pit");
		REQUIRE(k != nullptr);
		REQUIRE(world.WalkTo(10.95f)); // by the edge: it kicks the ball from over the far one
		const int hp = world.Hp();
		REQUIRE(waitFor(world, 4000, [&] { return broken(world); }));
		const auto& ball = world.dungeon.Balls().front();
		CHECK(ball.falling);
		CHECK_FALSE(ball.sank);
		CHECK(ball.y < static_cast<float>(k->Row()) - 1.5f); // at the bottom, two cells down
		CHECK(static_cast<int>(std::floor(ball.x)) == 11);
		CHECK(world.Hp() == hp);
	}
	SUBCASE("half water") {
		const Monster* k = load(world, "tests/levels/dung_ball_water");
		REQUIRE(k != nullptr);
		REQUIRE(world.WalkTo(10.95f)); // by the edge: it kicks the ball from over the far one
		const int hp = world.Hp();
		REQUIRE(waitFor(world, 4000, [&] { return broken(world); }));
		CHECK(world.dungeon.Balls().front().sank);
		CHECK(static_cast<int>(std::floor(world.dungeon.Balls().front().x)) == 11);
		CHECK(world.Hp() == hp);
	}
}

TEST_CASE("Khepri's ball burns: its hit sets the player burning") {
	SimWorld world;
	const Monster* khepri = load(world, "tests/levels/dung_ball_khepri");
	REQUIRE(khepri != nullptr);
	approach(world, *khepri);
	REQUIRE(khepri->Type()->id == MonsterBossScarab);
	REQUIRE(waitFor(world, 6000, [&] { return rolling(world); }));
	CHECK(khepri->PlayerGap(world.X()) > monsterKind(MonsterGiantScarab)->roll->maxRange); // from farther
	REQUIRE(waitFor(world, 1500, [&] { return broken(world); }));
	CHECK(world.player.burning.Any());
	CHECK(world.Saw(MonsterBossScarab, CreatureMove::RollBall));
}
