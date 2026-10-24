#include "inscription_layout.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace {
// The plates are drawn at 512 px (tools/textures/inscriptions.py); the slots below are in those pixels, y down.
constexpr float PLATE_PX = 512.f;

// A slot of the plate in its pixels, y down.
struct Slot {
	float x, y, w, h;
};

// The sign fitted into the slot, its width / height kept, centred.
SignQuad place(int cell, float aspect, Slot s, int colour) {
	float w = s.w;
	float h = s.w / aspect;
	if (h > s.h) {
		h = s.h;
		w = s.h * aspect;
	}
	const float x = s.x + (s.w - w) / 2.f;
	const float y = s.y + (s.h - h) / 2.f;
	return {cell, x / PLATE_PX, 1.f - (y + h) / PLATE_PX, (x + w) / PLATE_PX, 1.f - y / PLATE_PX, colour};
}

// The signs in a column of the box (x0, y0)-(x1, y1), one slot each, colours from `firstColour` on.
void column(
	std::vector<SignQuad>& out,
	const std::vector<int>& cells,
	const HieroglyphSheet& sheet,
	float x0,
	float y0,
	float x1,
	float y1,
	int firstColour
) {
	const float slot = (y1 - y0) / static_cast<float>(std::max<size_t>(1, cells.size()));
	for (size_t k = 0; k < cells.size(); k++) {
		const Slot s{x0, y0 + static_cast<float>(k) * slot + slot * 0.06f, x1 - x0, slot * 0.88f};
		out.push_back(place(cells[k], sheet.Aspect(cells[k]), s, (static_cast<int>(k) + firstColour) % SIGN_COLOURS));
	}
}
} // namespace

std::vector<std::string> HieroglyphSheet::Load(const std::string& path) {
	cells.clear();
	aspects.clear();
	std::vector<std::string> errors;
	std::ifstream in(path);
	if (!in)
		return {path + ": cannot be read"};
	std::string raw;
	int lineNo = 0;
	while (std::getline(in, raw)) {
		lineNo++;
		std::istringstream line(raw);
		std::string code;
		float aspect = 0.f;
		if (!(line >> code) || code[0] == '#')
			continue;
		if (!(line >> aspect) || aspect <= 0.f) {
			errors.push_back(path + ":" + std::to_string(lineNo) + ": expected a code and its width / height");
			aspect = 1.f; // keeps the cell order
		}
		Add(code, aspect);
	}
	return errors;
}

void HieroglyphSheet::Add(const std::string& code, float aspect) {
	cells.emplace(code, static_cast<int>(aspects.size()));
	aspects.push_back(aspect);
}

int HieroglyphSheet::Cell(const std::string& code) const {
	const auto found = cells.find(code);
	return found == cells.end() ? -1 : found->second;
}

AtlasRect HieroglyphSheet::Uv(int cell) const {
	AtlasRect uv = atlasCell(cell, HIEROGLYPH_SHEET_GRID);
	const float side = (uv.u1 - uv.u0) * (1.f - 2.f * HIEROGLYPH_GUTTER);
	const float aspect = Aspect(cell);
	const float w = aspect >= 1.f ? side : side * aspect;
	const float h = aspect >= 1.f ? side / aspect : side;
	const float cu = (uv.u0 + uv.u1) / 2.f;
	const float cv = (uv.v0 + uv.v1) / 2.f;
	return {cu - w / 2.f, cv - h / 2.f, cu + w / 2.f, cv + h / 2.f};
}

std::vector<std::string>
HieroglyphSheet::Missing(const std::vector<std::string>& glyphs, const std::string& source) const {
	std::vector<std::string> errors;
	for (const std::string& code : glyphs)
		if (Cell(code) < 0)
			errors.push_back(
				source + ": no sign " + code + " in the hieroglyph sheet (rerun tools/textures/inscriptions.py)"
			);
	return errors;
}

std::vector<std::string> checkSigns(const HieroglyphSheet& sheet, const InscriptionBook& book, const NameList& names) {
	std::vector<std::string> errors;
	auto add = [&](const std::vector<std::string>& found) { errors.insert(errors.end(), found.begin(), found.end()); };
	for (const InscriptionText& text : book.All())
		add(sheet.Missing(text.glyphs, text.glyphSource));
	for (size_t i = 0; i < names.Size(); i++)
		add(sheet.Missing(names.At(i).glyphs, names.At(i).glyphSource));
	return errors;
}

int signCap(InscriptionStyle style) {
	switch (style) {
	case InscriptionStyle::Cartouche:
		return 5;
	case InscriptionStyle::Column:
	case InscriptionStyle::Row:
		return 7;
	case InscriptionStyle::Stela:
		return 12;
	}
	return 0;
}

std::vector<SignQuad>
inscriptionLayout(InscriptionStyle style, const std::vector<std::string>& glyphs, const HieroglyphSheet& sheet) {
	std::vector<int> cells;
	for (const std::string& code : glyphs)
		if (const int cell = sheet.Cell(code); cell >= 0 && static_cast<int>(cells.size()) < signCap(style))
			cells.push_back(cell);
	std::vector<SignQuad> out;
	switch (style) {
	case InscriptionStyle::Cartouche:
		column(out, cells, sheet, 196, 66, 316, 412, 0);
		break;
	case InscriptionStyle::Column:
		column(out, cells, sheet, 182, 30, 330, 482, 1);
		break;
	case InscriptionStyle::Row: {
		const float slot = (476.f - 36.f) / static_cast<float>(std::max<size_t>(1, cells.size()));
		for (size_t k = 0; k < cells.size(); k++) {
			const Slot s{36 + static_cast<float>(k) * slot + slot * 0.05f, 192, slot * 0.9f, 128};
			out.push_back(place(cells[k], sheet.Aspect(cells[k]), s, (static_cast<int>(k) + 2) % SIGN_COLOURS));
		}
		break;
	}
	case InscriptionStyle::Stela: { // columns of 4, read from the right, as on most stelae
		constexpr size_t PER = 4;
		const size_t cols = std::max<size_t>(1, (cells.size() + PER - 1) / PER);
		const float width = (400.f - 112.f) / static_cast<float>(cols);
		for (size_t c = 0; c < cols; c++) {
			const float x1 = 400.f - static_cast<float>(c) * width;
			const auto first = cells.begin() + static_cast<std::ptrdiff_t>(std::min(cells.size(), c * PER));
			const auto last = cells.begin() + static_cast<std::ptrdiff_t>(std::min(cells.size(), (c + 1) * PER));
			column(out, std::vector<int>(first, last), sheet, x1 - width + 8, 176, x1 - 8, 488, static_cast<int>(c));
		}
		break;
	}
	}
	return out;
}
