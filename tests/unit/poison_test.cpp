#include "../../external/doctest/doctest.h"
#include "../../src/world/poison.h"
#include <sstream>

TEST_CASE("a tier deals its damage once a second for its duration") {
	Poison p;
	p.Apply(PoisonTier::Weak);
	CHECK(p.Advance(999) == 0);
	CHECK(p.Advance(1) == 1);
	int total = 1;
	for (int i = 0; i < 100; i++)
		total += p.Advance(1000);
	CHECK(total == 20);
	CHECK_FALSE(p.Any());
}

TEST_CASE("the totals of the three tiers") {
	for (auto [tier, total] : {std::pair{PoisonTier::Weak, 20}, {PoisonTier::Medium, 75}, {PoisonTier::Strong, 150}}) {
		Poison p;
		p.Apply(tier);
		int hp = 0;
		for (int t = 0; t < 40000; t += 16)
			hp += p.Advance(16);
		CHECK(hp == total);
	}
}

TEST_CASE("the same tier again restarts its timer, at the same strength") {
	Poison p;
	p.Apply(PoisonTier::Weak);
	CHECK(p.Advance(15000) == 15);
	p.Apply(PoisonTier::Weak);
	CHECK(p.LeftMs(PoisonTier::Weak) == 20000);
	CHECK(p.Advance(1000) == 1); // still 1 HP a second, not 2
	CHECK(p.Mask() == 1);
}

TEST_CASE("another tier runs beside it on its own timer") {
	Poison p;
	p.Apply(PoisonTier::Medium);
	CHECK(p.Advance(5000) == 15);
	p.Apply(PoisonTier::Weak);
	CHECK(p.Mask() == 3);
	CHECK(p.Advance(1000) == 4);
	CHECK(p.LeftMs(PoisonTier::Medium) == 19000);
	CHECK(p.LeftMs(PoisonTier::Weak) == 19000);
	p.Apply(PoisonTier::Strong);
	CHECK(p.Mask() == 7);
	CHECK(p.Advance(1000) == 9);
}

TEST_CASE("the antidote cures every tier") {
	Poison p;
	p.Apply(PoisonTier::Weak);
	p.Apply(PoisonTier::Strong);
	p.Cure();
	CHECK_FALSE(p.Any());
	CHECK(p.Advance(5000) == 0);
}

TEST_CASE("poison is saved and loaded; an old save has none") {
	Poison p;
	p.Apply(PoisonTier::Medium);
	p.Advance(2500);
	std::stringstream s;
	p.Save(s);
	s << "next";
	Poison q;
	q.Apply(PoisonTier::Weak);
	q.Load(s);
	CHECK(q.Mask() == 2);
	CHECK(q.LeftMs(PoisonTier::Medium) == 22500);
	CHECK(q.Advance(500) == 3); // the half second already counted
	std::string rest;
	s >> rest;
	CHECK(rest == "next");

	std::stringstream old("INV2 11");
	q.Load(old);
	CHECK_FALSE(q.Any());
	old >> rest;
	CHECK(rest == "INV2");
}
