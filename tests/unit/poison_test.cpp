#include "../../external/doctest/doctest.h"
#include "../../src/world/items.h"
#include "../../src/world/level_check.h"
#include "../../src/world/poison.h"
#include "../../src/world/tile_defs.h"
#include <sstream>
#include <string>
#include <vector>

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

namespace {
// One corridor drawn in the levelcheck legend, top row first.
LevelGrid corridor(const std::vector<std::string>& rows) {
	LevelGrid grid;
	for (size_t i = 0; i < rows.size(); i++)
		for (size_t col = 0; col < rows[i].size(); col++)
			for (const GlyphDef& g : glyphLegend())
				if (g.glyph == rows[i][col])
					grid.set(static_cast<int>(col), static_cast<int>(rows.size() - 1 - i), g.tile);
	return grid;
}

bool warnsNoAntidote(const LevelGrid& grid) {
	for (const std::string& w : checkLevel(grid).warnings)
		if (w.find("no antidote") != std::string::npos)
			return true;
	return false;
}
} // namespace

TEST_CASE("the checker wants an antidote in reach where something poisons") {
	const ItemFileId antidote = fileIdOf(ItemKind::Antidote);
	LevelGrid scorpion = corridor({"########", "#S.j..E#", "########"});
	CHECK(warnsNoAntidote(scorpion));
	scorpion.set(2, 1, Tile{Treasure, antidote.type, antidote.id});
	CHECK_FALSE(warnsNoAntidote(scorpion));
	CHECK_FALSE(warnsNoAntidote(corridor({"########", "#S.t..E#", "########"})));
}
