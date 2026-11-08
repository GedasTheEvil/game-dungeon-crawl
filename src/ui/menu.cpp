#include "menu.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <GL/gl.h>
#include "../graphics/gl_includes.h"
#include "../state/game_state.h"
#include "../level/input.h"
#include "../core/paths.h"
#include "menu_internal.h"

namespace menu_internal {
// ---- layout ----------------------------------------------------------------

Rect menuPanel(int count) { return count > 6 ? TALL_MENU_PANEL : MENU_PANEL; }

// The buttons sit in the middle of the panel; more of them are closer together.
Rect buttonRect(int index, int count) {
	const Rect panel = menuPanel(count);
	float step = std::min(BUTTON_STEP, (panel.h - 2 * BUTTON_MARGIN - BUTTON_H) / static_cast<float>(count - 1));
	float first = panel.cy() + step * static_cast<float>(count - 1) / 2 - BUTTON_H / 2;
	return {CENTRE - BUTTON_W / 2, first - static_cast<float>(index) * step, BUTTON_W, BUTTON_H};
}

// Slots 1-3 in the left column, 4-6 in the right one, top to bottom. Load Game: AUTOSAVE_TARGET and QUICKSAVE_TARGET
// in the top row (left, right), the slots a row lower and lower.
Rect slotRect(int slot, bool load) {
	const Rect panel = load ? LOAD_PANEL : SLOT_PANEL;
	const float h = load ? LOAD_SLOT_H : SLOT_H;
	const float gap = load ? LOAD_GAP_Y : SLOT_GAP_Y;
	float x0 = panel.cx() - SLOT_W - SLOT_GAP_X / 2;
	float top = panel.y + panel.h - 4.f;
	if (slot == AUTOSAVE_TARGET)
		return {x0, top - h, SLOT_W, h};
	if (slot == QUICKSAVE_TARGET)
		return {x0 + SLOT_W + SLOT_GAP_X, top - h, SLOT_W, h};
	int column = slot / 3;
	int row = slot % 3 + (load ? 1 : 0);
	return {
		x0 + static_cast<float>(column) * (SLOT_W + SLOT_GAP_X),
		top - h - static_cast<float>(row) * (h + gap),
		SLOT_W,
		h
	};
}

Rect tabRect(int tab) {
	float y = OPTIONS_PANEL.y + OPTIONS_PANEL.h - 4.f - TAB_H;
	return {OPTIONS_PANEL.x + 5 + static_cast<float>(tab) * (TAB_W + 2), y, TAB_W, TAB_H};
}

Rect visibleArea() { return ui::visibleArea(CANVAS_W, CANVAS_H, Game().render.resX, Game().render.resY); }

enum class MenuAction : std::uint8_t {
	Continue,
	NewGame,
	Resume,
	Journal,
	Save,
	Load,
	Records,
	Options,
	Credits,
	MainMenu,
	Quit
};

struct MenuButton {
	const char* label;
	Icon icon;
	MenuAction action;
	bool primary;
};

constexpr std::array<MenuButton, 6> MAIN_BUTTONS = {{
	{"New Game", Icon::Play, MenuAction::NewGame, true},
	{"Load Game", Icon::Load, MenuAction::Load, false},
	{"Hall of Records", Icon::Podium, MenuAction::Records, false},
	{"Options", Icon::Gear, MenuAction::Options, false},
	{"Credits", Icon::Ankh, MenuAction::Credits, false},
	{"Exit", Icon::Exit, MenuAction::Quit, false},
}};
// With a save (plan autosave, retry-latest-save): Continue (the newest save) first, New Game under it.
constexpr std::array<MenuButton, 7> CONTINUE_BUTTONS = {{
	{"Continue", Icon::Play, MenuAction::Continue, true},
	{"New Game", Icon::Pyramid, MenuAction::NewGame, false},
	{"Load Game", Icon::Load, MenuAction::Load, false},
	{"Hall of Records", Icon::Podium, MenuAction::Records, false},
	{"Options", Icon::Gear, MenuAction::Options, false},
	{"Credits", Icon::Ankh, MenuAction::Credits, false},
	{"Exit", Icon::Exit, MenuAction::Quit, false},
}};
constexpr std::array<MenuButton, 6> IN_GAME_BUTTONS = {{
	{"Return to Game", Icon::Play, MenuAction::Resume, true},
	{"Journal", Icon::Book, MenuAction::Journal, false},
	{"Save Game", Icon::Save, MenuAction::Save, false},
	{"Load Game", Icon::Load, MenuAction::Load, false},
	{"Options", Icon::Gear, MenuAction::Options, false},
	{"Main Menu", Icon::Pyramid, MenuAction::MainMenu, false},
}};

struct ButtonList {
	const MenuButton* buttons;
	int count;
	[[nodiscard]] const MenuButton& operator[](int i) const { return buttons[i]; }
};

ButtonList menuButtons(bool inGame) {
	if (inGame)
		return {IN_GAME_BUTTONS.data(), static_cast<int>(IN_GAME_BUTTONS.size())};
	if (Game().LatestSave())
		return {CONTINUE_BUTTONS.data(), static_cast<int>(CONTINUE_BUTTONS.size())};
	return {MAIN_BUTTONS.data(), static_cast<int>(MAIN_BUTTONS.size())};
}

constexpr const char* LOST = " The play since the last save is lost.";
} // namespace menu_internal
using namespace menu_internal;

MainMenu::MainMenu() {
	inGame = false;
	saveD = false;
	loadD = false;
}

MainMenu::~MainMenu() {}

// The menu exists before the GL context, so the fonts and the credits sheet load on the first frame.
void MainMenu::LoadAssets() {
	if (assetsLoaded)
		return;
	assetsLoaded = true;
	creditsSheet.LoadPNG(MEDIA_DIR "textures/ui/credits.png", TexFilter::Flat);
	loadScreenFonts(title, heading, body, small, 8.f);
}

void MainMenu::ShowToast(const std::string& text) { toast.Show(text, GameClock::now()); }

// ---- drawing ---------------------------------------------------------------

void MainMenu::Draw() {
	LoadAssets();
	BeginCanvas();
	if (recordsD) {
		DrawBackground(records.Caption().c_str());
		records.Draw(heading, body, small, pointer.hovered - RECORDS_BASE, pointer.pressed - RECORDS_BASE);
		DrawBackButton();
		DrawFooter(records.Hint().c_str());
	} else if (creditsD) {
		DrawBackground("Credits");
		DrawCredits();
		DrawBackButton();
		DrawFooter("Esc: back");
	} else if (optionsD) {
		DrawBackground("Options");
		DrawOptions();
		DrawBackButton();
		DrawFooterHint();
	} else if (saveD || loadD) {
		DrawBackground(saveD ? "Save Game" : "Load Game");
		DrawSlots();
		DrawBackButton();
		DrawFooter(
			saveD ? "Click or Enter: save to a slot    Delete: delete    Esc: back"
				  : "Click or Enter: load a slot    Delete: delete    Esc: back"
		);
	} else {
		DrawBackground("Dungeon Crawl");
		DrawButtons();
		DrawFooter(inGame ? "Arrows: move    Enter: pick    Esc: return to game" : "Arrows: move    Enter: pick");
	}
	if (confirm.Open())
		confirm.Draw(heading, body, small, visibleArea());
	EndFrame();
}

void MainMenu::BeginCanvas() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	Rect area = visibleArea();
	glOrtho(area.x, area.x + area.w, area.y, area.y + area.h, -200, 200);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
}

void MainMenu::EndFrame() {
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glColor3f(1, 1, 1);
}

void MainMenu::DrawBackground(const char* caption) {
	bool wide = saveD || loadD || optionsD || recordsD;
	backdrop(visibleArea(), Game().assets.textures.loadingBackground.ID());
	titleBar(title, CENTRE, caption, wide ? 58.f : 44.f);
	panel(
		recordsD   ? records.Panel()
		: creditsD ? CREDITS_PANEL
		: optionsD ? OPTIONS_PANEL
		: loadD	   ? LOAD_PANEL
		: saveD	   ? SLOT_PANEL
				   : menuPanel(menuButtons(inGame).count),
		0.9f
	);
}

void MainMenu::DrawButtons() {
	ButtonList buttons = menuButtons(inGame);
	for (int i = 0; i < buttons.count; i++) {
		const MenuButton& b = buttons[i];
		bool isHovered = pointer.hovered == i;
		bool held = pointer.Held(i);
		Rect r = tile(buttonRect(i, buttons.count), b.primary ? TileStyle::Lapis : TileStyle::Stone, isHovered, held);

		Rect well = iconWell(r, 1.3f);
		fillRect(well, b.primary ? LAPIS_DARK : WELL, b.primary ? LAPIS_HELD_BOTTOM : WELL, 0.9f);
		strokeRect(well, isHovered ? GOLD : GOLD_DIM, 0.8f, 1.f);
		drawIcon(b.icon, well.cx(), well.cy(), well.h * 0.3f, isHovered ? GOLD_BRIGHT : GOLD);

		beginText();
		Color c = isHovered ? TEXT_HOVER : (b.primary ? GOLD : LABEL);
		textMiddle(heading, well.x + well.w + 4.f, r.cy(), b.label, c);
		beginShapes();
	}
}

void MainMenu::DrawCredits() {
	texturedRect(CREDITS_SHEET, creditsSheet.ID(), {1, 1, 1});
	beginShapes();
	strokeRect(CREDITS_SHEET, GOLD_DIM, 1.f, 1.f);
}

void MainMenu::DrawBackButton() {
	bool isHovered = pointer.hovered == BACK;
	Rect r = tile(BACK_BUTTON, TileStyle::Stone, isHovered, pointer.Held(BACK));
	Color c = isHovered ? GOLD_BRIGHT : LABEL;
	float labelW = heading.TextWidth("Back");
	float x0 = r.cx() - (labelW + 6.f) / 2;
	drawIcon(Icon::Back, x0 + 2.f, r.cy(), 2.f, c);
	beginText();
	textMiddle(heading, x0 + 6.f, r.cy(), "Back", c);
	beginShapes();
}

void MainMenu::DrawFooter(const char* hint) {
	beginText();
	if (float alpha = toast.Alpha(GameClock::now()); alpha > 0.f) // under the Back button where there is one
		textCentered(
			body,
			CENTRE,
			saveD || loadD || optionsD || creditsD || recordsD ? 5.6f : 12.f,
			toast.text.c_str(),
			{1.f, 0.9f, 0.6f},
			alpha
		);
	textCentered(small, CENTRE, 2.2f, hint, LABEL_DIM);
}

// ---- input -----------------------------------------------------------------

std::vector<MainMenu::Placed> MainMenu::Targets() const {
	std::vector<Placed> targets;
	if (recordsD || creditsD) {
		targets.push_back({BACK, BACK_BUTTON});
		return targets;
	}
	if (optionsD) {
		for (int tab = 0; tab < static_cast<int>(TABS.size()); tab++)
			targets.push_back({TAB_BASE + tab, tabRect(tab)});
		if (optionsTab == CONTROLS_TAB) {
			for (int action = 0; action < BIND_ACTION_COUNT; action++)
				for (int slot = 0; slot < BINDING_SLOTS; slot++)
					targets.push_back({BIND_BASE + action * BINDING_SLOTS + slot, controlCell(action, slot)});
			targets.push_back({RESET_CONTROLS, resetButton()});
		}
		OptionRows rows = optionRows(optionsTab);
		for (int i = 0; i < rows.count; i++) {
			Rect r = optionControl(i, rows[i].kind);
			if (rows[i].kind == OptionKind::Slider) // the knob reaches past the ends
				r = {r.x - 1.5f, r.y, r.w + 3.f, r.h};
			targets.push_back({OPTION_BASE + i, r});
		}
		targets.push_back({BACK, BACK_BUTTON});
		return targets;
	}
	if (saveD || loadD) {
		for (int slot = 0; slot < SaveSlots::COUNT; slot++) {
			const bool used = SaveSlots::DescribeFile(Game().SlotPath(slot)).used;
			if (used) // before its slot, which it sits on
				targets.push_back({DELETE_BASE + slot, deleteRect(slot, loadD)});
			if (saveD || used)
				targets.push_back({slot, slotRect(slot, loadD)});
		}
		if (loadD && Game().HasAutosave())
			targets.push_back({AUTOSAVE_TARGET, slotRect(AUTOSAVE_TARGET, true)});
		if (loadD && Game().HasQuicksave())
			targets.push_back({QUICKSAVE_TARGET, slotRect(QUICKSAVE_TARGET, true)});
		targets.push_back({BACK, BACK_BUTTON});
		return targets;
	}
	int count = menuButtons(inGame).count;
	for (int i = 0; i < count; i++)
		targets.push_back({i, buttonRect(i, count)});
	return targets;
}

int MainMenu::TargetAt(int x, int y) {
	float cx = 0.f;
	float cy = 0.f;
	ui::toCanvas(visibleArea(), Game().render.resX, Game().render.resY, x, y, cx, cy);
	if (recordsD)
		if (const int target = records.TargetAt(cx, cy); target != RecordsView::NONE)
			return RECORDS_BASE + target;
	for (const Placed& p : Targets())
		if (p.rect.contains(cx, cy))
			return p.target;
	return NONE;
}

int MainMenu::SubScreen() const {
	return (inGame ? 1 : 0) | (saveD ? 2 : 0) | (loadD ? 4 : 0) | (optionsD ? 8 : 0) | (creditsD ? 16 : 0) |
		   (recordsD ? 32 : 0);
}

// A key moves the focus (pointer.hovered) to the next target that way, or picks it like a click. With no focus yet,
// any direction focuses the first target.
void MainMenu::Navigate(NavKey key) {
	const std::vector<Placed> targets = Targets();
	if (key == NavKey::None || targets.empty())
		return;
	const auto at = std::ranges::find(targets, pointer.hovered, &Placed::target);
	if (at == targets.end()) {
		if (key != NavKey::Activate)
			pointer.Move(targets.front().target);
		return;
	}
	const int target = pointer.hovered;
	if (key == NavKey::Activate) {
		const int screen = SubScreen();
		pointer.Move(NONE);
		Activate(target);
		// The same screen keeps the focus; another one starts on its first target.
		const std::vector<Placed> now = Targets();
		if (SubScreen() == screen)
			pointer.Move(target);
		else if (!now.empty())
			pointer.Move(now.front().target);
		return;
	}
	const bool sideways = key == NavKey::Left || key == NavKey::Right;
	const int step = key == NavKey::Right ? 1 : -1;
	const bool onTab = optionsD && target >= TAB_BASE && target < OPTION_BASE;
	if (onTab && sideways) {
		optionsTab = std::clamp(optionsTab + step, 0, static_cast<int>(TABS.size()) - 1);
		pointer.Move(TAB_BASE + optionsTab);
		return;
	}
	if (optionsD && sideways && target >= OPTION_BASE && target < RESET_CONTROLS) {
		StepOption(target - OPTION_BASE, step);
		return;
	}
	if (onTab && key == NavKey::Down) { // the tab's first row or key cell, under the tabs
		pointer.Move(targets[TABS.size()].target);
		return;
	}
	std::vector<Rect> rects;
	rects.reserve(targets.size());
	for (const Placed& p : targets)
		rects.push_back(p.rect);
	const int dx = key == NavKey::Left ? -1 : key == NavKey::Right ? 1 : 0;
	const int dy = key == NavKey::Up ? 1 : key == NavKey::Down ? -1 : 0;
	const int next = nearestRect(rects, static_cast<int>(at - targets.begin()), dx, dy);
	if (next < 0)
		return;
	const int to = targets[static_cast<size_t>(next)].target;
	const bool toTab = optionsD && to >= TAB_BASE && to < OPTION_BASE;
	pointer.Move(toTab && !onTab ? TAB_BASE + optionsTab : to); // up onto the tabs: the open one
}

void MainMenu::Activate(int target) {
	if (target == BACK) {
		BackOut();
		return;
	}
	if (recordsD) {
		records.Activate(target - RECORDS_BASE);
		return;
	}
	if (optionsD) {
		if (target >= BIND_BASE)
			StartCapture(target);
		else if (target == RESET_CONTROLS) {
			Game().settings.controls = Bindings{};
			Game().ApplySettings(true);
			ShowToast("Controls reset to the defaults");
		} else if (target >= OPTION_BASE)
			ActivateOption(target - OPTION_BASE);
		else
			optionsTab = target - TAB_BASE;
		return;
	}
	if (target >= DELETE_BASE) {
		const int slot = target - DELETE_BASE;
		confirm.Ask("Delete the save in slot " + std::to_string(slot + 1) + "?", [this, slot] { DeleteSlot(slot); });
		return;
	}
	if (saveD) {
		AskOrDo(
			SaveSlots::DescribeFile(Game().SlotPath(target)).used,
			"Overwrite the save in slot " + std::to_string(target + 1) + "?",
			[this, target] { SaveToSlot(target); }
		);
		return;
	}
	if (loadD) {
		AskOrDo(inGame && Game().UnsavedProgress(), "Load " + saveName(target) + "?" + LOST, [this, target] {
			LoadFrom(target);
		});
		return;
	}

	switch (menuButtons(inGame)[target].action) {
	case MenuAction::Continue:
		if (!Game().LoadLatestSave())
			ShowToast("Could not load the newest save");
		break;
	case MenuAction::NewGame:
		AskOrDo(Game().HasAutosave(), "Start a new game? The autosave of the last run is replaced.", [this] {
			Game().ui.screen = Screen::Gameplay;
			Game().NewGame();
			inGame = true;
		});
		break;
	case MenuAction::Resume:
		Game().ui.screen = Screen::Gameplay;
		break;
	case MenuAction::Journal:
		Game().ui.screen = Screen::Journal;
		openedJournal = true;
		break;
	case MenuAction::Save:
		saveD = true;
		break;
	case MenuAction::Load:
		loadD = true;
		break;
	case MenuAction::Records:
		OpenRecords(RecordsView::NONE);
		break;
	case MenuAction::Options:
		optionsD = true;
		break;
	case MenuAction::Credits:
		creditsD = true;
		break;
	case MenuAction::MainMenu:
		AskOrDo(Game().UnsavedProgress(), std::string("Leave for the main menu?") + LOST, [this] { inGame = false; });
		break;
	case MenuAction::Quit: // glutMainLoop returns, main cleans up
		glutLeaveMainLoop();
	}
}

void MainMenu::AskOrDo(bool ask, const std::string& question, const std::function<void()>& action) {
	if (ask)
		confirm.Ask(question, action);
	else
		action();
}

void MainMenu::ConfirmInput(const std::function<void()>& forward) {
	forward();
	if (!confirm.Open())
		pointer.Clear();
}

void MainMenu::MouseFunction(int button, int state, int x, int y) {
	if (confirm.Open()) {
		float cx = 0.f;
		float cy = 0.f;
		ui::toCanvas(visibleArea(), Game().render.resX, Game().render.resY, x, y, cx, cy);
		ConfirmInput([&] { confirm.Button(button, state == GLUT_DOWN, cx, cy); });
		return;
	}
	int target = TargetAt(x, y);
	pointer.Move(target);

	// Waiting for a binding: a mouse button binds the mouse cell; on a key cell a click cancels.
	if (capturing != NONE) {
		bool mouseCell = (capturing - BIND_BASE) % BINDING_SLOTS == MOUSE_SLOT;
		bool isButton = button == MOUSE_LEFT_BUTTON || button == MOUSE_MIDDLE_BUTTON || button == MOUSE_RIGHT_BUTTON;
		if (state == GLUT_DOWN && isButton) {
			if (mouseCell)
				Bind(InputKey::Mouse(button));
			else
				capturing = NONE;
		}
		pointer.Down(NONE, button);
		return;
	}

	// A slider follows the mouse while the button is down; the file is written when it comes up.
	if (dragging != NONE) {
		if (state == GLUT_UP) {
			const OptionRow option = optionRows(optionsTab)[dragging - OPTION_BASE];
			dragging = NONE;
			Game().ApplySettings(true);
			if (option.id == OptionId::Effects || option.id == OptionId::Master)
				Game().assets.sounds.lever.Play(); // a sample at the new volume
		}
		return;
	}
	if (recordsD && state == GLUT_DOWN && (button == MOUSE_WHEEL_UP || button == MOUSE_WHEEL_DOWN)) {
		records.Scroll(button == MOUSE_WHEEL_UP ? -1 : 1);
		pointer.Move(TargetAt(x, y));
		return;
	}
	if (button != MOUSE_LEFT_BUTTON)
		return;
	if (state == GLUT_DOWN && optionsD && target >= OPTION_BASE && target < RESET_CONTROLS &&
		optionRows(optionsTab)[target - OPTION_BASE].kind == OptionKind::Slider) {
		dragging = target;
		SetSlider(target - OPTION_BASE, x);
		return;
	}

	if (state != GLUT_UP) {
		pointer.Down(target, button);
		return;
	}
	if (const int fired = pointer.Up(target, button); fired != NONE) {
		pointer.Move(NONE); // the next screen has its own targets
		Activate(fired);
	}
}

void MainMenu::MousePassiveMotion(int x, int y) {
	if (confirm.Open()) {
		float cx = 0.f;
		float cy = 0.f;
		ui::toCanvas(visibleArea(), Game().render.resX, Game().render.resY, x, y, cx, cy);
		confirm.Move(cx, cy);
		return;
	}
	pointer.Move(TargetAt(x, y));
}

void MainMenu::MouseDrag(int x, int y) {
	if (confirm.Open())
		MousePassiveMotion(x, y);
	else if (dragging != NONE)
		SetSlider(dragging - OPTION_BASE, x);
	else
		pointer.Move(TargetAt(x, y));
}

void MainMenu::BackOut() {
	if (confirm.Open()) {
		confirm.Cancel();
		return;
	}
	if (recordsD && records.InDetail()) {
		records.CloseDetail();
		pointer.Move(NONE);
		return;
	}
	ResetSubScreens();
}

void MainMenu::OpenRecords(int marked) {
	ResetSubScreens();
	recordsD = true;
	records.Open(marked);
}

void MainMenu::KeyPressed(unsigned char key) {
	if (confirm.Open())
		ConfirmInput([&] { confirm.Navigate(navKeyOf(InputKey::Char(key), Game().settings.controls)); });
	else if ((saveD || loadD) && key == KEY_DELETE)
		AskDelete();
	else if (recordsD)
		records.Key(key);
	else
		Navigate(navKeyOf(InputKey::Char(key), Game().settings.controls));
}

void MainMenu::SpecialKeyPressed(int key) {
	if (confirm.Open())
		ConfirmInput([&] { confirm.Navigate(navKeyOf(InputKey::Special(key), Game().settings.controls)); });
	else if ((saveD || loadD) && key == SPECIAL_KEYPAD_DELETE)
		AskDelete();
	else if (recordsD)
		records.SpecialKey(key);
	else
		Navigate(navKeyOf(InputKey::Special(key), Game().settings.controls));
}

void MainMenu::ResetSubScreens() {
	confirm.Cancel();
	creditsD = false;
	recordsD = false;
	saveD = false;
	loadD = false;
	optionsD = false;
	pointer.Clear();
	capturing = NONE;
	dragging = NONE;
}
