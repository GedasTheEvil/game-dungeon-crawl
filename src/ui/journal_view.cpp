#include "journal_view.h"
#include "journal_internal.h"
#include "../graphics/gl_includes.h"
#include "../render/inscription_draw.h"
#include "../level/input.h"
#include "../input/input_actions.h"
#include "../state/game_state.h"
#include "../level/decor.h"
#include "../level/inscriptions.h"
#include "../level/names.h"
#include "../level/journal.h"
#include "../level/journal_text.h"
#include "../level/monster_kinds.h"
#include "page_curl.h"
#include <GL/gl.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <deque>
#include <vector>
#include <utility>
#include "../core/paths.h"

using namespace ui;
using namespace journal_view;

namespace {

// ---- layout ----------------------------------------------------------------

constexpr Rect COVER = {9, 10, 138, 75};
constexpr float SPINE = RIGHT_PAGE.x;
constexpr float CLOTH_TILE = 20.f; // units per repeat of the cloth texture
constexpr int EDGE_LINES = 7; // page edges showing, both sides together
constexpr float EDGE_STEP = 0.24f; // how far each page under sticks out
constexpr Rect STRAP_BAND = {8.1f, 8.8f, 1.8f, 77.4f}; // round the left cover edge, a little past top and bottom
constexpr float HEADBAND_W = 2.6f;
constexpr float HEADBAND_OUT = 0.9f; // past the pages' top and bottom

constexpr float RIBBON_IN = 3.f; // the ribbons lie over the page edge, then hang out of the book
constexpr float RIBBON_LEN = 14.f;
constexpr float RIBBON_H = 6.f;
constexpr float RIBBON_TOP = 76.f;
constexpr float RIBBON_STEP = 9.f;
constexpr float RIBBON_SHADOW = 1.f; // texture margin right and below
constexpr float RIBBON_TEX_LEN = 16.f; // the ribbon in the texture; shorter ones crop it
constexpr float RIBBON_TEX_W = RIBBON_TEX_LEN + RIBBON_SHADOW; // the texture: the ribbon and its shadow
constexpr float RIBBON_DEPTH_MAX = EDGE_LINES * EDGE_STEP; // the deepest page edge
constexpr float RIBBON_OVERHANG = RIBBON_LEN - RIBBON_IN + RIBBON_DEPTH_MAX + RIBBON_SHADOW; // past a turning page
constexpr float RIBBON_NEW_AT = 9.5f; // a section with new entries: the mark on its ribbon, in from the end
constexpr float RIBBON_RIDE_EASE = 0.3f; // of a turn: a riding ribbon slides in to the page, and out at the end

constexpr float CORNER = 7.f; // page turn corners
constexpr float ANSWER_W = 36.f;
constexpr float ANSWER_H = 7.f;

constexpr float FLIP_MS = 650.f; // a page over, clicked or let go of
constexpr float CORNER_LIFT = 10.f; // how high the bottom corner swings on its way over
constexpr float PEEK_X = 6.f; // the hovered corner lifts this far in and up
constexpr float PEEK_Y = 3.f;
constexpr float CLICK_SLOP = 1.5f; // a press let go of within this is a click, not a drag

constexpr const char* HAND_FONT = MEDIA_DIR
	"textures/fonts/kalam.png"; // Kalam (OFL, media/textures/fonts/kalam-OFL.txt)
constexpr const char* STAMP_FONT = MEDIA_DIR
	"textures/fonts/courier.png"; // Courier 10 Pitch (Bitstream, media/textures/fonts/courier-LICENSE.txt)

static_assert(static_cast<int>(JournalScreen::Section::Expedition) == static_cast<int>(JournalPart::Expedition));
static_assert(JOURNAL_PART_COUNT == static_cast<int>(JournalScreen::Section::Log));

constexpr Color PLUM = {0.40f, 0.15f, 0.36f}; // the diary's ribbon
constexpr Color SLATE = {0.30f, 0.34f, 0.40f}; // the expedition's

struct SectionLook {
	const char* name;
	const char* letter; // on its ribbon
	Color colour;
};
constexpr std::array<SectionLook, JournalScreen::SECTION_COUNT> SECTIONS = {{
	{"Diary", "D", PLUM},
	{"Creatures", "M", INK_RED},
	{"Riddles", "R", LAPIS},
	{"Field notes", "F", GOLD_DIM},
	{"Inscriptions", "I", INK_GREEN},
	{"Expedition", "E", SLATE},
	{"Log", "L", BRONZE},
}};

Rect visibleArea() { return ui::visibleArea(CANVAS_W, CANVAS_H, Game().render.resX, Game().render.resY); }

// A ribbon hanging out of the right or left page edge, `depth` further out.
Rect ribbonRect(int index, bool left, float depth) {
	float x = left ? LEFT_PAGE.x + RIBBON_IN - RIBBON_LEN - depth : RIGHT_PAGE.x + RIGHT_PAGE.w - RIBBON_IN + depth;
	return {x, RIBBON_TOP - static_cast<float>(index) * RIBBON_STEP, RIBBON_LEN, RIBBON_H};
}

Rect cornerRect(int side) {
	const Rect& p = pageRect(side);
	return side == 0 ? Rect{p.x + 1, p.y + 1, CORNER, CORNER} : Rect{p.x + p.w - 1 - CORNER, p.y + 1, CORNER, CORNER};
}

// A silk ribbon with a frayed swallowtail end (media/textures/ui/ribbon.png, tools/textures/ribbon.py). The texture is
// RIBBON_TEX_LEN x RIBBON_H plus one unit right and below for its shadow; a shorter ribbon crops its inner end. A left
// ribbon is mirrored, the swallowtail pointing left.
void ribbon(const Rect& r, bool left, Color c) {
	float u0 = (RIBBON_TEX_LEN - r.w) / RIBBON_TEX_W;
	Rect tex = {left ? r.x - RIBBON_SHADOW : r.x, r.y - RIBBON_SHADOW, r.w + RIBBON_SHADOW, r.h + RIBBON_SHADOW};
	texturedRect(tex, Game().assets.textures.ribbon.ID(), c, left ? 1.f : u0, 0.f, left ? u0 : 1.f, 1.f);
	beginShapes();
}

// Page turn arrow in a page corner; `left` points to the previous spread.
void cornerArrow(const Rect& r, bool left, Color c) {
	float cy = r.cy();
	float tip = left ? r.x + 1.f : r.x + r.w - 1.f;
	float base = left ? r.x + r.w - 2.f : r.x + 2.f;
	triangle(tip, cy, base, cy + 2.f, base, cy - 2.f, c, 1.f);
}

} // namespace

namespace journal_view {

const Rect& pageRect(int side) { return side == 0 ? LEFT_PAGE : RIGHT_PAGE; }

Rect answerRect(int side) {
	const Rect& p = pageRect(side);
	return {p.cx() - ANSWER_W / 2, p.y + 9.f, ANSWER_W, ANSWER_H};
}

Color darker(Color c, float f) { return {c.r * f, c.g * f, c.b * f}; }

} // namespace journal_view

// ---- pages -----------------------------------------------------------------

int JournalScreen::PageCount(Section s) const {
	if (s == Section::Diary)
		return static_cast<int>(Game().journal.Diary().size());
	if (s == Section::Riddles)
		return static_cast<int>(Game().journal.Riddles().size());
	if (s == Section::Creatures)
		return static_cast<int>(Game().journal.Creatures().size());
	if (s == Section::Inscriptions)
		return static_cast<int>(Game().journal.Inscriptions().size());
	if (s == Section::Expedition)
		return static_cast<int>(Game().journal.Pages().size());
	if (s == Section::Log)
		return static_cast<int>(logPages.size());
	return static_cast<int>(Game().journal.Notes().size());
}

int JournalScreen::SpreadCount(Section s) const { return std::max(1, (PageCount(s) + 1) / 2); }

int JournalScreen::FirstPage(int s) const {
	int pages = 0;
	for (int i = 0; i < s; i++)
		pages += SpreadCount(static_cast<Section>(i)) * 2;
	return pages;
}

bool JournalScreen::Shown(int ribbon) const { return PageCount(static_cast<Section>(ribbon)) > 0; }

// ---- ribbons ---------------------------------------------------------------

JournalScreen::RibbonPlace JournalScreen::PlaceOf(int ribbon, const Opening& at) const {
	int first = FirstPage(ribbon);
	int before = PagesBefore(at);
	bool left = first <= before;
	int between = left ? before - first : first - before - 2; // under the right page: from the one after it
	return {
		left,
		static_cast<float>(between) / static_cast<float>(PagesTotal()) * RIBBON_DEPTH_MAX,
		left && between == 0
	};
}

bool JournalScreen::Riding(int ribbon) const {
	if (!turn.active)
		return false;
	int first = FirstPage(ribbon);
	int from = PagesBefore(turn.from);
	int to = PagesBefore(turn.to);
	return std::min(from, to) < first && first <= std::max(from, to);
}

float JournalScreen::RidingDepth(int ribbon) const {
	float t = turn.progress;
	auto smooth = [](float x) {
		x = std::clamp(x, 0.f, 1.f);
		return x * x * (3 - 2 * x);
	};
	if (t < 0.5f)
		return PlaceOf(ribbon, turn.from).depth * (1 - smooth(t / RIBBON_RIDE_EASE));
	return PlaceOf(ribbon, turn.to).depth * smooth((t - 1 + RIBBON_RIDE_EASE) / RIBBON_RIDE_EASE);
}

int JournalScreen::EntryOnPage(int side) const {
	int page = spread[std::to_underlying(section)] * 2 + side;
	return page < PageCount(section) ? page : -1;
}

int JournalScreen::RiddleOnPage(int side) const { return section == Section::Riddles ? EntryOnPage(side) : -1; }

bool JournalScreen::CanAnswer(int side) const {
	int index = RiddleOnPage(side);
	return index >= 0 && !Game().journal.Riddles()[static_cast<size_t>(index)].solved;
}

void JournalScreen::ShowToast(const std::string& text) { toast.Show(text, GameClock::now()); }

void JournalScreen::SeeShown() {
	for (const auto& [s, index] : shownNew)
		Game().journal.MarkSeen(static_cast<JournalPart>(s), static_cast<size_t>(index));
	shownNew.clear();
}

void JournalScreen::ViewTo(UiViewState& view) const {
	view.journalSection = static_cast<int>(section);
	view.spread = spread;
}

void JournalScreen::ViewFrom(const UiViewState& view) {
	SeeShown();
	turn = {};
	pointer.Clear();
	section = static_cast<Section>(std::clamp(view.journalSection, 0, SECTION_COUNT - 1));
	spread = view.spread;
}

void JournalScreen::OpenDiary(const char* closes) {
	SeeShown();
	turn = {};
	pointer.Clear();
	section = Section::Diary;
	spread[std::to_underlying(section)] = SpreadCount(section) - 1;
	closesTo = closes;
	Game().assets.sounds.pageTurn.Play();
}

// ---- page turns ------------------------------------------------------------

void JournalScreen::Turn(int by) {
	Opening to = Open();
	to.spread = std::clamp(to.spread + by, 0, SpreadCount(section) - 1);
	GoTo(to);
}

void JournalScreen::GoTo(Opening to) {
	if (turn.dragging)
		return;
	if (turn.active) // a turn under way lands first
		FinishTurn();
	if (to.section == section && to.spread == spread[std::to_underlying(section)])
		return;
	StartTurn(to, false);
}

void JournalScreen::StartTurn(Opening to, bool dragged) {
	SeeShown(); // leaving the open spread
	Opening from = Open();
	if (pagesFailed) { // nothing to bend: straight there
		section = to.section;
		spread[std::to_underlying(section)] = to.spread;
		return;
	}
	turn = {};
	turn.active = true;
	turn.from = from;
	turn.to = to;
	turn.forward = to.section != from.section ? to.section > from.section : to.spread > from.spread;
	turn.dragging = dragged;
	turn.target = static_cast<float>(M_PI);
	turn.lastMs = GameClock::now();
	pointer.Move({});
	Game().assets.sounds.pageTurn.Play();
}

void JournalScreen::FinishTurn() {
	if (turn.target > 0) {
		section = turn.to.section;
		spread[std::to_underlying(section)] = turn.to.spread;
	}
	turn = {};
}

void JournalScreen::AdvanceTurn() {
	int now = GameClock::now();
	auto dt = static_cast<float>(std::clamp(now - turn.lastMs, 0, 100));
	turn.lastMs = now;
	if (!turn.active || turn.dragging)
		return;
	float step = dt * static_cast<float>(M_PI) / FLIP_MS;
	turn.angle = turn.target > turn.angle ? std::min(turn.target, turn.angle + step)
										  : std::max(turn.target, turn.angle - step);
	float ease = dt / FLIP_MS * 3.f;
	turn.spread += std::clamp(1.f - turn.spread, -ease, ease);
	if (turn.angle == turn.target && std::abs(turn.spread - 1.f) < 0.01f)
		FinishTurn();
}

// Canvas x -> page-local x of the turning page: from the spine outwards.
float JournalScreen::CornerLocalX(float canvasX) const { return turn.forward ? canvasX - SPINE : SPINE - canvasX; }

// ---- input -----------------------------------------------------------------

JournalScreen::Hit JournalScreen::HitAt(float x, float y) const {
	if (turn.active)
		return {};
	for (int i = 0; i < SECTION_COUNT; i++) {
		if (!Shown(i))
			continue;
		RibbonPlace place = PlaceOf(i, Open());
		bool underPage = !place.top && (place.left ? x > LEFT_PAGE.x : x < RIGHT_PAGE.x + RIGHT_PAGE.w);
		if (!underPage && ribbonRect(i, place.left, place.depth).contains(x, y))
			return {Target::Ribbon, i};
	}
	if (spread[std::to_underlying(section)] > 0 && cornerRect(0).contains(x, y))
		return {Target::PrevPage, 0};
	if (spread[std::to_underlying(section)] < SpreadCount(section) - 1 && cornerRect(1).contains(x, y))
		return {Target::NextPage, 0};
	if (CanAnswer(0) && answerRect(0).contains(x, y))
		return {Target::AnswerLeft, 0};
	if (CanAnswer(1) && answerRect(1).contains(x, y))
		return {Target::AnswerRight, 0};
	return {};
}

void JournalScreen::Activate(const Hit& hit) {
	switch (hit.target) {
	case Target::Ribbon:
		GoTo({static_cast<Section>(hit.ribbon), spread[static_cast<size_t>(hit.ribbon)]});
		break;
	case Target::PrevPage:
		Turn(-1);
		break;
	case Target::NextPage:
		Turn(1);
		break;
	case Target::AnswerLeft:
	case Target::AnswerRight:
		Game().ui.riddle->AskLate(static_cast<size_t>(RiddleOnPage(hit.target == Target::AnswerLeft ? 0 : 1)));
		Game().ui.screen = Screen::Riddle;
		break;
	case Target::None:
		break;
	}
}

void JournalScreen::KeyPressed(unsigned char key) { Navigate(navKeyOf(InputKey::Char(key), Game().settings.controls)); }

void JournalScreen::SpecialKeyPressed(int key) { Navigate(navKeyOf(InputKey::Special(key), Game().settings.controls)); }

// Left / Right turn the page, Up / Down go to the next section with pages. On a riddles spread Enter focuses its first
// Answer button, then answers; with an Answer focused, Left / Right go to the other one before they turn the page.
void JournalScreen::Navigate(NavKey key) {
	auto other = [this](int by) { // the next section with a ribbon
		for (int step = 1; step < SECTION_COUNT; step++) {
			int s = (static_cast<int>(section) + SECTION_COUNT + by * step) % SECTION_COUNT;
			if (Shown(s)) {
				GoTo({static_cast<Section>(s), spread[static_cast<size_t>(s)]});
				return;
			}
		}
	};
	const Target focus = turn.active ? Target::None : pointer.hovered.target;
	switch (key) {
	case NavKey::Left:
		if (focus == Target::AnswerRight && CanAnswer(0))
			pointer.Move({Target::AnswerLeft, 0});
		else
			Turn(-1);
		return;
	case NavKey::Right:
		if (focus == Target::AnswerLeft && CanAnswer(1))
			pointer.Move({Target::AnswerRight, 0});
		else
			Turn(1);
		return;
	case NavKey::Up:
		other(-1);
		return;
	case NavKey::Down:
		other(1);
		return;
	case NavKey::Activate:
		if (focus == Target::AnswerLeft || focus == Target::AnswerRight) {
			const Hit answer = pointer.hovered;
			pointer.Move({});
			Activate(answer);
		} else if (!turn.active && CanAnswer(0))
			pointer.Move({Target::AnswerLeft, 0});
		else if (!turn.active && CanAnswer(1))
			pointer.Move({Target::AnswerRight, 0});
		return;
	case NavKey::None:
		return;
	}
}

void JournalScreen::MouseMotion(int x, int y) {
	float cx = 0.f;
	float cy = 0.f;
	toCanvas(visibleArea(), Game().render.resX, Game().render.resY, x, y, cx, cy);
	if (turn.dragging) {
		// The corner moves with the mouse, from where it was grabbed.
		turn.cornerX = RIGHT_PAGE.w + CornerLocalX(cx) - CornerLocalX(turn.pressX);
		turn.cornerY = cy - turn.pressY;
		return;
	}
	pointer.Move(HitAt(cx, cy));
}

void JournalScreen::MouseFunction(int button, int state, int x, int y) {
	MouseMotion(x, y);
	if (button == MOUSE_WHEEL_UP || button == MOUSE_WHEEL_DOWN) {
		if (state == GLUT_DOWN)
			Turn(button == MOUSE_WHEEL_UP ? -1 : 1);
		return;
	}
	if (button != MOUSE_LEFT_BUTTON)
		return;
	float cx = 0.f;
	float cy = 0.f;
	toCanvas(visibleArea(), Game().render.resX, Game().render.resY, x, y, cx, cy);
	if (state == GLUT_DOWN) {
		const Hit at = pointer.hovered;
		bool corner = at.target == Target::PrevPage || at.target == Target::NextPage;
		if (corner && !pagesFailed) { // grab the page by its corner: the turn follows the drag
			Opening to = Open();
			to.spread += at.target == Target::NextPage ? 1 : -1;
			StartTurn(to, true);
			turn.pressX = cx;
			turn.pressY = cy;
			turn.cornerX = RIGHT_PAGE.w;
			pointer.Down({}, button);
		} else
			pointer.Down(at, button);
		return;
	}
	if (turn.dragging) {
		// Let go: a click turns the page; a drag goes over if the corner is past the spine, back if not.
		turn.dragging = false;
		bool click = std::hypot(cx - turn.pressX, cy - turn.pressY) < CLICK_SLOP;
		turn.target = click || turn.cornerX < 0 ? static_cast<float>(M_PI) : 0.f;
		turn.lastMs = GameClock::now();
		if (!click) { // from where the corner is, on to the ellipse
			float ex = turn.cornerX / RIGHT_PAGE.w;
			float ey = turn.cornerY / CORNER_LIFT;
			turn.spread = std::hypot(ex, ey);
			turn.angle = std::atan2(ey, ex);
			if (turn.angle < 0)
				turn.angle = turn.cornerX < 0 ? static_cast<float>(M_PI) : 0.f;
		}
		return;
	}
	if (const Hit fired = pointer.Up(pointer.hovered, button); fired.target != Target::None) {
		Activate(fired);
		pointer.Move({}); // the riddle screen, or the new page, has its own targets
	}
}

// ---- drawing ---------------------------------------------------------------

void JournalScreen::Draw() {
	if (!fontsLoaded) {
		loadScreenFonts(title, heading, body, small, 7.f);
		handHeading.Load(HAND_FONT, 5.f, 0.12f, true);
		hand.Load(HAND_FONT, 3.6f, 0.08f, true);
		handSmall.Load(HAND_FONT, 3.f, 0.06f, true);
		handTiny.Load(HAND_FONT, 2.5f, 0.05f, true);
		stamp.Load(STAMP_FONT, 3.6f, 0.f, true);
		stampSmall.Load(STAMP_FONT, 2.5f, 0.f, true);
		fontsLoaded = true;
	}
	PaginateLog();
	for (int s = 0; s < SECTION_COUNT; s++)
		spread[static_cast<size_t>(s)] = std::clamp(
			spread[static_cast<size_t>(s)],
			0,
			SpreadCount(static_cast<Section>(s)) - 1
		);
	// An empty section has no ribbon to come back to: the book opens on the first one with pages.
	if (!turn.active && !Shown(static_cast<int>(section)))
		for (int s = 0; s < SECTION_COUNT; s++)
			if (Shown(s)) {
				section = static_cast<Section>(s);
				break;
			}
	AdvanceTurn();

	// The pages a turn shows; a hovered corner lifts a little, showing the page behind.
	const Opening open = Open();
	const Hit& hovered = pointer.hovered;
	bool peek = !turn.active && (hovered.target == Target::PrevPage || hovered.target == Target::NextPage);
	bool turning = turn.active || peek;
	bool forward = turn.active ? turn.forward : hovered.target == Target::NextPage;
	Opening to = turn.to;
	if (peek) {
		to = open;
		to.spread += forward ? 1 : -1;
	}
	PageCurl curl{RIGHT_PAGE.w, RIGHT_PAGE.h};
	curl.overhang = RIBBON_OVERHANG;
	if (turn.dragging) {
		curl.cornerX = turn.cornerX;
		curl.cornerY = turn.cornerY;
	} else if (turn.active) {
		curl.cornerX = RIGHT_PAGE.w * std::cos(turn.angle) * turn.spread;
		curl.cornerY = CORNER_LIFT * std::sin(turn.angle) * turn.spread;
	} else {
		curl.cornerX = RIGHT_PAGE.w - PEEK_X;
		curl.cornerY = PEEK_Y;
	}
	clampCorner(curl);
	turn.progress = turn.active ? curlProgress(curl) : 0.f;

	std::array<int, PAGE_SLOTS> tex{};
	auto slot = [&tex](PageSlot k) -> int& { return tex[std::to_underlying(k)]; };
	if (!turning) {
		slot(PageSlot::Left) = RenderPage(PageSlot::Left, open, 0, true);
		slot(PageSlot::Right) = RenderPage(PageSlot::Right, open, 1, true);
	} else {
		slot(PageSlot::Left) = RenderPage(PageSlot::Left, forward ? open : to, 0, false);
		slot(PageSlot::Right) = RenderPage(PageSlot::Right, forward ? to : open, 1, false);
		slot(PageSlot::Front) = RenderPage(PageSlot::Front, open, forward ? 1 : 0, false);
		slot(PageSlot::Back) = RenderPage(PageSlot::Back, to, forward ? 0 : 1, false);
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	Rect area = visibleArea();
	glOrtho(area.x, area.x + area.w, area.y, area.y + area.h, -200, 200);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);

	backdrop(area, Game().assets.textures.loadingBackground.ID());
	titleBar(title, CANVAS_W / 2, "Journal", SCREEN_TABS_TITLE_REACH);
	DrawBook(open);
	DrawRibbons(false);
	if (pagesFailed) {
		for (int side = 0; side < 2; side++) {
			const Rect& p = pageRect(side);
			texturedRect(
				p,
				Game().assets.textures.journalPaper.ID(),
				{1, 1, 1},
				side == 0 ? 1.f : 0.f,
				0.f,
				side == 0 ? 0.f : 1.f,
				1.f
			);
			DrawPageContent(open, side, true);
		}
	} else {
		glDisable(GL_BLEND);
		texturedRect(LEFT_PAGE, slot(PageSlot::Left), {1, 1, 1});
		texturedRect(RIGHT_PAGE, slot(PageSlot::Right), {1, 1, 1});
	}
	beginShapes();
	line(SPINE, LEFT_PAGE.y, SPINE, LEFT_PAGE.y + LEFT_PAGE.h, BLACK, 0.35f, 1.f); // the gutter
	DrawRibbons(true); // under the turning page

	if (turning && !pagesFailed) {
		PageCurlPlacement at;
		at.spineX = SPINE;
		at.bottomY = RIGHT_PAGE.y;
		at.mirror = !forward;
		at.front = slot(PageSlot::Front);
		at.back = slot(PageSlot::Back);
		at.eyeX = SPINE;
		at.eyeY = RIGHT_PAGE.cy();
		drawPageCurl(curl, at);
	}
	DrawCorners();
	DrawFooter();
	ScreenTabs::Draw();

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glColor3f(1, 1, 1);
}

int JournalScreen::RenderPage(PageSlot slot, Opening at, int side, bool live) {
	if (pagesFailed)
		return 0;
	const Rect& p = pageRect(side);
	// A turning page reaches past its free edge for the ribbons hanging out of it (PageCurl::overhang).
	bool turning = slot == PageSlot::Front || slot == PageSlot::Back;
	float overhang = turning ? RIBBON_OVERHANG : 0.f;
	Rect r = {side == 0 ? p.x - overhang : p.x, p.y, p.w + overhang, p.h};
	Rect area = visibleArea();
	float perUnit = static_cast<float>(Game().render.resY) / area.h;
	RenderTarget& target = pages[std::to_underlying(slot)];
	if (!target.Begin(static_cast<int>(std::lround(r.w * perUnit)), static_cast<int>(std::lround(r.h * perUnit)))) {
		pagesFailed = true;
		return 0;
	}
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(r.x, r.x + r.w, r.y, r.y + r.h, -200, 200);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	// The ribbons riding on a turning page: under the paper, only hanging out past its edge, or on it as they lie
	// on the book on this side (RibbonPlace::top).
	auto riding = [&](bool top) {
		if (!turning)
			return;
		beginShapes();
		for (int i = 0; i < SECTION_COUNT; i++)
			if (Shown(i) && Riding(i) && PlaceOf(i, slot == PageSlot::Front ? turn.from : turn.to).top == top)
				DrawRibbon(i, side == 0, RidingDepth(i), static_cast<int>(turn.to.section) == i, false);
	};
	riding(false);
	glDisable(GL_BLEND);
	// The paper texture has its spine on the left: the left page draws it mirrored.
	texturedRect(
		p,
		Game().assets.textures.journalPaper.ID(),
		{1, 1, 1},
		side == 0 ? 1.f : 0.f,
		0.f,
		side == 0 ? 0.f : 1.f,
		1.f
	);
	DrawPageContent(at, side, live);
	riding(true);
	target.End();
	return target.TextureID();
}

void JournalScreen::DrawBook(const Opening& at) {
	beginShapes();
	fillRect({COVER.x + 1.f, COVER.y - 1.2f, COVER.w, COVER.h}, BLACK, BLACK, 0.5f); // shadow
	texturedRect(
		COVER,
		Game().assets.textures.journalCloth.ID(),
		CLOTH,
		0.f,
		0.f,
		COVER.w / CLOTH_TILE,
		COVER.h / CLOTH_TILE
	);
	beginShapes();
	// The boards' edges, worn lighter; the spine of the cover sinks between them.
	strokeRect(COVER, darker(CLOTH, 0.35f), 1.f, 2.f);
	strokeRect(COVER.inset(0.6f), darker(CLOTH, 1.25f), 0.25f, 1.f);
	fillRect({SPINE - 3.f, COVER.y, 3.f, COVER.h}, BLACK, BLACK, 0.12f);
	fillRect({SPINE - 1.f, COVER.y, 2.f, COVER.h}, BLACK, BLACK, 0.2f);

	// Headbands: a striped red and white silk roll peeking out above and below the pages at the spine.
	for (float y : {LEFT_PAGE.y + LEFT_PAGE.h - 0.3f, LEFT_PAGE.y - HEADBAND_OUT}) {
		Rect band = {SPINE - HEADBAND_W / 2, y, HEADBAND_W, HEADBAND_OUT + 0.3f};
		fillRect(band, INK_RED, darker(INK_RED, 0.6f), 1.f);
		for (int k = 0; k < static_cast<int>(HEADBAND_W / 0.6f); k++) {
			float x = band.x + 0.3f + static_cast<float>(k) * 0.6f;
			line(x, band.y, x, band.y + band.h, PAPER, 0.55f, 1.f);
		}
	}

	// The page block: the edges of the pages under the open ones, more on the side the book is thicker.
	float read = static_cast<float>(PagesBefore(at)) / static_cast<float>(PagesTotal());
	int left = std::clamp(static_cast<int>(std::lround(read * EDGE_LINES)), 1, EDGE_LINES - 1);
	int right = EDGE_LINES - left;
	auto edges = [](const Rect& page, int count, float dir) {
		for (int k = count; k >= 1; k--) {
			auto d = static_cast<float>(k) * EDGE_STEP;
			Rect r = {dir < 0 ? page.x - d : page.x, page.y - d * 0.5f, page.w + d, page.h + d * 0.2f};
			fillRect(r, darker(PAPER, 0.80f), darker(PAPER, 0.72f), 1.f);
			fillRect(
				dir < 0 ? Rect{r.x, r.y, 0.12f, r.h} : Rect{r.x + r.w - 0.12f, r.y, 0.12f, r.h},
				BLACK,
				BLACK,
				0.25f
			);
		}
	};
	edges(LEFT_PAGE, left, -1.f);
	edges(RIGHT_PAGE, right, 1.f);

	// The elastic strap round the left cover edge: cream, finely ribbed, rounder in the middle.
	const Rect& s = STRAP_BAND;
	fillRect({s.x + s.w, s.y + 0.6f, 0.5f, s.h - 1.2f}, BLACK, BLACK, 0.25f); // its shadow on the cover
	fillRect(s, STRAP, darker(STRAP, 0.9f), 1.f);
	fillRect({s.x, s.y, s.w * 0.25f, s.h}, BLACK, BLACK, 0.12f);
	fillRect({s.x + s.w * 0.75f, s.y, s.w * 0.25f, s.h}, BLACK, BLACK, 0.18f);
	for (int k = 0; k < static_cast<int>(s.h / 0.45f); k++) {
		float y = s.y + 0.3f + static_cast<float>(k) * 0.45f;
		line(s.x, y, s.x + s.w, y, BLACK, 0.06f, 1.f);
	}
	fillRect({s.x, s.y, s.w, 0.8f}, BLACK, BLACK, 0.25f); // where it wraps round behind the board
	fillRect({s.x, s.y + s.h - 0.8f, s.w, 0.8f}, BLACK, BLACK, 0.25f);
}

void JournalScreen::DrawRibbon(int ribbon, bool left, float depth, bool open, bool hovered) {
	Rect r = ribbonRect(ribbon, left, depth);
	const SectionLook& look = SECTIONS[static_cast<size_t>(ribbon)];
	::ribbon(r, left, hovered ? darker(look.colour, 1.25f) : look.colour);
	if (open) // tucked in between the open pages
		fillRect({left ? r.x + r.w - 1.f : r.x, r.y, 1.f, r.h}, BLACK, BLACK, 0.25f);
	if (ribbon < JOURNAL_PART_COUNT && Game().journal.AnyNew(static_cast<JournalPart>(ribbon)))
		newMark(left ? r.x + RIBBON_NEW_AT : r.x + r.w - RIBBON_NEW_AT, r.cy(), 1.1f);
	if (open || hovered) { // its letter, written on the ribbon end
		beginText();
		float x = left ? r.x + 6.2f - handSmall.TextWidth(look.letter) : r.x + r.w - 6.2f;
		textMiddle(handSmall, x, r.cy(), look.letter, PAPER, 0.9f);
		beginShapes();
	}
}

void JournalScreen::DrawRibbons(bool top) {
	beginShapes();
	Section shown = turn.active ? turn.to.section : section; // a turn to another section shows its letter at once
	for (int i = 0; i < SECTION_COUNT; i++) {
		if (!Shown(i) || Riding(i))
			continue; // no ribbon yet, or drawn on the turning page
		// Still, but the pages move under it: from where it lay to where it lands.
		RibbonPlace place = PlaceOf(i, turn.active ? turn.from : Open());
		if (turn.active) {
			RibbonPlace to = PlaceOf(i, turn.to);
			place.depth += (to.depth - place.depth) * turn.progress;
			place.top = turn.progress < 0.5f ? place.top : to.top;
		}
		if (place.top != top)
			continue;
		bool isHovered = pointer.hovered.target == Target::Ribbon && pointer.hovered.ribbon == i;
		DrawRibbon(i, place.left, place.depth, static_cast<int>(shown) == i, isHovered);
	}
	// The hovered ribbon's name, on a small dark label next to it on the page.
	const Hit& hovered = pointer.hovered;
	if (!top || hovered.target != Target::Ribbon)
		return;
	const char* name = SECTIONS[static_cast<size_t>(hovered.ribbon)].name;
	RibbonPlace place = PlaceOf(hovered.ribbon, Open());
	Rect r = ribbonRect(hovered.ribbon, place.left, place.depth);
	float w = small.TextWidth(name) + 3.f;
	Rect label = {place.left ? r.x + r.w + 1.f : r.x - w - 1.f, r.y + 0.8f, w, 4.4f};
	fillRect(label, PANEL_TOP, PANEL_BOTTOM, 0.95f);
	strokeRect(label, GOLD_DIM, 1.f, 1.f);
	beginText();
	textMiddle(small, label.x + 1.5f, label.cy(), name, GOLD);
	beginShapes();
}

void JournalScreen::DrawCorners() {
	if (turn.active)
		return;
	beginShapes();
	int open = spread[std::to_underlying(section)];
	if (open > 0)
		cornerArrow(cornerRect(0), true, pointer.hovered.target == Target::PrevPage ? INK_RED : PENCIL);
	if (open < SpreadCount(section) - 1)
		cornerArrow(cornerRect(1), false, pointer.hovered.target == Target::NextPage ? INK_RED : PENCIL);
}

void JournalScreen::DrawFooter() {
	beginText();
	if (float alpha = toast.Alpha(GameClock::now()); alpha > 0.f)
		textCentered(body, CANVAS_W / 2, 6.2f, toast.text.c_str(), {1.f, 0.9f, 0.6f}, alpha);
	const bool answers = CanAnswer(0) || CanAnswer(1);
	textCentered(
		small,
		CANVAS_W / 2,
		2.2f,
		answers ? "Ribbons, Up / Down: section    Corners, Left / Right, wheel: turn the page    Enter: answer    "
				  "J / Esc: close"
				: "Ribbons, Up / Down: section    Corners (click or drag), Left / Right, wheel: turn the page    "
				  "J / Esc: close",
		{0.55f, 0.45f, 0.30f}
	);
	beginShapes();
}
