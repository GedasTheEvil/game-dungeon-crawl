#ifndef EDITOR_SHAPE_BRUSH_H
#define EDITOR_SHAPE_BRUSH_H

// The editor's shape brushes (plan level-subcells), in the structure layer's palette after the structures: the
// common shapes, painted on a whole cell, and two brushes that make the sub-cell under the mouse solid or open.
// Header only, no GL: the unit tests use it.

#include "../../src/level/level.h"
#include <cstdint>
#include <iterator>

enum class ShapeTool : std::uint8_t { Whole, SolidSubcell, OpenSubcell };

struct ShapeBrush {
	const char* label; // short, for the palette
	const char* description;
	std::uint16_t mask; // a Whole brush's shape
	ShapeTool tool = ShapeTool::Whole;
};

constexpr std::uint16_t FULL_SHAPE = 0xffff;

// Masks: bit = sub-column + 4 x sub-row, sub-row 0 at the bottom (level.h).
constexpr ShapeBrush SHAPE_BRUSHES[] = {
	{"No shape", "Takes the shape off the cell: it is just its structure again.", NO_SHAPE},
	{"+ Sub", "Makes the sub-cell under the mouse solid (drag to paint more).", 0, ShapeTool::SolidSubcell},
	{"- Sub",
	 "Makes the sub-cell under the mouse open; on rock it carves the cell open around it.",
	 0,
	 ShapeTool::OpenSubcell},
	{"Step", "A step a quarter high over the whole floor: a jump gets the player onto it.", 0x000f},
	{"Step L", "A step a quarter high on the left half of the floor.", 0x0003},
	{"Step R", "A step a quarter high on the right half of the floor.", 0x000c},
	{"Half", "A block half a cell high over the whole floor: too high for one jump.", 0x00ff},
	{"Half L", "A block half a cell high on the left half of the floor.", 0x0033},
	{"Half R", "A block half a cell high on the right half of the floor.", 0x00cc},
	{"Stair R", "A stair of quarter steps rising to the right, three steps.", 0x08ce},
	{"Stair L", "A stair of quarter steps rising to the left, three steps.", 0x0137},
	{"Low 1", "A ceiling a quarter lower: three quarters of head room.", 0xf000},
	{"Low 2", "A ceiling half a cell lower: half a cell of head room, a crawl space.", 0xff00},
	{"Low 3", "A ceiling three quarters lower: a quarter of head room, a crawl space.", 0xfff0},
	{"Lip L", "A lip on the left edge: a rim a quarter high and a quarter wide, as at the edge of a ledge.", 0x0001},
	{"Lip R", "A lip on the right edge: a rim a quarter high and a quarter wide.", 0x0008},
};
constexpr int SHAPE_BRUSH_COUNT = static_cast<int>(std::size(SHAPE_BRUSHES));

// Paints the brush on the cell; (subCol, subRow) is the sub-cell under the mouse, for the sub-cell brushes. Rock and
// deep water count as all solid: a shape that leaves part of them open carves the cell open (Empty). No shape keeps
// the structure.
inline void paintShape(Tile& cell, const ShapeBrush& brush, int subCol, int subRow) {
	const bool open = takesShape(cell.structure);
	auto shape = static_cast<std::uint16_t>(open ? cell.shape : FULL_SHAPE);
	const std::uint16_t bit = subcellBit(subCol, subRow);
	switch (brush.tool) {
	case ShapeTool::Whole:
		shape = brush.mask;
		break;
	case ShapeTool::SolidSubcell:
		shape = static_cast<std::uint16_t>(shape | bit);
		break;
	case ShapeTool::OpenSubcell:
		shape = static_cast<std::uint16_t>(shape & ~bit);
		break;
	}
	if (!open && (shape == FULL_SHAPE || shape == NO_SHAPE))
		return;
	if (!open)
		cell.structure = Structure::Empty;
	cell.shape = shape;
}

// The Whole brush with this shape, -1 for none (a shape painted by hand).
[[nodiscard]] inline int shapeBrushOf(std::uint16_t mask) {
	for (int i = 0; i < SHAPE_BRUSH_COUNT; i++)
		if (SHAPE_BRUSHES[i].tool == ShapeTool::Whole && SHAPE_BRUSHES[i].mask == mask)
			return i;
	return -1;
}

#endif
