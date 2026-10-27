// The ring transporter (plan ring-transporter): a ring platform carries the player to its partner like a teleporter,
// the rings dropping round them first, a white flash, the rings rising at the partner; the checker's rules for it.
#include "../../external/doctest/doctest.h"
#include "../../src/world/level_check.h"
#include "../../src/world/tile_defs.h"
#include "sim_world.h"
#include <algorithm>
#include <string>

namespace {
// tests/levels/rings: the ring platform at (6, 2), its partner at (4, 5), a plain teleporter pair at cols 10 and 14.
constexpr float PLATFORM_X = 6.3f;
constexpr int RING_TICKS = (RINGS_DOWN_MS + 2 * RING_FLASH_MS + RINGS_UP_MS) / UPDATE_TICK_MS + 4;

void load(SimWorld& world) {
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/rings"));
	world.WaitTicks(30);
}

bool played(const SimWorld& world, WorldSound sound) {
	return std::any_of(world.said.begin(), world.said.end(), [&](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Sound && e.sound == sound;
	});
}

bool noted(const SimWorld& world, FieldNote note) {
	return std::any_of(world.said.begin(), world.said.end(), [&](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Note && e.note == note;
	});
}

bool warns(const LevelReport& r, const char* part) {
	return std::any_of(r.warnings.begin(), r.warnings.end(), [&](const std::string& w) {
		return w.find(part) != std::string::npos;
	});
}

// A corridor on row 1 with a ring platform pair at cols 3 and 8, rock above both.
LevelGrid ringLevel() {
	LevelGrid grid;
	for (int col = 1; col <= 10; col++)
		grid.set(col, 1, Tile{});
	grid.set(1, 1, cellWith({Door, GateEntrance, 0}));
	grid.set(10, 1, cellWith({Door, GateExit, 0}));
	grid.set(3, 1, cellWith({Door, GateTeleport, 1 | RING_PLATFORM}));
	grid.set(8, 1, cellWith({Door, GateTeleport, 1 | RING_PLATFORM}));
	return grid;
}
} // namespace

TEST_CASE("a ring platform is a teleporter with the ring bit, its pair id unchanged; glyph 0") {
	const Tile ring{Door, GateTeleport, 3 | RING_PLATFORM};
	CHECK(isTeleporter(ring));
	CHECK(isRingPlatform(ring));
	CHECK(teleportPair(ring) == 3);
	CHECK_FALSE(isNameGate(ring));
	CHECK_FALSE(isRingPlatform(Tile{Door, GateTeleport, 3}));
	CHECK(tileGlyph(ring) == '0');
}

TEST_CASE("the rings drop round the player, a flash, and the player stands on the partner platform") {
	SimWorld world;
	load(world);
	REQUIRE(world.WalkTo(PLATFORM_X));
	CHECK(world.dungeon.InteractVerb() == Interaction::Enter);
	world.Interact();
	world.WaitTicks(1);
	CHECK(world.dungeon.Travelling());
	CHECK(world.dungeon.TravelByRings());
	CHECK(world.X() == doctest::Approx(6.f + TELEPORT_ARRIVAL_X)); // stepped into the middle
	CHECK(played(world, WorldSound::RingsDown));
	CHECK(world.dungeon.TravelFade() == 0.f); // no flash while they drop
	world.Wait(RING_FALL_MS / 2);
	CHECK(world.dungeon.RingDrop(0) > 0.f);
	CHECK(world.dungeon.RingDrop(RING_COUNT - 1) == 0.f); // the last one has not started
	world.Wait(RINGS_DOWN_MS - RING_FALL_MS / 2 - 2 * UPDATE_TICK_MS);
	CHECK(world.X() == doctest::Approx(6.f + TELEPORT_ARRIVAL_X));
	CHECK(world.Y() < 3.f);
	for (int k = 0; k < RING_COUNT; k++)
		CHECK(world.dungeon.RingDrop(k) == doctest::Approx(1.f));
	world.Wait(RING_FLASH_MS);
	CHECK(world.dungeon.TravelFade() > 0.5f); // the white flash
	world.WaitTicks(RING_TICKS);
	CHECK_FALSE(world.dungeon.Travelling());
	CHECK(world.X() == doctest::Approx(4.f + TELEPORT_ARRIVAL_X));
	CHECK(world.Y() > 4.f);
	CHECK(played(world, WorldSound::RingsUp));
	CHECK(noted(world, FieldNote::Rings));
	CHECK(world.dungeon.RingDrop(0) == 0.f);
	CHECK(world.dungeon.InteractVerb() == Interaction::Enter); // and back again
	world.Interact();
	world.WaitTicks(RING_TICKS);
	CHECK(world.X() == doctest::Approx(6.f + TELEPORT_ARRIVAL_X));
	CHECK(world.Y() < 3.f);
}

TEST_CASE("a plain teleporter keeps its blue fade, no rings, no rings note") {
	SimWorld world;
	load(world);
	REQUIRE(world.WalkTo(10.5f));
	world.Interact();
	world.WaitTicks(1);
	CHECK_FALSE(world.dungeon.TravelByRings());
	CHECK(world.dungeon.RingDrop(0) == 0.f);
	world.WaitTicks(SimWorld::GATE_TRAVEL_TICKS);
	CHECK(world.X() > 14.f);
	CHECK_FALSE(noted(world, FieldNote::Rings));
}

TEST_CASE("the checker: a ring platform's partner is a ring platform, rock above each") {
	CHECK(checkLevel(ringLevel()).warnings.empty());
	CHECK(checkLevel(ringLevel()).teleporters == 2);

	LevelGrid plainPartner = ringLevel();
	plainPartner.set(8, 1, cellWith({Door, GateTeleport, 1}));
	CHECK(warns(checkLevel(plainPartner), "has a plain teleporter for a partner"));

	LevelGrid openAbove = ringLevel();
	openAbove.set(3, 2, Tile{});
	CHECK(warns(checkLevel(openAbove), "ring platform at (col 3, row 1) without rock above it"));
}

TEST_CASE("a ring platform sealed by the name asks for it, then carries by rings (the way to the Stargate)") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/rings_sealed"));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(PLATFORM_X));
	CHECK(world.dungeon.InteractVerb() == Interaction::Answer);
	world.Interact();
	world.WaitTicks(1);
	CHECK(world.said.back().kind == WorldEvent::Kind::AskName);
	CHECK_FALSE(world.dungeon.Travelling());
	REQUIRE(world.dungeon.SpeakName());
	CHECK(world.dungeon.InteractVerb() == Interaction::Enter);
	world.Interact();
	world.WaitTicks(1);
	CHECK(world.dungeon.TravelByRings()); // the seal gone, the rings stay
	world.WaitTicks(RING_TICKS);
	CHECK(world.X() == doctest::Approx(4.f + TELEPORT_ARRIVAL_X));
	CHECK(world.Y() > 4.f);
}
