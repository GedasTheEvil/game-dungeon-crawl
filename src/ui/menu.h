#ifndef MENU_H
#define MENU_H
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "../render/font.h"
#include "../render/textures.h"
#include "../level/bindings.h"
#include "confirm_box.h"
#include "../level/pointer.h"
#include "records_view.h"
#include "../render/ui_draw.h"

/// @file menu.h
/// Main menu, in-game menu and its save / load / options / credits / Hall of Records screens.

class MainMenu {
  private:
	static constexpr int NONE = -1;
	// Click targets: menu buttons and save slots use their index.
	static constexpr int BACK = 100; // Back button of the sub-screens
	static constexpr int TAB_BASE = 200; // options tabs
	static constexpr int OPTION_BASE = 300; // Options > Display / Sound: the row's switch, choice or slider
	static constexpr int RESET_CONTROLS = 390; // Options > Controls: back to the default keys
	static constexpr int BIND_BASE = 400; // Options > Controls: action * BINDING_SLOTS + slot
	static constexpr int RECORDS_BASE = 600; // Hall of Records: its own targets (RecordsView)
	static constexpr int DELETE_BASE = 700; // Save / Load Game: a used slot's delete button, + slot

	ui::Pointer<int> pointer{NONE};

	bool assetsLoaded = false;
	Font title, heading, body, small;
	Texture creditsSheet;
	ui::Toast toast; // feedback line after an action
	int optionsTab = 0;
	int capturing = NONE; // the BIND_BASE target waiting for a key or a mouse button
	int dragging = NONE; // the slider the mouse went down on
	RecordsView records;
	ConfirmBox confirm; // "Overwrite slot 2?" over the screen (plan save-load-safety)

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
	void ActivateOption(int row, int step = 1); // a switch flips, a choice steps on (1) or back (-1)
	void StepOption(int row, int step); // Left / Right on a row: also a slider, by 5
	void SetSlider(int row, int x);
	void StartCapture(int target);
	void Bind(const InputKey& key);
	void DrawCredits();
	void DrawBackButton();
	void DrawFooter(const char* hint);
	int TargetAt(int x, int y);
	// The open sub-screen's targets and their canvas rects, in TargetAt's order (not the Hall of Records'):
	// the keyboard moves the focus between them (plan keyboard-menus).
	struct Placed {
		int target;
		ui::Rect rect;
	};
	[[nodiscard]] std::vector<Placed> Targets() const;
	[[nodiscard]] int SubScreen() const; // which screen is open, to tell when an action opened another
	void Navigate(NavKey key);
	void Activate(int target);
	void ShowToast(const std::string& text);
	// What would throw away play asks first (plan save-load-safety); these run on the answer.
	void SaveToSlot(int slot);
	void LoadFrom(int target); // a slot, AUTOSAVE_TARGET or QUICKSAVE_TARGET
	void DeleteSlot(int slot);
	void AskDelete();
	void AskOrDo(bool ask, const std::string& question, const std::function<void()>& action);
	void ConfirmInput(const std::function<void()>& forward); // the box had the input: a closed box ends the focus

  public:
	MainMenu();
	~MainMenu();
	bool inGame;
	bool saveD;
	bool loadD;
	bool optionsD = false;
	bool creditsD = false;
	bool recordsD = false;
	void Draw();
	void MouseFunction(int button, int state, int x, int y);
	void MousePassiveMotion(int x, int y);
	void MouseDrag(int x, int y); // a button held down: drags a volume slider
	// Options > Controls waits for the key or mouse button of a binding.
	[[nodiscard]] bool Capturing() const { return capturing != NONE; }
	void CaptureKey(const InputKey& key); // Esc cancels, Delete clears the binding
	// A sub-screen or the confirm box is open: Esc backs out of it.
	[[nodiscard]] bool InSubScreen() const {
		return saveD || loadD || optionsD || creditsD || recordsD || confirm.Open();
	}
	void ResetSubScreens();
	// Esc or Back: the confirm box says no, a run's details back to the Hall of Records' list, any other sub-screen
	// closes.
	void BackOut();
	// The Hall of Records, with the run just recorded marked (an index into the board's records, or
	// RecordsView::NONE).
	void OpenRecords(int marked);
	// The arrows (and the bound move keys) move the focus, Enter activates, Left / Right on an Options row change it,
	// on the tabs switch them; the Hall of Records has its own keys. Esc is input.cpp's (BackOut).
	void KeyPressed(unsigned char key);
	void SpecialKeyPressed(int key);
	bool openedJournal = false; // the in-game menu opened the journal: Esc there comes back here
};

#endif
