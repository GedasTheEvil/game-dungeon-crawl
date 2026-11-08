#ifndef MOUSE_LOOK_H
#define MOUSE_LOOK_H

#include <cstdlib>
#include <optional>
#include <utility>

// Mouse look (plan mouse-look-fixes): the turn a cursor motion makes, in pixels from the last position seen. Every
// screen passes its motions through, so the position stays fresh and the game does not jump when it comes back. After
// Forget (the cursor left or entered the window) the next motion only sets the position.
//
// The pointer held in the window (plan mouse-leaves-window): GLUT has no grab, so while the game looks, a pointer that
// wandered off the centre is warped back to it (Hold says where, the caller warps). The warp's own motion turns
// nothing; the few motions still queued from before it (not at the centre) are dropped, at most MOUSE_WARP_STALE of
// them.
constexpr int MOUSE_WARP_STALE = 4;

class MouseLook {
  public:
	struct Turn {
		int dx = 0;
		int dy = 0;
	};
	struct Point {
		int x;
		int y;
	};

	// looking: the game is shown, no screen over it; else the motion turns nothing.
	Turn Motion(int x, int y, bool looking) {
		if (warpedTo) {
			if (x == warpedTo->x && y == warpedTo->y) {
				warpedTo.reset();
				return {};
			}
			if (++stale <= MOUSE_WARP_STALE)
				return {}; // queued before the warp: measured from where the pointer no longer is
			warpedTo.reset(); // the warp's motion never came: start afresh here
			last = Point{x, y};
			return {};
		}
		const std::optional<Point> from = std::exchange(last, Point{x, y});
		if (!looking || !from)
			return {};
		return {x - from->x, y - from->y};
	}

	// After a looking motion in a window of width x height: the pointer is more than a quarter of the window off its
	// centre, so the point to warp it to (the centre), else none. From here on the turns measure from the centre.
	std::optional<Point> Hold(int width, int height) {
		if (!last || warpedTo)
			return std::nullopt;
		const Point centre{width / 2, height / 2};
		if (std::abs(last->x - centre.x) <= width / 4 && std::abs(last->y - centre.y) <= height / 4)
			return std::nullopt;
		last = centre;
		warpedTo = centre;
		stale = 0;
		return centre;
	}

	void Forget() {
		last.reset();
		warpedTo.reset();
	}

  private:
	std::optional<Point> last;
	std::optional<Point> warpedTo; // a warp is on its way: its motion turns nothing
	int stale = 0; // motions dropped since the warp
};

#endif
