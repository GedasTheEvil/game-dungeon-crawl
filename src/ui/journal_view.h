#ifndef JOURNAL_VIEW_H
#define JOURNAL_VIEW_H

#include "../graphics/font.h"
#include "../graphics/render_target.h"
#include "../graphics/textures.h"
#include "../world/level.h"
#include "ui_draw.h"
#include <array>
#include <cstdint>
#include <string>

// The archaeologist's journal ([J]): a cloth-bound field notebook open on two grid pages, a coloured ribbon bookmark
// in the first page of each section, hanging out on the left or right. Pages turn over the spine (ui/page_curl.h): a
// click, a key or a drag of a page corner. The data is Game().journal (world/journal.h); this is only the screen. The
// game is paused while it is open. It opens on the section and pages looked at last.
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
	// Where the book lies open: a section and a spread of it.
	struct Opening {
		Section section = Section::Creatures;
		int spread = 0;
	};
	// A page on its way over the spine. Its bottom free corner goes round an ellipse from the page's edge (angle 0)
	// to the far side (pi); dragged, it follows the mouse instead.
	struct PageTurn {
		bool active = false;
		Opening from;		   // open when it began
		Opening to;			   // open here once the page is over
		bool forward = true;   // the right page turns to the left
		bool dragging = false; // held by the mouse
		float angle = 0;	   // round the ellipse
		float spread = 1;	   // of the corner from the ellipse's centre, 1 on it; a released drag eases back to it
		float target = 0;	   // the angle it goes to: pi over, 0 back
		float cornerX = 0, cornerY = 0; // page-local while dragging (PageCurl)
		float progress = 0;				// 0 flat .. 1 over, this frame (curlProgress)
		float pressX = 0, pressY = 0;	// canvas, where the drag began: a press let go nearby is a click
		int lastMs = 0;
	};
	// The four pages a turn shows: the spread under it, the turning page's front and back.
	enum class PageSlot : std::uint8_t { Left, Right, Front, Back };
	static constexpr size_t PAGE_SLOTS = 4;

	bool fontsLoaded = false;
	Font title, heading, body, small;
	Font handHeading, hand, handSmall, handTiny; // the archaeologist's handwriting, for what is written on the pages
	Font stamp, stampSmall;						 // typewriter: page and catalogue numbers
	Section section = Section::Creatures;
	std::array<int, SECTION_COUNT> spread{}; // the spread open in each section
	PageTurn turn;
	Hit hovered;
	Hit pressed;
	ui::Toast toast;
	std::array<Texture, MONSTER_TYPE_MAX + 1> photoTextures; // monster textures in black and white, by type, on demand
	std::array<RenderTarget, PAGE_SLOTS> pages;				 // drawn each frame, then laid on the book or bent
	bool pagesFailed = false; // no framebuffers: the pages are drawn straight onto the book, turns are instant

	[[nodiscard]] int PageCount(Section s) const;
	[[nodiscard]] int SpreadCount(Section s) const;
	[[nodiscard]] Opening Open() const { return {section, spread[static_cast<size_t>(section)]}; }
	// Pages before a section's first one, before the open spread's left page, in the whole book (all sections).
	[[nodiscard]] int FirstPage(int s) const;
	[[nodiscard]] int PagesBefore(const Opening& at) const {
		return FirstPage(static_cast<int>(at.section)) + at.spread * 2;
	}
	[[nodiscard]] int PagesTotal() const { return FirstPage(SECTION_COUNT); }
	// Where a section's ribbon hangs out of the book open at `at`: left of the spine once its first page is open or
	// turned, and how far out (canvas units), from the pages between it and the open spread. Under the open page on
	// its side, coming out of the page edge, but for the ribbon of the open left page: that one lies on top.
	struct RibbonPlace {
		bool left = false;
		float depth = 0;
		bool top = false;
	};
	[[nodiscard]] RibbonPlace PlaceOf(int ribbon, const Opening& at) const;
	// The ribbon's first page goes over with the turning page.
	[[nodiscard]] bool Riding(int ribbon) const;
	// How far out a riding ribbon hangs from the turning page: from where it lay, in, then out to where it lands.
	[[nodiscard]] float RidingDepth(int ribbon) const;
	void Turn(int by);
	void GoTo(Opening to);
	void StartTurn(Opening to, bool dragged);
	void FinishTurn();
	void AdvanceTurn();
	[[nodiscard]] float CornerLocalX(float canvasX) const;
	// Journal riddle index on the left (0) or right (1) page of the open spread, -1 if none.
	[[nodiscard]] int RiddleOnPage(int side) const;
	// Index in the open section's list on the left (0) or right (1) page, -1 if the page is empty.
	[[nodiscard]] int EntryOnPage(int side) const;
	[[nodiscard]] bool CanAnswer(int side) const;
	void Activate(const Hit& hit);
	[[nodiscard]] Hit HitAt(float x, float y) const;

	// A page into its slot's texture (or straight onto the book, see pagesFailed); `live` for the open spread's
	// pages, whose Answer buttons react to the mouse. Returns the texture, 0 if drawn straight.
	int RenderPage(PageSlot slot, Opening at, int side, bool live);
	void DrawPageContent(Opening at, int side, bool live);
	void DrawBook(const Opening& at);
	// The ribbons lying still (not on a turning page), under the open pages or on top of them (RibbonPlace::top);
	// with the top ones the hovered one's name.
	void DrawRibbons(bool top);
	void DrawRibbon(int ribbon, bool left, float depth, bool open, bool hovered);
	void DrawCreature(const ui::Rect& page, int index);
	void DrawFieldNote(const ui::Rect& page, int index);
	void DrawSketch(int type, const ui::Rect& box, bool photo, float tilt);
	void DrawRiddle(const ui::Rect& page, int index, bool answerHovered, bool answerHeld);
	void DrawCorners();
	void DrawFooter();
};

#endif
