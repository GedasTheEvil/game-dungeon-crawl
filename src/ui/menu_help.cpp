#include "menu.h"
#include <array>
#include <vector>
#include "../level/controls_help.h"
#include "../state/game_state.h"
#include "menu_internal.h"

// How to Play (plan controls-help): the keys as bound now, by group, read from the bindings (controlsHelp).

namespace menu_internal {
namespace {
// controlsHelp's groups (Move, Fight, Items, Screens, Saves) by column, top to bottom; -1 ends a column.
constexpr std::array<std::array<int, 2>, 3> COLUMNS = {{{0, 4}, {1, 2}, {3, -1}}};

float columnX(int column) { return HELP_LEFT + static_cast<float>(column) * (HELP_COLUMN_W + HELP_COLUMN_GAP); }
} // namespace

// The right column's foot: the button to Options > Controls over the hints switch.
Rect helpHintsSwitch() { return {columnX(2), HELP_PANEL.y + 4.f, HELP_COLUMN_W, HELP_TILE_H}; }

Rect helpKeysButton() {
	const Rect hints = helpHintsSwitch();
	return {hints.x, hints.y + HELP_TILE_H + 2.f, hints.w, HELP_TILE_H};
}
} // namespace menu_internal
using namespace menu_internal;

void MainMenu::DrawHelp() {
	const std::vector<HelpGroup> groups = controlsHelp(Game().settings.controls);
	for (int column = 0; column < static_cast<int>(COLUMNS.size()); column++) {
		const float x = columnX(column);
		float top = HELP_TOP;
		for (int g : COLUMNS[static_cast<size_t>(column)]) {
			if (g < 0)
				continue;
			const HelpGroup& group = groups[static_cast<size_t>(g)];
			beginText();
			text(body, x + 0.5f, top - 3.4f, group.title, GOLD);
			beginShapes();
			top -= 4.2f;
			line(x, top, x + HELP_COLUMN_W, top, BRONZE, 1.f, 1.f);
			for (size_t i = 0; i < group.rows.size(); i++) {
				const Rect row = {x, top - HELP_ROW_H, HELP_COLUMN_W, HELP_ROW_H};
				if (i % 2 == 0)
					fillRect(row, {1, 1, 1}, {1, 1, 1}, 0.035f);
				beginText();
				text(small, row.x + 1.f, row.y + 0.5f, group.rows[i].label.c_str(), LABEL);
				const float w = small.TextWidth(group.rows[i].keys.c_str());
				text(small, row.x + row.w - 1.f - w, row.y + 0.5f, group.rows[i].keys.c_str(), GOLD);
				beginShapes();
				top -= HELP_ROW_H;
			}
			top -= HELP_GROUP_GAP;
		}
	}

	const bool on = Game().settings.gameplay.hints;
	const bool keysHovered = pointer.hovered == HELP_KEYS;
	const Rect keys = tile(helpKeysButton(), TileStyle::Stone, keysHovered, pointer.Held(HELP_KEYS));
	const bool hintsHovered = pointer.hovered == HELP_HINTS;
	const Rect hints =
		tile(helpHintsSwitch(), on ? TileStyle::Lapis : TileStyle::Stone, hintsHovered, pointer.Held(HELP_HINTS));
	beginText();
	textCenteredIn(small, keys, "Change keys", keysHovered ? GOLD_BRIGHT : LABEL);
	textCenteredIn(
		small,
		hints,
		on ? "First hints: On" : "First hints: Off",
		hintsHovered ? GOLD_BRIGHT : (on ? GOLD : LABEL)
	);
	beginShapes();
}
