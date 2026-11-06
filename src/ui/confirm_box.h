#ifndef CONFIRM_BOX_H
#define CONFIRM_BOX_H

#include "../render/font.h"
#include "../level/bindings.h"
#include "../level/pointer.h"
#include "../level/ui_layout.h"
#include <functional>
#include <string>

/// @file confirm_box.h
/// A question with Yes and No over the screen that asks it (plan save-load-safety), on the 160 x 100 canvas: the
/// screen under it dims and takes no input while it is open. Enter answers the focused button (Yes at first), Esc is
/// No; the arrows move between the two. The screen forwards its mouse (in canvas units) and keys while Open().

class ConfirmBox {
  public:
	void Ask(const std::string& question, std::function<void()> yes);
	[[nodiscard]] bool Open() const { return static_cast<bool>(onYes); }
	void Cancel(); // Esc, or No

	void Draw(Font& heading, Font& body, Font& small, const ui::Rect& area);
	void Move(float cx, float cy);
	void Button(int button, bool down, float cx, float cy); // the left button: Yes or No on its release
	void Navigate(NavKey key);

  private:
	static constexpr int NONE = -1;
	static constexpr int YES = 0;
	static constexpr int NO = 1;

	std::string question;
	std::function<void()> onYes;
	ui::Pointer<int> pointer{NONE};

	[[nodiscard]] static int targetAt(float cx, float cy);
	void answer(int target);
};

#endif
