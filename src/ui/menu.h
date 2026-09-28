#ifndef MENU_H
#define MENU_H
#include <memory>
#include <string>
#include "../graphics/font.h"
#include "../graphics/textures.h"

/// @file menu.h
/// Main menu, in-game menu and its save / load / options / credits screens.

class MainMenu {
  private:
	static constexpr int NONE = -1;
	// Click targets: menu buttons and save slots use their index.
	static constexpr int BACK = 100;	 // Back button of the sub-screens
	static constexpr int TAB_BASE = 200; // options tabs

	int hovered = NONE;
	int pressed = NONE; // mouse went down here; the action runs when it comes up on the same target

	bool assetsLoaded = false;
	Font title, heading, body, small;
	Texture creditsSheet;
	std::string toast;
	int toastStartMs = 0;
	int optionsTab = 0;

	void LoadAssets();
	void BeginCanvas();
	void EndFrame();
	void DrawBackground(const char* caption);
	void DrawButtons();
	void DrawSlots();
	void DrawSlot(int slot);
	void DrawOptions();
	void DrawCredits();
	void DrawBackButton();
	void DrawFooter(const char* hint);
	int TargetAt(int x, int y);
	void Activate(int target);
	void ShowToast(const std::string& text);

  public:
	MainMenu();
	~MainMenu();
	bool inGame;
	bool saveD;
	bool loadD;
	bool optionsD = false;
	bool creditsD = false;
	bool show;
	void Draw();
	void MouseFunction(int button, int state, int x, int y);
	void MousePassiveMotion(int x, int y);
	[[nodiscard]] bool InSubScreen() const { return saveD || loadD || optionsD || creditsD; }
	void ResetSubScreens();
};

#endif
