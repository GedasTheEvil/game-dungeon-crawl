// The decor and decal layers (plan placed-decor): the level format v4, the scatter reading them, the checker's
// warnings, the editor's brushes.
#include "../../external/doctest/doctest.h"
#include "../../src/world/campaign.h"
#include "../../src/world/decor_scatter.h"
#include "../../src/world/level_check.h"
#include "../../tools/editor/dressing_brush.h"
#include <memory>
#include <sstream>
#include <string>

namespace {
int idx(int col, int row) { return row * LEVEL_WIDTH + col; }

Dressing& dressing(LevelGrid& grid, int col, int row) { return grid.cells[idx(col, row)].dressing; }

std::unique_ptr<DecorLayout> scatter(const LevelGrid& grid, const char* name, int depth) {
	auto layout = std::make_unique<DecorLayout>();
	scatterDecor(grid.cells, name, depth, grid.meta.zone, *layout);
	return layout;
}

// The level as v3 text: the version 3 head, no decor and decal lists.
std::string asV3(const LevelGrid& grid) {
	std::ostringstream v4;
	writeLevel(v4, grid.cells);
	std::string text = v4.str();
	const std::string head = std::string(LEVEL_MAGIC) + " " + std::to_string(LEVEL_VERSION) + " ";
	REQUIRE(text.rfind(head, 0) == 0);
	text.replace(0, head.size(), std::string(LEVEL_MAGIC) + " 3 ");
	const size_t zone = text.find("zone "); // v5 on
	text.erase(zone, text.find('\n', zone) + 1 - zone);
	text.erase(text.find("decor "));
	return text;
}

bool sameLayout(const DecorLayout& a, const DecorLayout& b) {
	for (int c = 0; c < LEVEL_CELL_COUNT; c++) {
		const DecorCell& p = a.decor[c];
		const DecorCell& q = b.decor[c];
		const DecalCell& r = a.decal[c];
		const DecalCell& s = b.decal[c];
		if (p.type != q.type || p.mirror != q.mirror || p.offsetX != q.offsetX || r.type != s.type ||
			r.mirror != s.mirror || r.x != s.x || r.y != s.y || a.torch[c] != b.torch[c])
			return false;
	}
	return true;
}

bool warns(const LevelReport& r, const std::string& part) {
	for (const std::string& w : r.warnings)
		if (w.find(part) != std::string::npos)
			return true;
	return false;
}

// A corridor on row 1 from col 1 to 20 (rock above and below), entrance at col 1, exit at col 20.
LevelGrid corridor() {
	LevelGrid grid;
	for (int col = 1; col <= 20; col++)
		grid.set(col, 1, Tile{});
	grid.set(1, 1, Tile{Door, GateEntrance, 0});
	grid.set(20, 1, Tile{Door, GateExit, 0});
	return grid;
}
} // namespace

TEST_CASE("v4 keeps every cell's prop, decal and torch with their mirrors; unset cells are not listed") {
	LevelGrid grid = corridor();
	dressing(grid, 3, 1) = Dressing{DECOR_COFFIN_MUMMY, true};
	dressing(grid, 4, 1).prop = PLACED_BARE;
	dressing(grid, 5, 1) = Dressing{PLACED_UNSET, false, 7, true, TorchPlacing::None};
	dressing(grid, 6, 1).torch = TorchPlacing::Torch; // a torch alone: its decal stays unset
	dressing(grid, 7, 1).decal = PLACED_BARE;
	std::stringstream file;
	writeLevel(file, grid.cells);
	CHECK(
		file.str().find("decor 2\n3 1 24 1\n4 1 -1 0\ndecals 3\n5 1 7 1 1\n6 1 -2 0 2\n7 1 -1 0 0\n") !=
		std::string::npos
	);
	LevelGrid loaded;
	int version = 0;
	REQUIRE(readLevel(file, loaded.cells, &version).empty());
	CHECK(version == LEVEL_VERSION);
	for (int c = 0; c < LEVEL_WIDTH * LEVEL_HEIGHT; c++) {
		const Dressing& a = grid.cells[c].dressing;
		const Dressing& b = loaded.cells[c].dressing;
		CHECK(a.prop == b.prop);
		CHECK(a.propMirror == b.propMirror);
		CHECK(a.decal == b.decal);
		CHECK(a.decalMirror == b.decalMirror);
		CHECK(a.torch == b.torch);
	}
}

TEST_CASE("v4 refuses a prop, decal or torch it does not know") {
	LevelGrid grid = corridor();
	std::ostringstream out;
	writeLevel(out, grid.cells);
	const std::string text = out.str();
	auto readWith = [&](const std::string& from, const std::string& to) {
		std::string bad = text;
		bad.replace(bad.find(from), from.size(), to);
		std::istringstream in(bad);
		LevelGrid loaded;
		return readLevel(in, loaded.cells);
	};
	CHECK(
		readWith("decor 0\n", "decor 1\n2 1 " + std::to_string(DECOR_COUNT) + " 0\n").find("unknown prop") !=
		std::string::npos
	);
	CHECK(readWith("decals 0\n", "decals 1\n2 1 -3 0 0\n").find("unknown decal") != std::string::npos);
	CHECK(readWith("decals 0\n", "decals 1\n2 1 0 0 3\n").find("unknown torch") != std::string::npos);
	CHECK(readWith("decor 0\n", "decor 1\n2 99 0 0\n").find("outside the level") != std::string::npos);
	CHECK(readWith("decor 0\ndecals 0\n", "").find("no decor layer") != std::string::npos);
}

TEST_CASE("a v3 level loads with every cell unset and scatters as before") {
	for (int n = 1; n <= CAMPAIGN_LEVELS; n++) {
		const std::string file = campaignLevelFile(n);
		LevelGrid v4;
		REQUIRE(loadLevelFile(file.c_str(), v4).empty());
		std::istringstream in(asV3(v4));
		LevelGrid v3;
		int version = 0;
		REQUIRE(readLevel(in, v3.cells, &version).empty());
		CHECK(version == 3);
		bool unset = true;
		for (const Tile& t : v3.cells)
			unset = unset && !hasPlacedProp(t.dressing) && !hasPlacedDecal(t.dressing);
		CHECK(unset);
		bool placed = false; // a generated level's tombs and boss gate (level_gen.h): v3 drops them
		for (const Tile& t : v4.cells)
			placed = placed || hasPlacedProp(t.dressing) || hasPlacedDecal(t.dressing);
		v3.meta = v4.meta; // v3 has no zone line: the same zone, to compare the scatter
		if (!placed)
			CHECK_MESSAGE(sameLayout(*scatter(v3, file.c_str(), n), *scatter(v4, file.c_str(), n)), "lvl", n);
	}
}

TEST_CASE("bare and placed cells beat the roll, props, decals and torches alike") {
	const int level = 20;
	const std::string file = campaignLevelFile(level);
	LevelGrid grid;
	REQUIRE(loadLevelFile(file.c_str(), grid).empty());
	const auto rolled = scatter(grid, file.c_str(), level);

	// Bare every cell the scatter dressed (not the coffins, skeletons and ushabti: they come with their tile); place a
	// prop and a decal on every open cell it left bare, and turn each random torch into none and the rest into torches.
	LevelGrid placed = grid;
	for (int c = 0; c < LEVEL_WIDTH * LEVEL_HEIGHT; c++) {
		Dressing& d = placed.cells[c].dressing;
		if (isWall(grid.cells[c]))
			continue;
		d.prop = rolled->decor[c].type >= 0 ? PLACED_BARE : DECOR_CAT;
		d.propMirror = true;
		d.decal = rolled->decal[c].type >= 0 ? PLACED_BARE : 3;
		d.torch = rolled->torch[c] ? TorchPlacing::None : TorchPlacing::Torch;
	}
	const auto layout = scatter(placed, file.c_str(), level);
	int coffins = 0;
	for (int c = 0; c < LEVEL_WIDTH * LEVEL_HEIGHT; c++) {
		if (isWall(grid.cells[c]))
			continue;
		const int8_t before = rolled->decor[c].type;
		const bool own = isCoffin(before) || ((before == DECOR_SKELETON || before == DECOR_USHABTI) &&
											  (grid.cells[c].type == Page || spawnType(grid.cells[c]) != 0));
		coffins += own ? 1 : 0;
		if (own)
			CHECK(layout->decor[c].type == before); // the scatter's own beats the layer
		else if (before >= 0)
			CHECK(layout->decor[c].type == -1);
		else {
			CHECK(layout->decor[c].type == DECOR_CAT);
			CHECK(layout->decor[c].mirror);
		}
		CHECK(layout->decal[c].type == (rolled->decal[c].type >= 0 ? -1 : 3));
		CHECK(layout->torch[c] == !rolled->torch[c]);
	}
	CHECK(coffins > 0); // lvl20 has mummies: the coffins stay on their spawns
}

TEST_CASE("random torches keep the gap to a placed one, placed ones need none") {
	const int level = 12;
	const std::string file = campaignLevelFile(level);
	LevelGrid grid;
	REQUIRE(loadLevelFile(file.c_str(), grid).empty());
	// A placed torch every 7 cells of every row: a random one fits between them only right in the middle.
	for (int row = 0; row < LEVEL_HEIGHT; row++)
		for (int col = 3; col < LEVEL_WIDTH; col += 7)
			if (!isWall(grid.at(col, row)))
				dressing(grid, col, row).torch = TorchPlacing::Torch;
	dressing(grid, 2, 1).torch = TorchPlacing::Torch; // two placed side by side, if row 1 is open there
	const auto layout = scatter(grid, file.c_str(), level);
	int placedNear = 0;
	for (int row = 0; row < LEVEL_HEIGHT; row++)
		for (int col = 0; col < LEVEL_WIDTH; col++) {
			const bool placed = grid.at(col, row).dressing.torch == TorchPlacing::Torch;
			if (placed)
				CHECK(layout->torch[idx(col, row)]);
			if (!layout->torch[idx(col, row)])
				continue;
			for (int k = col + 1; k <= col + 4 && k < LEVEL_WIDTH; k++) {
				const bool both = placed && grid.at(k, row).dressing.torch == TorchPlacing::Torch;
				placedNear += both && layout->torch[idx(k, row)] ? 1 : 0;
				if (!both)
					CHECK_MESSAGE(!layout->torch[idx(k, row)], "row ", row, " cols ", col, " and ", k);
			}
		}
	if (!isWall(grid.at(2, 1)) && !isWall(grid.at(3, 1)))
		CHECK(placedNear > 0);
}

TEST_CASE("no random brazier under a placed torch, no random prop in front of a placed floor decal") {
	const int level = 30;
	const std::string file = campaignLevelFile(level);
	LevelGrid grid;
	REQUIRE(loadLevelFile(file.c_str(), grid).empty());
	LevelGrid torches = grid;
	LevelGrid grass = grid;
	for (int c = 0; c < LEVEL_WIDTH * LEVEL_HEIGHT; c++) {
		torches.cells[c].dressing.torch = TorchPlacing::Torch;
		grass.cells[c].dressing.decal = 13; // dry grass grows from the floor
	}
	REQUIRE(DECAL_DEFS[13].anchor == DecalAnchor::Floor);
	const auto lit = scatter(torches, file.c_str(), level);
	const auto grown = scatter(grass, file.c_str(), level);
	int props = 0;
	for (int c = 0; c < LEVEL_WIDTH * LEVEL_HEIGHT; c++) {
		const int prop = lit->decor[c].type;
		props += prop >= 0 ? 1 : 0;
		CHECK_FALSE((prop == DECOR_BRAZIER || prop == DECOR_LAMP || isThoth(prop)));
		const int beforeGrass = grown->decor[c].type;
		CHECK(
			(beforeGrass < 0 || isCoffin(beforeGrass) || isWaterProp(beforeGrass) || beforeGrass == DECOR_SKELETON ||
			 beforeGrass == DECOR_USHABTI || hasPlacedProp(grass.cells[c].dressing))
		);
	}
	CHECK(props > 0);
}

TEST_CASE("the checker warns where a placed prop, decal or torch does not fit") {
	LevelGrid grid = corridor();
	grid.set(10, 2, Tile{}); // an open cell over the corridor: no ceiling at col 10, no floor at (10, 2)
	auto warnsFor = [&](int col, int row, const Dressing& d, const std::string& part) {
		LevelGrid g = grid;
		dressing(g, col, row) = d;
		return warns(checkLevel(g), part);
	};
	CHECK_FALSE(warnsFor(5, 1, Dressing{DECOR_CAT}, "the cat"));
	CHECK(warnsFor(5, 3, Dressing{DECOR_CAT}, "is in rock"));
	CHECK(warnsFor(10, 2, Dressing{DECOR_CAT}, "no floor under it"));
	CHECK(warnsFor(10, 1, Dressing{DECOR_WEB}, "has no ceiling"));
	CHECK_FALSE(warnsFor(5, 1, Dressing{DECOR_WEB}, "the web"));
	CHECK(warnsFor(5, 1, Dressing{DECOR_WATER_LOTUS}, "out of half water"));
	CHECK(warnsFor(1, 1, Dressing{DECOR_OSIRIS}, "in front of the Door"));
	CHECK_FALSE(warnsFor(1, 1, Dressing{DECOR_WEB}, "in front of"));
	CHECK(warnsFor(5, 1, Dressing{PLACED_UNSET, false, 3}, "hangs from the ceiling") == false);
	CHECK(warnsFor(10, 1, Dressing{PLACED_UNSET, false, 3}, "hangs from the ceiling"));
	CHECK(warnsFor(10, 2, Dressing{PLACED_UNSET, false, 13}, "grows from the floor"));
	CHECK(warnsFor(5, 1, Dressing{DECOR_CAT, false, 13}, "behind a prop"));
	CHECK(warnsFor(5, 1, Dressing{PLACED_UNSET, false, 0, false, TorchPlacing::Torch}, "behind the torch"));
	CHECK(warnsFor(5, 1, Dressing{DECOR_BRAZIER, false, PLACED_UNSET, false, TorchPlacing::Torch}, "over a brazier"));
	CHECK(warnsFor(1, 1, Dressing{PLACED_UNSET, false, PLACED_UNSET, false, TorchPlacing::Torch}, "takes no torch"));
	CHECK(warnsFor(1, 1, Dressing{PLACED_UNSET, false, 0}, "takes no decals"));
	CHECK_FALSE(warnsFor(5, 1, Dressing{PLACED_BARE, false, PLACED_BARE, false, TorchPlacing::None}, "at (col 5"));

	LevelGrid mummy = grid;
	mummy.set(8, 1, Tile{MonsterSpawn, MonsterMummy, 0});
	dressing(mummy, 8, 1).prop = DECOR_JACKAL;
	CHECK(warns(checkLevel(mummy), "is not shown"));
}

TEST_CASE("levelcheck --map draws the placed props and decals") {
	LevelGrid grid = corridor();
	dressing(grid, 3, 1).prop = DECOR_COFFIN_MUMMY;
	dressing(grid, 4, 1).prop = PLACED_BARE;
	dressing(grid, 5, 1) = Dressing{PLACED_UNSET, false, 2, false, TorchPlacing::Torch};
	dressing(grid, 6, 1).torch = TorchPlacing::Torch;
	dressing(grid, 7, 1).torch = TorchPlacing::None;
	const std::string map = renderLevel(grid, nullptr);
	CHECK(map.find("decor\n 1 #..y-.") != std::string::npos);
	CHECK(map.find("decals\n 1 #....Ct-.") != std::string::npos);
	CHECK(renderLevel(corridor(), nullptr).find("decor") == std::string::npos);
}

TEST_CASE("the editor's decal brushes: a torch and a free decal clear each other, Unset and Bare set both") {
	Dressing d;
	paintDecal(d, 0, true); // a free crack
	CHECK(d.decal == 0);
	CHECK(d.decalMirror);
	paintDecal(d, TORCH_BRUSH, false);
	CHECK(d.torch == TorchPlacing::Torch);
	CHECK(d.decal == PLACED_UNSET); // the torch took the middle of the wall
	paintDecal(d, 3, false); // vines from the ceiling go with a torch
	CHECK(d.torch == TorchPlacing::Torch);
	CHECK(pickDecalBrush(d) == 3);
	paintDecal(d, 7, false); // a glyph column does not
	CHECK(d.torch == TorchPlacing::Unset);
	paintDecal(d, NO_TORCH_BRUSH, false);
	CHECK(d.decal == 7);
	CHECK(d.torch == TorchPlacing::None);
	paintDecal(d, PLACED_BARE, false);
	CHECK(d.decal == PLACED_BARE);
	CHECK(d.torch == TorchPlacing::None);
	paintDecal(d, PLACED_UNSET, false);
	CHECK_FALSE(hasPlacedDecal(d));
	CHECK(pickDecalBrush(d) == PLACED_UNSET);
	d.torch = TorchPlacing::Torch;
	CHECK(pickDecalBrush(d) == TORCH_BRUSH);

	paintProp(d, DECOR_CAT, true);
	CHECK(d.prop == DECOR_CAT);
	CHECK(d.propMirror);
	paintProp(d, PLACED_BARE, true);
	CHECK_FALSE(d.propMirror);
}
