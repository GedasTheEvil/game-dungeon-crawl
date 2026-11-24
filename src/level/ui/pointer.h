#ifndef POINTER_H
#define POINTER_H

// The click rules every UI screen shares (plan shared-click-handling): what the mouse is over, and the target and the
// mouse button it went down on. An action fires on the button's release over the target it went down on. Generic over
// the screen's own target type (an int, an enum, a struct with ==) and its "none" value. The screen keeps its own
// cases around it: a select on press (inventory slots, skill cards), a slider drag (menu), the wheel.
// `hovered` is the keyboard focus too: the mouse and the keys move the same slot.
namespace ui {
template <class Target> class Pointer {
  public:
	constexpr explicit Pointer(Target none = Target{}) : none(none), hovered(none), pressed(none) {}

	Target none;
	Target hovered; // under the mouse, or moved there by the keys
	Target pressed; // the mouse button went down here
	int button = 0; // ... this mouse button

	void Move(const Target& at) { hovered = at; }
	// The button went down on `at` (none: on nothing). Does not move `hovered`.
	void Down(const Target& at, int mouseButton) {
		pressed = at;
		button = mouseButton;
	}
	// A button came up over `at`: the target to fire if it went down there with this button, else none. The release
	// of another button keeps the press.
	Target Up(const Target& at, int mouseButton) {
		if (mouseButton != button)
			return none;
		const Target fired = pressed == at ? pressed : none;
		pressed = none;
		return fired;
	}
	// Pushed in: pressed, and the mouse is still over it.
	[[nodiscard]] bool Held(const Target& at) const { return at != none && hovered == at && pressed == at; }
	[[nodiscard]] bool Pressing() const { return pressed != none; }
	// The page or screen changed: what was under the mouse is gone.
	void Clear() {
		hovered = none;
		pressed = none;
	}
};
} // namespace ui

#endif
