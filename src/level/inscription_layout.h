#ifndef INSCRIPTION_LAYOUT_H
#define INSCRIPTION_LAYOUT_H

// Composed inscriptions (plan composed-inscriptions): the game draws an inscription from its plate (the empty frame of
// its style, media/textures/decorations/inscription_plates.png) and a quad per sign from the hieroglyph sheet
// (hieroglyphs.png, its manifest hieroglyphs.txt), both baked by tools/textures/inscriptions.py. Here the sheet's
// manifest and where the signs go on the plate; no GL, so the tests check the layout.

#include "../core/paths.h"
#include "decor.h"
#include "inscriptions.h"
#include "names.h"
#include <string>
#include <unordered_map>
#include <vector>

constexpr const char* HIEROGLYPH_MANIFEST = MEDIA_DIR "textures/decorations/hieroglyphs.txt";
constexpr int PLATE_GRID = 2; // inscription_plates.png: a plate per InscriptionStyle, row by row from the top left
constexpr int HIEROGLYPH_SHEET_GRID = 16; // hieroglyphs.png: a sign per cell, manifest order
constexpr float HIEROGLYPH_GUTTER = 4.f / 128.f; // the share of a cell kept clear around its sign

// The signs' paint, cycled per sign: blue, red, green, black, gold (tools/textures/decals.py PAINT). The sheet's
// signs are white with an ink outline, so this tints the fill and leaves the ink dark.
constexpr int SIGN_COLOURS = 5;
constexpr float SIGN_PAINT[SIGN_COLOURS][3] = {
	{0.18f, 0.34f, 0.59f},
	{0.63f, 0.24f, 0.13f},
	{0.21f, 0.47f, 0.35f},
	{0.12f, 0.09f, 0.08f},
	{0.80f, 0.60f, 0.19f},
};

class HieroglyphSheet {
  public:
	// The manifest: per line a Gardiner code and the sign's width / height. Returns the problems found.
	std::vector<std::string> Load(const std::string& path);
	void Add(const std::string& code, float aspect);
	// The cell of a code, -1 for one the sheet lacks.
	[[nodiscard]] int Cell(const std::string& code) const;
	[[nodiscard]] float Aspect(int cell) const { return aspects.at(static_cast<size_t>(cell)); }
	// The sign's own box in the sheet: its cell less the gutter, narrowed to the sign's width / height.
	[[nodiscard]] AtlasRect Uv(int cell) const;
	[[nodiscard]] size_t Size() const { return aspects.size(); }
	// "file:line: no sign X in the hieroglyph sheet" for each code of a G: line it lacks.
	[[nodiscard]] std::vector<std::string>
	Missing(const std::vector<std::string>& glyphs, const std::string& source) const;

  private:
	std::unordered_map<std::string, int> cells;
	std::vector<float> aspects;
};

// Every G: line of the texts and the names checked against the sheet: a stale sheet shows up in game.log.
[[nodiscard]] std::vector<std::string>
checkSigns(const HieroglyphSheet& sheet, const InscriptionBook& book, const NameList& names);

// Up to this many signs fit a style's plate; the rest are not drawn.
[[nodiscard]] int signCap(InscriptionStyle style);

// A sign on the plate: its sheet cell, its rect in the plate's 0..1 space (y up) and its SIGN_PAINT colour.
struct SignQuad {
	int cell;
	float x0, y0, x1, y1;
	int colour;
};

// Where the signs go on the style's plate: a column (cartouche, column), a row, or the stela's columns of 4 read
// right to left. Codes the sheet lacks are left out; each sign keeps its width / height, centred in its slot.
[[nodiscard]] std::vector<SignQuad>
inscriptionLayout(InscriptionStyle style, const std::vector<std::string>& glyphs, const HieroglyphSheet& sheet);

#endif
