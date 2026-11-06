#ifndef DUNGEON_SAVE_H
#define DUNGEON_SAVE_H

#include "player.h"
#include "dungeon.h"
#include "../level/item_bag.h"
#include "../level/journal.h"
#include <array>
#include <istream>
#include <ostream>
#include <string>

// What the player set on the screens, kept in a save (plan save-remembers-ui-state): the way they look, the
// inventory's open tab and each tab's sort order, the journal's open section and each section's spread. The views
// hand it over (GameState::ViewState) and take it back after a load; the defaults are a new game's.
struct UiViewState {
	static constexpr int JOURNAL_SECTIONS = 7; // JournalScreen::SECTION_COUNT
	static constexpr int JOURNAL_CREATURES = 1; // the section a new book opens on (JournalScreen::Section)
	int facing = 1; // -1 left, +1 right
	ItemGroup tab = ItemGroup::Weapons;
	std::array<SortOrder, ITEM_GROUP_COUNT> sort{};
	int journalSection = JOURNAL_CREATURES;
	std::array<int, JOURNAL_SECTIONS> spread{};
	bool operator==(const UiViewState&) const = default;
};

// A save game (GameState::Save / LoadSave): the level number, the player's stats, the bag, the dungeon, the
// remembered settings (the stamina-skill toggle, lit arrows, the views) and the journal, in that order. Without the
// game, so the unit tests read and write one (plan save-load-safety).
struct SaveParts {
	Dungeon& dungeon;
	Player& player;
	ItemBag& bag;
	Journal& journal;
	UiViewState& view;
};

void writeSaveGame(std::ostream& out, const SaveParts& parts);
// Reads a save into the parts, the player reanimated first. False on a broken save: the parts are then half read.
[[nodiscard]] bool readSaveGame(std::istream& in, const SaveParts& parts);
// readSaveGame, but a broken save puts back what the parts held, as a save and a load of them would. The caller
// applies the rest (the inventory's selection, the level's decorations) after either outcome.
[[nodiscard]] bool loadSaveGame(const std::string& text, const SaveParts& parts);

// Writes `path` through `path`.tmp renamed over it: a failed or cut write leaves the old file as it was.
[[nodiscard]] bool writeFileSafely(const std::string& path, const std::string& text);
// The whole file; false if it cannot be read.
[[nodiscard]] bool readWholeFile(const std::string& path, std::string& text);

#endif
