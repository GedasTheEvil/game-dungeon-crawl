#ifndef RECORDS_VIEW_H
#define RECORDS_VIEW_H

#include "../graphics/font.h"
#include "../world/leaderboard.h"
#include "ui_layout.h"
#include <string>
#include <vector>

/// @file records_view.h
/// The Hall of Records (plan leaderboard): the main menu's sub-screen of the leaderboard. A list of rank, name and
/// score in the chosen order, a sort button per order; a row opens the run's detail screen. MainMenu draws the
/// backdrop, the title, the Back button and the footer, and hands the mouse and the keys on.

class RecordsView {
  public:
	static constexpr int NONE = -1;
	static constexpr int VISIBLE_ROWS = 10;

	// Opens the list in the time order; `marked` (an index into the board's records, NONE for none) is the run just
	// recorded: marked and scrolled into view.
	void Open(int marked);
	[[nodiscard]] bool InDetail() const { return detail != NONE; }
	void CloseDetail() { detail = NONE; } // back to the list, the sort kept
	[[nodiscard]] std::string Caption() const; // "Hall of Records", or the run's name
	[[nodiscard]] std::string Hint() const;
	[[nodiscard]] ui::Rect Panel() const;

	void Draw(Font& heading, Font& body, Font& small, int hovered, int pressed);
	[[nodiscard]] int TargetAt(float cx, float cy) const; // a sort button or a row, NONE
	void Activate(int target);
	void SpecialKey(int key); // Up / Down move the selection
	void Key(unsigned char key); // Enter opens the selected row
	void Scroll(int rows);

  private:
	static constexpr int SORT_BASE = 0;
	static constexpr int ROW_BASE = 10;

	RecordOrder order = RecordOrder::Time;
	int first = 0; // the list's first visible rank
	int selected = NONE; // a rank, from the keys
	int marked = NONE; // record index of the run just recorded
	int detail = NONE; // record index (into Shown) of the open detail screen

	[[nodiscard]] std::vector<RunRecord> shown() const;
	void select(int rank, int count);
	void drawList(Font& heading, Font& body, Font& small, int hovered, int pressed);
	void drawDetail(Font& heading, Font& small) const;
};

#endif
