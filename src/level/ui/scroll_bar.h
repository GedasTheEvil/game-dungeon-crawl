#ifndef SCROLL_BAR_H
#define SCROLL_BAR_H

// The scroll bar every scrolling UI list shares (plan scrollbar-mouse-drag): a track beside the rows shown and a thumb
// where they are, the list scrolled by whole rows. The mouse drags the thumb (the list follows) and a click on the
// track pages towards it. Without GL: the screens draw Track() and Thumb() and feed the mouse in canvas units.
#include "ui_layout.h"
#include <algorithm>
#include <cmath>

namespace ui {

struct ScrollBar {
	Rect track; // the whole bar, the rows shown beside it
	int rows = 0; // the list's rows
	int visible = 1; // the rows shown at once

	// Beside the thin track, the mouse may miss by this much and still hit the bar.
	static constexpr float GRAB_PAD = 0.6f;

	[[nodiscard]] bool Shown() const { return rows > visible; }
	[[nodiscard]] int MaxScroll() const { return std::max(0, rows - visible); }
	[[nodiscard]] float RowH() const { return track.h / static_cast<float>(std::max(rows, 1)); }
	[[nodiscard]] float Top() const { return track.y + track.h; }
	// The thumb for the first row shown `scroll`: as tall as the rows shown, lower the further down.
	[[nodiscard]] Rect Thumb(int scroll) const {
		const float top = Top() - RowH() * static_cast<float>(scroll);
		const float h = RowH() * static_cast<float>(std::min(visible, rows));
		return {track.x, top - h, track.w, h};
	}
	// The scroll whose thumb top is nearest `thumbTop`, within the list.
	[[nodiscard]] int ScrollAt(float thumbTop) const {
		return std::clamp(static_cast<int>(std::lround((Top() - thumbTop) / RowH())), 0, MaxScroll());
	}
	// On the bar (the track widened by GRAB_PAD), when it is shown.
	[[nodiscard]] bool Hit(float x, float y) const {
		return Shown() && Rect{track.x - GRAB_PAD, track.y, track.w + 2 * GRAB_PAD, track.h}.contains(x, y);
	}
};

// The thumb held by the mouse: where on it the button went down, so it does not jump to the pointer.
class ScrollDrag {
  public:
	// The button went down at (x, y). On the thumb: the drag starts. On the track: a page up or down towards it. True
	// if the bar took the click (the screen then ignores it).
	bool Down(const ScrollBar& bar, float x, float y, int& scroll) {
		dragging = false;
		if (!bar.Hit(x, y))
			return false;
		const Rect thumb = bar.Thumb(scroll);
		if (y >= thumb.y && y <= thumb.y + thumb.h) {
			dragging = true;
			grab = thumb.y + thumb.h - y;
		} else {
			scroll = std::clamp(scroll + (y > thumb.y + thumb.h ? -bar.visible : bar.visible), 0, bar.MaxScroll());
		}
		return true;
	}
	// The mouse moved to height y: while dragging, the list follows. True while dragging.
	bool Move(const ScrollBar& bar, float y, int& scroll) const {
		if (dragging)
			scroll = bar.ScrollAt(y + grab);
		return dragging;
	}
	void Up() { dragging = false; }
	[[nodiscard]] bool Dragging() const { return dragging; }

  private:
	bool dragging = false;
	float grab = 0.f; // from the mouse down to the thumb's top
};

} // namespace ui

#endif
