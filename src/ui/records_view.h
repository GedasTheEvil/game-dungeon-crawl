#ifndef RECORDS_VIEW_H
#define RECORDS_VIEW_H

#include "../render/font.h"
#include "../level/files/leaderboard.h"
#include "../level/ui/scroll_bar.h"
#include "../level/ui/ui_layout.h"
#include <string>
#include <vector>
#include "screen_world.h"

/// @file records_view.h
/// The Hall of Records (plan leaderboard): the main menu's sub-screen of the leaderboard. A list of rank, name and
/// score in the chosen order, a sort button per order; a row opens the run's detail screen. MainMenu draws the
/// backdrop, the title, the Back button and the footer, and hands the mouse and the keys on.

class RecordsView {
  public:
	explicit RecordsView(const ScreenWorld& world) : world(world) {}
	static constexpr int NONE = -1;
	static constexpr int VISIBLE_ROWS = 10;

	// Opens the list in the time order; `marked` (an index into the board's records, NONE for none) is the run just
	// recorded: marked and scrolled into view.
	void Open(int marked);
	[[nodiscard]] bool InDetail() const { return detail != NONE; }
	void CloseDetail(); // back to the list, the sort kept
	[[nodiscard]] std::string Caption() const; // "Hall of Records", or the run's name
	[[nodiscard]] std::string Hint() const;
	[[nodiscard]] ui::Rect Panel() const;

	void Draw(Font& heading, Font& body, Font& small, int hovered, int pressed);
	[[nodiscard]] int TargetAt(float cx, float cy) const; // a sort button or a row, NONE
	void Activate(int target);
	void SpecialKey(int key); // Up / Down move the selection; in the details, they and PgUp / PgDn scroll
	void Key(unsigned char key); // Enter opens the selected row
	void Scroll(int rows); // the list, or the open run's details (plan endgame-stats-skills-weapon)
	// The run's details scroll bar (canvas units): shown when its rows do not all fit the panel.
	[[nodiscard]] ui::ScrollBar DetailBar() const;
	// The left button down or up at (cx, cy): the details' bar takes it (drags the thumb, pages on the track). True
	// if it did.
	bool BarButton(bool down, float cx, float cy);
	[[nodiscard]] bool BarDragging() const { return detailDrag.Dragging(); }
	void BarDrag(float cy); // the mouse moved to height cy, the thumb held

  private:
	const ScreenWorld& world; // the game it shows and changes (plan game-state-split)
	static constexpr int SORT_BASE = 0;
	static constexpr int ROW_BASE = 10;

	RecordOrder order = RecordOrder::Time;
	int first = 0; // the list's first visible rank
	int selected = NONE; // a rank, from the keys
	int marked = NONE; // record index of the run just recorded
	int detail = NONE; // record index (into Shown) of the open detail screen
	int detailScroll = 0; // its rows scrolled away, DETAIL_LINE each
	ui::ScrollDrag detailDrag; // its bar's thumb held

	[[nodiscard]] std::vector<RunRecord> shown() const;
	void select(int rank, int count);
	void openDetail(int index);
	void drawList(Font& heading, Font& body, Font& small, int hovered, int pressed);
	void drawDetail(Font& heading, Font& small);
};

#endif
