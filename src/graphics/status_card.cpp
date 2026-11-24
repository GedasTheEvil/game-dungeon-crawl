#include "status_card.h"
#include "../level/ui/hud_icon.h"
#include "../render/lighting.h"

using namespace ui;

namespace {
constexpr float CARD_MARGIN = 0.6f; // the card's frame outside its tile
using PlayerHud::ICON_COLUMNS;
using PlayerHud::ICON_ROWS;
} // namespace

namespace StatusCard {
// The card's parts are measured on the HUD's (HUD_SIZE across) and scaled to r: the same look at any size.
void draw(const Status& s, const Rect& r, int icons, int cards, bool pips) {
	const float k = r.w / HUD_SIZE;
	beginShapes();
	fillRect({r.x + 0.5f * k, r.y - 0.6f * k, r.w, r.h}, BLACK, BLACK, 0.45f); // drop shadow
	atlasIcon(r.inset(-CARD_MARGIN * k), cards, static_cast<int>(s.kind), STATUS_KIND_COUNT, 1, {1, 1, 1});
	atlasIcon(
		{r.x + 0.6f * k, r.y + 1.3f * k, r.w - 1.2f * k, r.h - 1.6f * k},
		icons,
		static_cast<int>(s.icon),
		ICON_COLUMNS,
		ICON_ROWS,
		{s.tint.r, s.tint.g, s.tint.b}
	);
	{
		// The world's current texture coordinate (Lighting::batch) as the legacy GL atlasIcon left it, its last
		// corner (u0, v1): the next monster's health bar outline samples its texture there (plans gl-core-ui,
		// gl-core-world; the snapshots keep that picture).
		const float w = 1.f / static_cast<float>(ICON_COLUMNS);
		const float h = 1.f / static_cast<float>(ICON_ROWS);
		const int cell = static_cast<int>(s.icon);
		const int row = cell / ICON_COLUMNS;
		const float v = 1.f - h * static_cast<float>(row + 1);
		Lighting::batch().texCoord(w * static_cast<float>(cell % ICON_COLUMNS), v + h);
	}
	beginShapes();
	if (!pips)
		return;
	for (int p = 0; p < s.pips; p++)
		diamond(
			r.cx() + (static_cast<float>(p) - static_cast<float>(s.pips - 1) / 2.f) * 1.4f * k,
			r.y + 0.9f * k,
			0.45f * k,
			GOLD_BRIGHT,
			1.f
		);
}
} // namespace StatusCard
