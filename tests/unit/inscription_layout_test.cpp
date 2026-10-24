// Composed inscriptions (plan composed-inscriptions): the hieroglyph sheet's manifest, every G: code in it, and where
// the signs go on the plates.
#include "../../external/doctest/doctest.h"
#include "../../src/world/inscription_layout.h"
#include <filesystem>
#include <fstream>
#include <set>
#include "../../src/core/paths.h"

namespace {
// A sheet of square signs named S0, S1, ...
HieroglyphSheet squares(int count) {
	HieroglyphSheet sheet;
	for (int k = 0; k < count; k++)
		sheet.Add("S" + std::to_string(k), 1.f);
	return sheet;
}

std::vector<std::string> codes(int count) {
	std::vector<std::string> out;
	for (int k = 0; k < count; k++)
		out.push_back("S" + std::to_string(k));
	return out;
}
} // namespace

TEST_CASE("every G: code of the texts and the names is in the shipped sheet") {
	HieroglyphSheet sheet;
	REQUIRE(sheet.Load(HIEROGLYPH_MANIFEST).empty());
	CHECK(sheet.Size() <= static_cast<size_t>(HIEROGLYPH_SHEET_GRID * HIEROGLYPH_SHEET_GRID));
	InscriptionBook book;
	REQUIRE(book.Load(CONTENT_DIR "inscriptions").empty());
	NameList names;
	REQUIRE(names.Load(CONTENT_DIR "names").empty());
	CHECK(checkSigns(sheet, book, names).empty());
	for (size_t i = 0; i < names.Size(); i++) {
		CAPTURE(names.At(i).name);
		CHECK_FALSE(names.At(i).glyphs.empty()); // each name spells itself
		CHECK(names.At(i).glyphs.size() <= static_cast<size_t>(signCap(InscriptionStyle::Cartouche)));
	}
}

TEST_CASE("a code the sheet lacks is a load error with its file and line") {
	const HieroglyphSheet sheet = squares(2);
	InscriptionBook book;
	InscriptionText text;
	text.glyphs = {"S0", "X9", "S1"};
	text.glyphSource = "f.txt:4";
	book.Add(text);
	const std::vector<std::string> errors = checkSigns(sheet, book, NameList{});
	REQUIRE(errors.size() == 1);
	CHECK(errors[0].find("f.txt:4: no sign X9 in the hieroglyph sheet") == 0);
	CHECK(inscriptionLayout(InscriptionStyle::Row, text.glyphs, sheet).size() == 2); // left out, the rest drawn
}

TEST_CASE("the sheet's uv: the sign's own box in its cell, its width / height kept") {
	HieroglyphSheet sheet;
	sheet.Add("A1", 1.f);
	sheet.Add("N35", 4.f);
	CHECK(sheet.Cell("N35") == 1);
	CHECK(sheet.Cell("Q3") == -1);
	const AtlasRect cell = atlasCell(1, HIEROGLYPH_SHEET_GRID);
	const AtlasRect uv = sheet.Uv(1);
	CHECK(uv.u0 > cell.u0);
	CHECK(uv.u1 < cell.u1);
	CHECK((uv.u1 - uv.u0) / (uv.v1 - uv.v0) == doctest::Approx(4.f));
	CHECK((uv.v0 + uv.v1) / 2 == doctest::Approx((cell.v0 + cell.v1) / 2));
}

TEST_CASE("the layout: caps per style, every sign inside the plate, the colour cycle") {
	const HieroglyphSheet sheet = squares(20);
	for (int s = 0; s < INSCRIPTION_STYLE_COUNT; s++) {
		const auto style = static_cast<InscriptionStyle>(s);
		const std::vector<SignQuad> quads = inscriptionLayout(style, codes(20), sheet);
		CAPTURE(s);
		CHECK(static_cast<int>(quads.size()) == signCap(style));
		for (const SignQuad& q : quads) {
			CHECK(q.x0 >= 0.f);
			CHECK(q.y0 >= 0.f);
			CHECK(q.x1 <= 1.f);
			CHECK(q.y1 <= 1.f);
			CHECK(q.x1 - q.x0 == doctest::Approx(q.y1 - q.y0)); // square signs stay square
			CHECK(q.colour >= 0);
			CHECK(q.colour < SIGN_COLOURS);
		}
	}
	CHECK(signCap(InscriptionStyle::Cartouche) == 5);
	CHECK(signCap(InscriptionStyle::Column) == 7);
	CHECK(signCap(InscriptionStyle::Stela) == 12);

	// A column reads top down, the colours cycle on.
	const std::vector<SignQuad> column = inscriptionLayout(InscriptionStyle::Cartouche, codes(5), sheet);
	for (size_t k = 1; k < column.size(); k++) {
		CHECK(column[k].y1 <= column[k - 1].y0);
		CHECK(column[k].colour == (column[k - 1].colour + 1) % SIGN_COLOURS);
	}
	// A row reads left to right.
	const std::vector<SignQuad> row = inscriptionLayout(InscriptionStyle::Row, codes(7), sheet);
	for (size_t k = 1; k < row.size(); k++)
		CHECK(row[k].x0 >= row[k - 1].x1);
}

TEST_CASE("the stela: columns of 4 read right to left") {
	const HieroglyphSheet sheet = squares(12);
	const std::vector<SignQuad> quads = inscriptionLayout(InscriptionStyle::Stela, codes(9), sheet);
	REQUIRE(quads.size() == 9);
	CHECK(quads[0].x0 > quads[4].x1); // the first column is the right one
	CHECK(quads[4].x0 > quads[8].x1);
	CHECK(quads[1].y1 <= quads[0].y0); // each column top down
	CHECK(quads[0].x0 == doctest::Approx(quads[3].x0));
	std::set<float> columns;
	for (const SignQuad& q : quads)
		columns.insert(q.x0);
	CHECK(columns.size() == 3);
}

TEST_CASE("the manifest: codes in order, a bad line told by line, a missing file told") {
	const std::filesystem::path path = std::filesystem::temp_directory_path() / "hieroglyphs_test.txt";
	std::ofstream(path) << "# a comment\nA1 0.9\nG17\nN35 3.5\n";
	HieroglyphSheet sheet;
	const std::vector<std::string> errors = sheet.Load(path.string());
	std::filesystem::remove(path);
	REQUIRE(errors.size() == 1);
	CHECK(errors[0] == path.string() + ":3: expected a code and its width / height");
	CHECK(sheet.Size() == 3); // the bad line keeps its cell
	CHECK(sheet.Cell("A1") == 0);
	CHECK(sheet.Cell("N35") == 2);
	CHECK(sheet.Aspect(2) == doctest::Approx(3.5f));
	CHECK(sheet.Load("no-such-manifest.txt").at(0) == "no-such-manifest.txt: cannot be read");
}
