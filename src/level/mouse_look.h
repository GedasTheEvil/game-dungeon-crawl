#ifndef MOUSE_LOOK_H
#define MOUSE_LOOK_H

#include <optional>
#include <utility>

// Mouse look (plan mouse-look-fixes): the turn a cursor motion makes, in pixels from the last position seen. Every
// screen passes its motions through, so the position stays fresh and the game does not jump when it comes back. After
// Forget (the cursor left or entered the window) the next motion only sets the position.
class MouseLook {
  public:
	struct Turn {
		int dx = 0;
		int dy = 0;
	};

	// looking: the game is shown, no screen over it; else the motion turns nothing.
	Turn Motion(int x, int y, bool looking) {
		const std::optional<Point> from = std::exchange(last, Point{x, y});
		if (!looking || !from)
			return {};
		return {x - from->x, y - from->y};
	}

	void Forget() { last.reset(); }

  private:
	struct Point {
		int x;
		int y;
	};
	std::optional<Point> last;
};

#endif
