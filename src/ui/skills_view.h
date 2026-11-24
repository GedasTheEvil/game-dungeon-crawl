#ifndef SKILLS_VIEW_H
#define SKILLS_VIEW_H

#include "ui_screen.h"
#include "../render/font.h"
#include "../level/input/bindings.h"
#include "../level/ui/pointer.h"
#include "../level/ui/scroll_bar.h"
#include "../render/ui_draw.h"
#include <string>
#include <vector>
#include "screen_world.h"

// The Heka screen ([K], plan skills): a card per skill (a list that scrolls, plan heka-surplus) and the heka left to
// spend; the papyrus on the right tells the selected skill and learns it, after a second press to confirm (no
// unlearning). Its layout: skills_layout.h. The game is paused while it is open.
struct UIContext;

class SkillsScreen final : public UiScreen {
  public:
	SkillsScreen(const ScreenWorld& world, UIContext& ui) : world(world), ui(ui) {}
	void Draw() override;
	void MouseFunction(int button, int state, int x, int y) override;
	void MouseMotion(int x, int y) override;
	void KeyPressed(unsigned char key) override;
	void SpecialKeyPressed(int key) override;
	// The most lines any skill's effect (rank I or II) takes on its card (at most 2 fit; scenario field
	// skill_card_lines).
	[[nodiscard]] int MostEffectLines();

  private:
	const ScreenWorld& world; // the game it shows and changes (plan game-state-split)
	UIContext& ui;
	enum class Target : unsigned char { None, Card, LearnButton };
	struct Pick {
		Target target = Target::None;
		int card = -1;
		bool operator==(const Pick&) const = default;
	};

	int selected = 0; // a SkillId
	int scroll = 0; // the first card row shown (plan heka-surplus): follows the selection, the wheel moves it
	ui::Pointer<Pick> pointer;
	ui::ScrollDrag scrollDrag; // the bar's thumb held (plan scrollbar-mouse-drag)
	int detailScroll = 0; // the papyrus text's first line shown: the wheel over it, its bar, PgUp / PgDn
	ui::ScrollDrag detailDrag;
	bool confirming = false; // the first press of Learn: the next one learns
	ui::Toast toast;
	Font title, heading, body, small;
	int fontGeneration = -1; // ensureScreenFonts

	void LoadFonts();
	std::vector<std::string> EffectLines(int card, bool rankTwo);
	struct DetailLine {
		std::string text;
		bool lore = false; // `small` INK_FADED, else the effect in `body` INK
		float drop = 0.f; // below the first line's baseline
	};
	std::vector<DetailLine> DetailLines(); // the selected skill's effect and lore on the papyrus, wrapped
	ui::ScrollBar DetailBar(); // beside them, when they do not fit over the status line
	void ScrollDetail(int lines);
	void Select(int card);
	void MoveSelection(int dx, int dy);
	void Navigate(NavKey key); // the arrows and the bound move keys pick a card, Enter / Space / Interact learn
	void PressLearn();
	void UpdateHover(float x, float y);

	void DrawPoints();
	void DrawCard(int card);
	void DrawScrollBar();
	void DrawDetails();
	void DrawFooter();
};

#endif
