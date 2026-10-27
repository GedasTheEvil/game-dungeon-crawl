#ifndef FONT_H
#define FONT_H
#include <memory>

struct FontSheet; // font.cpp: a sheet's texture and glyph edges, shared by every size loaded from it

class Font {
  private:
	static constexpr int GLYPHS = 95; // printable ASCII from ' '
	std::shared_ptr<const FontSheet> sheet;
	int base;
	float quad = 0; // the glyph quad's side (Load)
	float advance[GLYPHS] = {};

  public:
	// The sheet layout (tools/textures/font_sheet.py), in px of a glyph quad (a 51.2 px tenth of the 512 px sheet):
	// the baseline sits this far above the quad bottom, the cap height reaches this far above the baseline. Papyrus,
	// kalam and courier share it; impact (the HUD numbers) has a lower cap height (19 px).
	static constexpr float QUAD_PX = 51.2f;
	static constexpr float BASELINE_PX = 18.f;
	static constexpr float CAP_HEIGHT_PX = 24.f;

	Font();
	~Font();
	Font(const Font&) = delete; // owns display lists: a copy would free them twice
	Font& operator=(const Font&) = delete;
	// Monospaced: every glyph advances size / 2 + spacing.
	// Proportional: each glyph advances its own inked width + spacing (measured from the texture).
	// Fonts of one file share its texture while any of them lives.
	void Load(const char filename[], float size = 12, float spacing = 0, bool proportional = false);
	void print(float x, float y, const char* string, ...);
	[[nodiscard]] float TextWidth(const char* text) const;
	// print() puts the quad bottom at y: the baseline is Baseline() above it, the cap top CapHeight() above that.
	[[nodiscard]] float Baseline() const { return quad * BASELINE_PX / QUAD_PX; }
	[[nodiscard]] float CapHeight() const { return quad * CAP_HEIGHT_PX / QUAD_PX; }
};

#endif
