#ifndef SKILLS_VIEW_H
#define SKILLS_VIEW_H

#include "../graphics/font.h"
#include "ui_draw.h"

// The Heka screen ([K], docs/plan/skills.md): a card per skill and the heka left to spend; the papyrus on the right
// tells the selected skill and learns it, after a second press to confirm (no unlearning). Its layout:
// skills_layout.h. The game is paused while it is open.
class SkillsScreen {
  public:
	void Draw();
	void MouseFunction(int button, int state, int x, int y);
	void MouseMotion(int x, int y);
	void KeyPressed(unsigned char key);
	void SpecialKeyPressed(int key);

  private:
	enum class Target : unsigned char { None, Card, LearnButton };

	int selected = 0; // a SkillId
	int hoveredCard = -1;
	bool hoveredButton = false;
	Target pressed = Target::None;
	int pressedCard = -1;
	bool confirming = false; // the first press of Learn: the next one learns
	ui::Toast toast;
	Font title, heading, body, small;
	bool fontsLoaded = false;

	void Select(int card);
	void MoveSelection(int dx, int dy);
	void PressLearn();
	void UpdateHover(float x, float y);

	void DrawPoints();
	void DrawCard(int card);
	void DrawDetails();
	void DrawFooter();
};

#endif
