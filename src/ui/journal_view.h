#ifndef JOURNAL_VIEW_H
#define JOURNAL_VIEW_H

#include "../graphics/font.h"
#include "../graphics/textures.h"
#include "../world/level.h"
#include "ui_draw.h"
#include <array>
#include <cstdint>
#include <string>

// The archaeologist's journal ([J]): a book open on two pages, a coloured ribbon bookmark per section on its right
// edge. The data is Game().journal (world/journal.h); this is only the screen. The game is paused while it is open.
// It opens on the section and pages looked at last.
class JournalScreen {
  public:
	enum class Section : std::uint8_t { Creatures, Riddles, FieldNotes };
	static constexpr int SECTION_COUNT = 3;

	void Draw();
	void SpecialKeyPressed(int key);
	void MouseFunction(int button, int state, int x, int y);
	void MouseMotion(int x, int y);
	void ShowToast(const std::string& text);

  private:
	// Clickable things: the ribbons, the page corners, the Answer button of each page.
	enum class Target : std::uint8_t { None, Ribbon, PrevPage, NextPage, AnswerLeft, AnswerRight };
	struct Hit {
		Target target = Target::None;
		int ribbon = 0;
		bool operator==(const Hit& o) const { return target == o.target && ribbon == o.ribbon; }
	};

	bool fontsLoaded = false;
	Font title, heading, body, small;
	Font handHeading, hand, handSmall; // the archaeologist's handwriting, for what is written on the pages
	Section section = Section::Creatures;
	std::array<int, SECTION_COUNT> spread{}; // the spread open in each section
	Hit hovered;
	Hit pressed;
	ui::Toast toast;
	std::array<Texture, MONSTER_TYPE_MAX + 1> sketchTextures; // monster textures as a pencil wash, by type, on demand

	[[nodiscard]] int PageCount(Section s) const;
	[[nodiscard]] int SpreadCount(Section s) const;
	void Turn(int by);
	// Journal riddle index on the left (0) or right (1) page of the open spread, -1 if none.
	[[nodiscard]] int RiddleOnPage(int side) const;
	// Index in the open section's list on the left (0) or right (1) page, -1 if the page is empty.
	[[nodiscard]] int EntryOnPage(int side) const;
	[[nodiscard]] bool CanAnswer(int side) const;
	void Activate(const Hit& hit);
	[[nodiscard]] Hit HitAt(float x, float y) const;

	void DrawBook();
	void DrawRibbons();
	void DrawPage(int side);
	void DrawCreature(const ui::Rect& page, int index);
	void DrawFieldNote(const ui::Rect& page, int index);
	void DrawSketches();
	void DrawSketch(int type, const ui::Rect& box, bool washed);
	void DrawRiddle(const ui::Rect& page, int index, bool answerHovered, bool answerHeld);
	void DrawCorners();
	void DrawFooter();
};

#endif
