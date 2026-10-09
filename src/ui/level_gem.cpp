#include "level_gem.h"
#include "ui_draw.h"
#include "../graphics/font.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <string>

using namespace ui;

namespace {
constexpr float SCALE = 0.25f;	   // badge size on screen; everything below is in badge units (canvas 100 / SCALE high)
constexpr int SIDES = 8;		   // octagonal cut
constexpr float RADIUS = 5.5f;	   // gem
constexpr float STRETCH_Y = 1.15f; // a little taller than wide, like a cabochon
constexpr float BEZEL = 1.1f;	   // gold rim around the gem
constexpr float TABLE = 0.55f;	   // flat top facet, relative to the gem
constexpr float MARGIN_X = 10.f;   // from the right edge to the bezel
constexpr float MARGIN_Y = 10.f;   // from the top edge to the bezel
constexpr float NUMBER_DROP = 5.4f; // font pen y below the gem centre, so the digits sit in the middle

struct GemLook {
	Color dark, base, light;
	int beads;	 // gold beads on the bezel
	bool flecks; // golden flecks: pyrite in lapis lazuli, gold dust on obsidian
};

constexpr GemLook GEMS[] = {
	{{0.35f, 0.06f, 0.02f}, {0.78f, 0.22f, 0.08f}, {1.f, 0.55f, 0.30f}, 0, false},	  // carnelian
	{{0.04f, 0.30f, 0.30f}, {0.18f, 0.70f, 0.66f}, {0.60f, 0.95f, 0.88f}, 4, false},  // turquoise
	{{0.04f, 0.07f, 0.28f}, {0.14f, 0.24f, 0.66f}, {0.45f, 0.58f, 1.f}, 8, true},	  // lapis lazuli
	{{0.02f, 0.20f, 0.10f}, {0.08f, 0.52f, 0.30f}, {0.50f, 0.90f, 0.62f}, 10, false}, // malachite
	{{0.20f, 0.05f, 0.28f}, {0.48f, 0.20f, 0.66f}, {0.82f, 0.62f, 1.f}, 12, false},	  // amethyst
	{{0.02f, 0.02f, 0.03f}, {0.12f, 0.11f, 0.14f}, {0.48f, 0.46f, 0.55f}, 16, true},  // obsidian
};
static_assert(sizeof(GEMS) / sizeof(GEMS[0]) == LevelGem::GEM_COUNT);

float cornerAngle(int i) { return 2.f * static_cast<float>(M_PI) * (static_cast<float>(i) + 0.5f) / SIDES; }

// Octagon of radius r around (cx, cy), fan-shaded from `centre` to `edge`.
void octagon(float cx, float cy, float r, Color centre, Color edge) {
	glBegin(GL_TRIANGLE_FAN);
	glColor4f(centre.r, centre.g, centre.b, 1.f);
	glVertex2f(cx, cy);
	glColor4f(edge.r, edge.g, edge.b, 1.f);
	for (int i = 0; i <= SIDES; i++)
		glVertex2f(cx + r * std::cos(cornerAngle(i)), cy + r * STRETCH_Y * std::sin(cornerAngle(i)));
	glEnd();
}

// Crown facets between the table and the girdle: the upper left ones catch the light, the lower right are in shade.
void facets(float cx, float cy, const GemLook& gem) {
	glBegin(GL_QUADS);
	for (int i = 0; i < SIDES; i++) {
		float mid = (cornerAngle(i) + cornerAngle(i + 1)) / 2;
		float lit = 0.5f + 0.5f * std::cos(mid - 2.3f); // light from the upper left
		Color c = {gem.base.r + (gem.light.r - gem.base.r) * lit - (1 - lit) * (gem.base.r - gem.dark.r) * 0.6f,
				   gem.base.g + (gem.light.g - gem.base.g) * lit - (1 - lit) * (gem.base.g - gem.dark.g) * 0.6f,
				   gem.base.b + (gem.light.b - gem.base.b) * lit - (1 - lit) * (gem.base.b - gem.dark.b) * 0.6f};
		glColor4f(c.r, c.g, c.b, 1.f);
		for (float r : {RADIUS, RADIUS * TABLE})
			for (int k : {0, 1}) {
				int corner = r == RADIUS ? i + k : i + 1 - k;
				glVertex2f(cx + r * std::cos(cornerAngle(corner)), cy + r * STRETCH_Y * std::sin(cornerAngle(corner)));
			}
	}
	glEnd();
}
} // namespace

namespace LevelGem {
void draw(int level, int resX, int resY, Font& font) {
	const GemLook& gem = GEMS[LevelGem::GemOf(level)];
	// Font::print resets the modelview, so the badge is scaled through the projection.
	float canvasH = 100.f / SCALE;
	float canvasW = beginSquareCanvas(canvasH, resX, resY);

	float outer = RADIUS + BEZEL;
	float cx = canvasW - MARGIN_X - outer;
	float cy = canvasH - MARGIN_Y - outer * STRETCH_Y;

	beginShapes();
	ellipse(cx, cy, outer * 1.6f, outer * STRETCH_Y * 1.6f, BLACK, 0.55f); // soft shadow behind
	octagon(cx, cy, outer, GOLD_BRIGHT, BRONZE);
	octagon(cx, cy, RADIUS, gem.base, gem.dark);
	facets(cx, cy, gem);
	octagon(cx, cy, RADIUS * TABLE, gem.light, gem.base);
	if (gem.flecks)
		for (auto [fx, fy] : {std::pair{-0.45f, 0.5f}, {0.55f, 0.25f}, {-0.2f, -0.6f}, {0.35f, -0.45f}})
			diamond(cx + fx * RADIUS, cy + fy * RADIUS * STRETCH_Y, 0.28f, GOLD_BRIGHT, 0.9f);
	for (int i = 0; i < gem.beads; i++) {
		float a = 2.f * static_cast<float>(M_PI) * static_cast<float>(i) / static_cast<float>(gem.beads);
		float bx = cx + (RADIUS + BEZEL / 2) * std::cos(a);
		float by = cy + (RADIUS + BEZEL / 2) * STRETCH_Y * std::sin(a);
		diamond(bx, by, 0.85f, GOLD_BRIGHT, 1.f);
		diamond(bx, by, 0.35f, GOLD_DIM, 1.f);
	}
	ellipse(cx - RADIUS * 0.35f, cy + RADIUS * 0.5f, RADIUS * 0.3f, RADIUS * 0.18f, {1.f, 1.f, 1.f}, 0.7f); // glint

	std::string number = std::to_string(level);
	beginText();
	float textX = cx - font.TextWidth(number.c_str()) / 2;
	float textY = cy - NUMBER_DROP;
	text(font, textX + 0.35f, textY - 0.35f, number.c_str(), BLACK); // drop shadow
	text(font, textX, textY, number.c_str(), GOLD_BRIGHT);

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glColor3f(1, 1, 1);
}
} // namespace LevelGem
