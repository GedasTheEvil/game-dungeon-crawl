#ifndef SKILLS_VIEW_H
#define SKILLS_VIEW_H

#include "../graphics/font.h"
#include "../input/bindings.h"
#include "pointer.h"
#include "ui_draw.h"

// The Heka screen ([K], plan skills): a card per skill and the heka left to spend; the papyrus on the right tells the
// selected skill and learns it, after a second press to confirm (no unlearning). Its layout: skills_layout.h. The game
// is paused while it is open.
class SkillsScreen {
  public:
	void Draw();
	void MouseFunction(int button, int state, int x, int y);
	void MouseMotion(int x, int y);
	void KeyPressed(unsigned char key);
	void SpecialKeyPressed(int key);

  private:
	enum class Target : unsigned char { None, Card, LearnButton };
	struct Pick {
		Target target = Target::None;
		int card = -1;
		bool operator==(const Pick&) const = default;
	};

	int selected = 0; // a SkillId
	ui::Pointer<Pick> pointer;
	bool confirming = false; // the first press of Learn: the next one learns
	ui::Toast toast;
	Font title, heading, body, small;
	bool fontsLoaded = false;

	void Select(int card);
	void MoveSelection(int dx, int dy);
	void Navigate(NavKey key); // the arrows and the bound move keys pick a card, Enter / Space / Interact learn
	void PressLearn();
	void UpdateHover(float x, float y);

	void DrawPoints();
	void DrawCard(int card);
	void DrawDetails();
	void DrawFooter();
};

#endif
