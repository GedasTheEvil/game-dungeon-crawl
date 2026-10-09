// The held weapon's swing (swingPose, docs/plan/solved/scenarios-to-unit-tests.md technique 1): tilt, thrust and the
// bow's draw at the marks of every weapon's WeaponMotion. tests/scenarios/weapons_held.txt shows the grip in the fist.
#include "../../external/doctest/doctest.h"
#include "../../src/world/items.h"
#include <cmath>
#include <string>

namespace {
// The poses of a whole swing, one per ms.
template <typename F> void eachMs(const WeaponMotion& m, F f) {
	for (int t = 0; t <= m.swingMs; t++)
		f(t, swingPose(m, t));
}
} // namespace

TEST_CASE("every weapon rests, winds up, strikes at its hit time and comes back to rest") {
	for (int i = 0; i < WEAPON_KIND_COUNT; i++) {
		const auto kind = static_cast<ItemKind>(i);
		const WeaponMotion& m = weaponDef(kind).motion;
		CAPTURE(std::string(itemText(kind).name));
		REQUIRE(m.hitMs > 0);
		REQUIRE(m.swingMs >= m.hitMs);

		const SwingPose rest = swingPose(m, -1);
		CHECK(rest.tilt == m.restTilt);
		CHECK(rest.thrust == 0.f);
		CHECK(rest.draw == 0.f);
		const SwingPose start = swingPose(m, 0); // the attack begins where the rest pose is
		CHECK(start.tilt == doctest::Approx(m.restTilt));
		CHECK(start.thrust == doctest::Approx(0.f));

		const int windupMs = static_cast<int>(std::ceil(static_cast<float>(m.hitMs) * SWING_WINDUP_SHARE));
		const SwingPose windup = swingPose(m, windupMs);
		CHECK(windup.tilt == doctest::Approx(m.windupTilt).epsilon(0.05));
		CHECK(windup.thrust == doctest::Approx(-SWING_WINDUP_PULL * m.thrust).epsilon(0.05));

		const SwingPose strike = swingPose(m, m.hitMs);
		CHECK(strike.tilt == doctest::Approx(m.strikeTilt));
		CHECK(strike.thrust == doctest::Approx(m.thrust));
		CHECK(strike.draw == 0.f); // the bow's string is let go

		const SwingPose done = swingPose(m, m.swingMs);
		CHECK(done.tilt == doctest::Approx(m.restTilt));
		CHECK(done.thrust == doctest::Approx(0.f));
	}
}

TEST_CASE("a swing moves smoothly: no jump in its tilt from one ms to the next") {
	for (int i = 0; i < WEAPON_KIND_COUNT; i++) {
		const auto kind = static_cast<ItemKind>(i);
		const WeaponMotion& m = weaponDef(kind).motion;
		CAPTURE(std::string(itemText(kind).name));
		SwingPose last = swingPose(m, 0);
		float worst = 0.f;
		eachMs(m, [&](int, const SwingPose& p) {
			worst = std::max(worst, std::fabs(p.tilt - last.tilt));
			last = p;
		});
		// The fastest stretch is the strike, gathering speed: at most twice the even pace of its arc.
		const float strikeArc = std::fabs(m.strikeTilt - m.windupTilt);
		const float strikeMs = static_cast<float>(m.hitMs) * (1.f - SWING_WINDUP_SHARE);
		CHECK(worst <= 2.f * strikeArc / strikeMs + 0.5f);
	}
}

TEST_CASE("the bow draws its string to the hit, then lets go") {
	const WeaponMotion& m = weaponDef(ItemKind::SelfBow).motion;
	float last = -1.f;
	for (int t = 0; t < m.hitMs; t += 10) {
		const float draw = swingPose(m, t).draw;
		CHECK(draw > last);
		CHECK(draw < 1.f);
		last = draw;
	}
	CHECK(swingPose(m, m.hitMs - 1).draw == doctest::Approx(1.f).epsilon(0.01));
	CHECK(swingPose(m, m.hitMs).draw == 0.f);
}

TEST_CASE("a thrusting weapon draws back first, then pushes out its reach") {
	for (int i = 0; i < WEAPON_KIND_COUNT; i++) {
		const auto kind = static_cast<ItemKind>(i);
		const WeaponMotion& m = weaponDef(kind).motion;
		if (m.thrust <= 0.f)
			continue;
		CAPTURE(std::string(itemText(kind).name));
		float least = 0.f, most = 0.f;
		eachMs(m, [&](int, const SwingPose& p) {
			least = std::min(least, p.thrust);
			most = std::max(most, p.thrust);
		});
		CHECK(least == doctest::Approx(-SWING_WINDUP_PULL * m.thrust).epsilon(0.02));
		CHECK(most == doctest::Approx(m.thrust));
	}
}
