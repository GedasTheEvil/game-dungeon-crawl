#ifndef MENU_H
#define MENU_H
#include <memory>
#include <string>
#include "../graphics/font.h"
#include "../graphics/textures.h"
#include "../input/bindings.h"
#include "ui_draw.h"

/// @file menu.h
/// Main menu, in-game menu and its save / load / options / credits screens.

class MainMenu {
  private:
	static constexpr int NONE = -1;
	// Click targets: menu buttons and save slots use their index.
	static constexpr int BACK = 100;		   // Back button of the sub-screens
	static constexpr int TAB_BASE = 200;	   // options tabs
	static constexpr int OPTION_BASE = 300;	   // Options > Display / Sound: the row's switch, choice or slider
	static constexpr int RESET_CONTROLS = 390; // Options > Controls: back to the default keys
	static constexpr int BIND_BASE = 400;	   // Options > Controls: action * BINDING_SLOTS + slot

	int hovered = NONE;
	int pressed = NONE; // mouse went down here; the action runs when it comes up on the same target

	bool assetsLoaded = false;
	Font title, heading, body, small;
	Texture creditsSheet;
	ui::Toast toast; // feedback line after an action
	int optionsTab = 0;
	int capturing = NONE; // the BIND_BASE target waiting for a key or a mouse button
	int dragging = NONE;  // the slider the mouse went down on

	void LoadAssets();
	void BeginCanvas();
	void EndFrame();
	void DrawBackground(const char* caption);
	void DrawButtons();
	void DrawSlots();
	void DrawSlot(int slot);
	void DrawOptions();
	void DrawControls(float left, float right);
	void DrawControlCell(int target, const ui::Rect& r);
	void DrawRows();
	void DrawFooterHint();
	void ActivateOption(int row);
	void SetSlider(int row, int x);
	void StartCapture(int target);
	void Bind(const InputKey& key);
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
	void Draw();
	void MouseFunction(int button, int state, int x, int y);
	void MousePassiveMotion(int x, int y);
	void MouseDrag(int x, int y); // a button held down: drags a volume slider
	// Options > Controls waits for the key or mouse button of a binding.
	[[nodiscard]] bool Capturing() const { return capturing != NONE; }
	void CaptureKey(const InputKey& key); // Esc cancels, Delete clears the binding
	[[nodiscard]] bool InSubScreen() const { return saveD || loadD || optionsD || creditsD; }
	void ResetSubScreens();
};

#endif
