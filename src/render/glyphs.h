#ifndef GLYPHS_H
#define GLYPHS_H

#include <array>

// The characters the fonts draw: printable ASCII from ' ' as glyphs 0-94, then the gamepad's symbols (plan
// sdl3-gamepad) as 95-102, written as the control characters 1-8 (PadGlyph in bindings.h): the PlayStation face
// buttons' cross, circle, square and triangle, then the d-pad's arrows up, down, left and right. A sheet (10 x 10
// cells) holds the symbols in its cells 95-99 (tools/textures/font_sheet.py), the up arrow in 99 and the other arrows
// as it turned (sheetCell); a TrueType font has them at their Unicode code points.
constexpr int ASCII_GLYPHS = 95;
constexpr std::array<unsigned long, 8> PAD_CODE_POINTS = {
	0x2715, // cross
	0x25CB, // circle
	0x25A1, // square
	0x25B3, // triangle
	0x25B2, // the d-pad's arrows, filled
	0x25BC,
	0x25C0,
	0x25B6,
};
constexpr int FONT_GLYPHS = ASCII_GLYPHS + static_cast<int>(PAD_CODE_POINTS.size());

constexpr int SHEET_CELLS = 100;
constexpr int ARROW_CELL = 99;

// Where a glyph sits in a sheet: its cell, and the quarter turns (counter-clockwise) it is drawn turned by.
struct SheetCell {
	int cell;
	int quarterTurns;
};
[[nodiscard]] constexpr SheetCell sheetCell(int glyph) {
	if (glyph < ARROW_CELL)
		return {glyph, 0};
	constexpr std::array<int, 4> TURNS = {0, 2, 1, 3}; // up as drawn, down, left, right
	return {ARROW_CELL, TURNS[static_cast<unsigned long>(glyph - ARROW_CELL)]};
}

// A character's glyph, -1 for one the fonts do not draw (it is skipped: no ink, no advance).
[[nodiscard]] constexpr int glyphOf(unsigned char c) {
	if (c >= ' ' && c < 127)
		return c - ' ';
	if (c >= 1 && c <= PAD_CODE_POINTS.size()) // never a line break: 9 and 10 are past them
		return ASCII_GLYPHS + c - 1;
	return -1;
}

// A glyph's Unicode code point.
[[nodiscard]] constexpr unsigned long glyphCodePoint(int glyph) {
	return glyph < ASCII_GLYPHS ? static_cast<unsigned long>(glyph) + ' '
								: PAD_CODE_POINTS[static_cast<unsigned long>(glyph - ASCII_GLYPHS)];
}

#endif
