#ifndef EDITOR_DRESSING_BRUSH_H
#define EDITOR_DRESSING_BRUSH_H

// The editor's brushes for the decor and decal layers (plan placed-decor): what painting one does to a cell's
// dressing. Header only, no GL: the unit tests use it.

#include "../../src/world/decor.h"
#include "../../src/world/level.h"

// The decal palette's brushes besides PLACED_UNSET, PLACED_BARE and the decals (DECAL_DEFS indices): the torch field
// alone.
constexpr int TORCH_BRUSH = -3;
constexpr int NO_TORCH_BRUSH = -4;

// A prop brush: PLACED_UNSET, PLACED_BARE or a prop (DECOR_NAMES).
inline void paintProp(Dressing& d, int prop, bool mirror) {
	d.prop = prop;
	d.propMirror = prop >= 0 && mirror;
}

// A decal brush: Unset and Bare set both fields, Torch and No torch the torch only, a decal the decal only. A torch
// and a free decal would overlap on the wall: painting one clears the other.
inline void paintDecal(Dressing& d, int brush, bool mirror) {
	const auto freeDecal = [](int decal) { return decal >= 0 && DECAL_DEFS[decal].anchor == DecalAnchor::Free; };
	switch (brush) {
	case PLACED_UNSET:
		d.decal = PLACED_UNSET;
		d.decalMirror = false;
		d.torch = TorchPlacing::Unset;
		break;
	case PLACED_BARE:
		d.decal = PLACED_BARE;
		d.decalMirror = false;
		d.torch = TorchPlacing::None;
		break;
	case TORCH_BRUSH:
		d.torch = TorchPlacing::Torch;
		if (freeDecal(d.decal)) {
			d.decal = PLACED_UNSET;
			d.decalMirror = false;
		}
		break;
	case NO_TORCH_BRUSH:
		d.torch = TorchPlacing::None;
		break;
	default:
		d.decal = brush;
		d.decalMirror = mirror;
		if (freeDecal(brush) && d.torch == TorchPlacing::Torch)
			d.torch = TorchPlacing::Unset;
		break;
	}
}

// The decal brush that picks up the cell's decal layer: its decal if it has one set, else its torch field.
[[nodiscard]] inline int pickDecalBrush(const Dressing& d) {
	if (d.decal != PLACED_UNSET)
		return d.decal;
	if (d.torch == TorchPlacing::Torch)
		return TORCH_BRUSH;
	return d.torch == TorchPlacing::None ? NO_TORCH_BRUSH : PLACED_UNSET;
}

#endif
