#ifndef FONT_H
#define FONT_H
#include <memory>

struct FontSheet; // font.cpp: a sheet's texture and glyph edges, shared by every size loaded from it

class Font {
  private:
	static constexpr int GLYPHS = 95; // printable ASCII from ' '
	std::shared_ptr<const FontSheet> sheet;
	int base;
	float advance[GLYPHS] = {};

  public:
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
};

#endif
