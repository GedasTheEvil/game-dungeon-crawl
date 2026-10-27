// The Stargate chamber (plan stargate-chamber): the ring dialled, its chevrons locking one by one, the seventh that
// will not, the XP the first time; the dial device's note; the chamber on lvl48 and the checker's torch rule.
#include "../../external/doctest/doctest.h"
#include "../../src/world/decor.h"
#include "../../src/world/level_check.h"
#include "sim_world.h"
#include <algorithm>
#include <string>

namespace {
// tests/levels/rings: its corridor on row 2, cols 2 to 5 empty: the dial device at col 3, the ring at col 4.
constexpr int ROW = 2;
constexpr int DIAL_COL = 3;
constexpr int GATE_COL = 4;

void load(SimWorld& world) {
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/rings"));
	REQUIRE(world.dungeon.PlaceDecor(DIAL_COL, ROW, DECOR_DIAL));
	REQUIRE(world.dungeon.PlaceDecor(GATE_COL, ROW, DECOR_STARGATE));
	world.WaitTicks(30);
}

int played(const SimWorld& world, WorldSound sound) {
	return static_cast<int>(std::count_if(world.said.begin(), world.said.end(), [&](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Sound && e.sound == sound;
	}));
}

bool noted(const SimWorld& world, FieldNote note) {
	return std::any_of(world.said.begin(), world.said.end(), [&](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Note && e.note == note;
	});
}

float glow(const SimWorld& world, int chevron) { return world.dungeon.ChevronGlow(GATE_COL, ROW, chevron); }
} // namespace

TEST_CASE("the Stargate: Dial while it is still, Press on its dial device") {
	SimWorld world;
	load(world);
	REQUIRE(world.WalkTo(GATE_COL + 0.5f));
	CHECK(world.dungeon.InteractVerb() == Interaction::Dial);
	CHECK(std::string(interactionVerb(Interaction::Dial)) == "Dial");
	world.Interact();
	world.WaitTicks(1);
	CHECK(world.dungeon.Dialling());
	CHECK(world.dungeon.InteractVerb() == Interaction::None); // not twice at once
	REQUIRE(world.WalkTo(DIAL_COL + 0.5f));
	CHECK(world.dungeon.InteractVerb() == Interaction::Press);
	world.Interact();
	world.WaitTicks(1);
	CHECK(played(world, WorldSound::DialPress) == 1);
	CHECK(noted(world, FieldNote::DialDevice));
	CHECK(world.Told("Nothing happens"));
}

TEST_CASE("dialled, six chevrons lock one by one, the seventh flickers out, the first dial gives a riddle's XP") {
	SimWorld world;
	load(world);
	REQUIRE(world.WalkTo(GATE_COL + 0.5f));
	const int xp = world.Xp();
	const int reward = world.player.stats.RiddleXP();
	const float rest = world.dungeon.StargateTrackDeg(GATE_COL, ROW);
	world.Interact();
	world.Wait(STARGATE_TURN_MS / 2);
	CHECK(glow(world, 0) == 0.f); // still turning
	CHECK(world.dungeon.StargateTrackDeg(GATE_COL, ROW) != doctest::Approx(rest));
	world.Wait(STARGATE_TURN_MS);
	CHECK(glow(world, 0) == 1.f);
	CHECK(glow(world, 1) == 0.f);
	world.Wait(STARGATE_FAIL_AT_MS - STARGATE_TURN_MS / 2 - STARGATE_TURN_MS * 3 / 2); // the seventh turning
	for (int k = 0; k < STARGATE_CHEVRONS - 1; k++)
		CHECK(glow(world, k) == 1.f);
	CHECK(played(world, WorldSound::ChevronLock) == STARGATE_CHEVRONS - 1);
	CHECK(played(world, WorldSound::ChevronFail) == 1);
	CHECK(world.Xp() == xp); // not yet
	world.Wait(STARGATE_TURN_MS / 2 + 2 * UPDATE_TICK_MS);
	CHECK(world.Told("Chevron seven... will not lock. +"));
	CHECK(world.Xp() == xp + reward);
	CHECK(noted(world, FieldNote::Stargate));
	float seventh = 0.f;
	for (int t = 0; t < STARGATE_FAIL_MS; t += UPDATE_TICK_MS) {
		seventh = std::max(seventh, glow(world, STARGATE_CHEVRONS - 1));
		world.WaitTicks(1);
	}
	CHECK(seventh > 0.f); // it flickered
	world.WaitTicks(2);
	CHECK(glow(world, STARGATE_CHEVRONS - 1) == 0.f); // and died
	world.Wait(STARGATE_FADE_MS + 2 * UPDATE_TICK_MS);
	CHECK_FALSE(world.dungeon.Dialling());
	for (int k = 0; k < STARGATE_CHEVRONS; k++)
		CHECK(glow(world, k) == 0.f);
	CHECK(world.dungeon.InteractVerb() == Interaction::Dial); // again, the track where it stopped

	world.journal.LearnNote(FieldNote::Stargate); // the app writes the note down
	world.Interact();
	world.Wait(STARGATE_DIAL_MS + 2 * UPDATE_TICK_MS);
	CHECK(world.Xp() == xp + reward); // only the first dial pays
	CHECK(world.Told("Chevron seven... will not lock."));
}

TEST_CASE("the Stargate chamber on lvl48: the ring and its dial behind the name gate, the ring's text and a chest") {
	LevelGrid grid;
	REQUIRE(loadLevelFile("content/levels/lvl48", grid).empty());
	int gates = 0, dials = 0;
	for (const Tile& t : grid.cells) {
		gates += t.dressing.prop == DECOR_STARGATE ? 1 : 0;
		dials += t.dressing.prop == DECOR_DIAL ? 1 : 0;
	}
	CHECK(gates == 1);
	CHECK(dials == 1);
	CHECK(grid.at(35, 4).dressing.prop == DECOR_STARGATE);
	CHECK(grid.at(32, 4).fixture.type == Inscription);
	CHECK(isTreasure(grid.at(32, 4)));
	CHECK(checkLevel(grid).warnings.empty());
}

TEST_CASE("the checker warns about a torch over the Stargate") {
	LevelGrid grid;
	REQUIRE(loadLevelFile("content/levels/lvl48", grid).empty());
	Tile cell = grid.at(35, 4);
	cell.dressing.torch = TorchPlacing::Torch;
	grid.set(35, 4, cell);
	const LevelReport r = checkLevel(grid);
	CHECK(std::any_of(r.warnings.begin(), r.warnings.end(), [](const std::string& w) {
		return w.find("is over a stargate") != std::string::npos;
	}));
}
