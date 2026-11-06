#include "menu.h"
#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>
#include "../state/game_state.h"
#include "menu_internal.h"

namespace menu_internal {
// ---- save slots ------------------------------------------------------------

// A used slot's delete button, in its top right corner.
Rect deleteRect(int slot, bool load) {
	const Rect r = slotRect(slot, load);
	constexpr float SIZE = 4.5f;
	return {r.x + r.w - SIZE - 1.5f, r.y + r.h - SIZE - 1.5f, SIZE, SIZE};
}

// Load Game's slot, autosave or quick save: what the toast and the confirm box call it.
std::string saveName(int target) {
	if (target == AUTOSAVE_TARGET)
		return "the autosave";
	if (target == QUICKSAVE_TARGET)
		return "the quick save";
	return "slot " + std::to_string(target + 1);
}

std::string pathOf(int target) {
	if (target == AUTOSAVE_TARGET)
		return Game().AutosavePath();
	if (target == QUICKSAVE_TARGET)
		return Game().QuicksavePath();
	return Game().SlotPath(target);
}
} // namespace menu_internal
using namespace menu_internal;

void MainMenu::DrawSlots() {
	if (loadD) {
		DrawSlot(AUTOSAVE_TARGET);
		DrawSlot(QUICKSAVE_TARGET);
	}
	for (int slot = 0; slot < SaveSlots::COUNT; slot++)
		DrawSlot(slot);
}

// A save slot, or the autosave or the quick save (AUTOSAVE_TARGET, QUICKSAVE_TARGET) in Load Game. The text sits as on
// the Save Game's taller tiles, scaled to the tile: the level and its zone, under it the date, the character level and
// the play time.
void MainMenu::DrawSlot(int slot) {
	const bool quicksave = slot == QUICKSAVE_TARGET;
	const bool autosave = slot == AUTOSAVE_TARGET || quicksave; // the game's own files, read-only
	const SaveSlots::Info info = SaveSlots::DescribeFile(pathOf(slot));
	bool enabled = saveD || info.used;
	bool isHovered = enabled && pointer.hovered == slot;
	bool held = isHovered && pointer.Held(slot);
	TileStyle style = !enabled ? TileStyle::Disabled : TileStyle::Stone;
	Rect r = tile(slotRect(slot, loadD), style, isHovered, held);
	const float k = r.h / SLOT_H;

	// Slot number (the autosave: an ankh) on a lapis seal when the slot holds a save.
	Rect well = iconWell(r, 2.f * k);
	if (info.used)
		fillRect(well, isHovered ? LAPIS_HOVER_TOP : LAPIS, isHovered ? LAPIS_HOVER_BOTTOM : LAPIS_DARK, 1.f);
	else
		fillRect(well, WELL, WELL, 0.9f);
	strokeRect(well, info.used ? GOLD : BRONZE, 1.f, info.used ? 1.5f : 1.f);
	if (autosave)
		drawIcon(
			quicksave ? Icon::Save : Icon::Ankh,
			well.cx(),
			well.cy(),
			well.h * 0.3f,
			info.used ? GOLD : LABEL_DIM
		);

	const bool deletable = info.used && !autosave;
	float textX = well.x + well.w + 3.5f;
	const float textRight = r.x + r.w - (deletable ? 8.f : 3.f);
	if (info.used) // divider between the level and the date
		line(textX, r.y + 7.4f * k, r.x + r.w - 3.f, r.y + 7.4f * k, BRONZE, 1.f, 1.f);
	if (deletable) { // a cross on a small stone tile
		const int target = DELETE_BASE + slot;
		const bool deleteHovered = pointer.hovered == target;
		const Rect d = tile(deleteRect(slot, loadD), TileStyle::Stone, deleteHovered, pointer.Held(target));
		const Color c = deleteHovered ? Color{0.95f, 0.5f, 0.35f} : LABEL_DIM;
		const float s = d.w * 0.25f;
		line(d.cx() - s, d.cy() - s, d.cx() + s, d.cy() + s, c, 1.f, 1.5f);
		line(d.cx() - s, d.cy() + s, d.cx() + s, d.cy() - s, c, 1.f, 1.5f);
	}

	beginText();
	if (!autosave) {
		char number[12];
		std::snprintf(number, sizeof(number), "%d", slot + 1);
		textCenteredIn(title, well, number, info.used ? GOLD : LABEL_DIM);
	}

	if (info.used) {
		std::string level = "Saved game";
		if (autosave)
			level = std::string(quicksave ? "Quick save" : "Autosave") +
					(info.level > 0 ? ", level " + std::to_string(info.level) : "");
		else if (info.level > 0)
			level = "Level " + std::to_string(info.level);
		text(heading, textX, r.y + 8.8f * k, level.c_str(), isHovered ? GOLD_BRIGHT : GOLD);
		if (const Zone* zone = zoneOf(info.level)) { // plan save-load-safety: the zone after it, smaller, if it fits
			const char* name = zoneInfo(zone->id).name;
			const float zoneX = textX + heading.TextWidth(level.c_str()) + 2.f;
			if (zoneX + body.TextWidth(name) < textRight)
				text(body, zoneX, r.y + 8.8f * k, name, isHovered ? LABEL : LABEL_DIM);
		}
		text(small, textX, r.y + 3.f * k, info.when.c_str(), isHovered ? LABEL : LABEL_DIM);
		std::string run;
		if (info.characterLevel > 0)
			run = "Lv " + std::to_string(info.characterLevel);
		if (info.playMs >= 0)
			run += (run.empty() ? "" : ", ") + SaveSlots::PlayTime(info.playMs);
		if (!run.empty())
			text(
				small,
				r.x + r.w - 3.f - small.TextWidth(run.c_str()),
				r.y + 3.f * k,
				run.c_str(),
				isHovered ? LABEL : LABEL_DIM
			);
	} else {
		textMiddle(
			heading,
			textX,
			r.cy(),
			quicksave  ? "No quick save yet"
			: autosave ? "No autosave yet"
					   : "Empty slot",
			isHovered ? GOLD_BRIGHT : LABEL_DIM
		);
	}
	beginShapes();
}

// A failed save stays on Save Game with the old file kept (GameState::Save).
void MainMenu::SaveToSlot(int slot) {
	const std::string name = "slot " + std::to_string(slot + 1);
	if (!Game().Save(Game().SlotPath(slot).c_str())) {
		ShowToast("Could not save to " + name);
		return;
	}
	ResetSubScreens();
	ShowToast("Saved to " + name);
}

// A save that cannot be loaded leaves the game as it was, on Load Game.
void MainMenu::LoadFrom(int target) {
	if (!Game().LoadSaveIntoGame(pathOf(target)))
		ShowToast("Could not load " + saveName(target));
}

void MainMenu::DeleteSlot(int slot) {
	std::error_code error;
	std::filesystem::remove(Game().SlotPath(slot), error);
	ShowToast(
		error ? "Could not delete slot " + std::to_string(slot + 1) : "Slot " + std::to_string(slot + 1) + " deleted"
	);
}

// Delete on a used slot (or its delete button): as a click on the delete button.
void MainMenu::AskDelete() {
	const int slot = pointer.hovered >= DELETE_BASE ? pointer.hovered - DELETE_BASE : pointer.hovered;
	if (slot >= 0 && slot < SaveSlots::COUNT && SaveSlots::DescribeFile(Game().SlotPath(slot)).used)
		Activate(DELETE_BASE + slot);
}
