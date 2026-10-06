#include "journal_view.h"
#include "../graphics/gl_includes.h"
#include "../input/input.h"
#include "../state/game_state.h"
#include "../world/journal.h"
#include "../world/monster_kinds.h"
#include "page_curl.h"
#include <GL/gl.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace ui;

namespace {

// ---- layout ----------------------------------------------------------------

constexpr Rect COVER = {9, 10, 138, 75};
constexpr Rect LEFT_PAGE = {12, 12.5f, 66, 70};
constexpr Rect RIGHT_PAGE = {78, 12.5f, 66, 70};
constexpr float SPINE = RIGHT_PAGE.x;
constexpr float PAGE_PAD = 6.f;
constexpr float CLOTH_TILE = 20.f;					   // units per repeat of the cloth texture
constexpr int EDGE_LINES = 7;						   // page edges showing, both sides together
constexpr float EDGE_STEP = 0.24f;					   // how far each page under sticks out
constexpr Rect STRAP_BAND = {8.1f, 8.8f, 1.8f, 77.4f}; // round the left cover edge, a little past top and bottom
constexpr float HEADBAND_W = 2.6f;
constexpr float HEADBAND_OUT = 0.9f; // past the pages' top and bottom

constexpr float RIBBON_IN = 3.f; // the ribbons lie over the page edge, then hang out of the book
constexpr float RIBBON_LEN = 14.f;
constexpr float RIBBON_H = 6.f;
constexpr float RIBBON_TOP = 76.f;
constexpr float RIBBON_STEP = 9.f;
constexpr float RIBBON_SHADOW = 1.f;						   // texture margin right and below
constexpr float RIBBON_TEX_LEN = 16.f;						   // the ribbon in the texture; shorter ones crop it
constexpr float RIBBON_TEX_W = RIBBON_TEX_LEN + RIBBON_SHADOW; // the texture: the ribbon and its shadow
constexpr float RIBBON_DEPTH_MAX = EDGE_LINES * EDGE_STEP;	   // the deepest page edge
constexpr float RIBBON_OVERHANG = RIBBON_LEN - RIBBON_IN + RIBBON_DEPTH_MAX + RIBBON_SHADOW; // past a turning page
constexpr float RIBBON_RIDE_EASE = 0.3f; // of a turn: a riding ribbon slides in to the page, and out at the end

constexpr float CORNER = 7.f; // page turn corners
constexpr float ANSWER_W = 36.f;
constexpr float ANSWER_H = 7.f;

constexpr float FLIP_MS = 650.f;	// a page over, clicked or let go of
constexpr float CORNER_LIFT = 10.f; // how high the bottom corner swings on its way over
constexpr float PEEK_X = 6.f;		// the hovered corner lifts this far in and up
constexpr float PEEK_Y = 3.f;
constexpr float CLICK_SLOP = 1.5f; // a press let go of within this is a click, not a drag

constexpr const char* HAND_FONT = "fonts/kalam.png";	// Kalam (OFL, fonts/kalam-OFL.txt)
constexpr const char* STAMP_FONT = "fonts/courier.png"; // Courier 10 Pitch (Bitstream, fonts/courier-LICENSE.txt)
constexpr float FADED = 0.7f;							// alpha of pencil notes that matter less

constexpr float NUMBER_TOP = 5.8f; // page number baseline below the page top
constexpr float XREF_TOP = 2.9f;   // the pencilled level above it
constexpr float HEADING_TOP = 11.5f;
constexpr float PHOTO_TOP = 16.f;
constexpr float PHOTO_W = 40.f;
constexpr float PHOTO_H = 17.f;
constexpr float PHOTO_BORDER = 1.1f;
constexpr float CAPTION_TOP = 36.6f;
constexpr float ROWS_TOP = 40.8f; // first note's baseline below the page top
constexpr float ROW_STEP = 3.2f;
constexpr float MARGIN_W = 10.f; // the label column on the left of the notes
constexpr float FORM_COLUMN = 22.f;
constexpr float BLANK_W = 7.f; // the line of a field not filled in yet

constexpr float SKETCH_YAW = -25.f;	   // from the side, turned a little towards the reader
constexpr float SKETCH_TILT = 12.f;	   // seen a little from above
constexpr float SKETCH_LINE_PX = 1.6f; // at 720 rows

constexpr float QUESTION_TOP = 19.f;
constexpr float QUESTION_STEP = 4.4f;
constexpr float FIELD_NOTE_TOP = 19.f;
constexpr float FIELD_NOTE_STEP = 4.6f;
constexpr size_t MAX_QUESTION_LINES = 7;
constexpr size_t MAX_HINT_LINES = 2;

struct SectionLook {
	const char* name;
	const char* letter; // on its ribbon
	Color colour;
};
constexpr std::array<SectionLook, JournalScreen::SECTION_COUNT> SECTIONS = {{
	{"Creatures", "M", INK_RED},
	{"Riddles", "R", LAPIS},
	{"Field notes", "F", GOLD_DIM},
}};

Rect visibleArea() { return ui::visibleArea(CANVAS_W, CANVAS_H, Game().render.resX, Game().render.resY); }

const Rect& pageRect(int side) { return side == 0 ? LEFT_PAGE : RIGHT_PAGE; }

// A ribbon hanging out of the right or left page edge, `depth` further out.
Rect ribbonRect(int index, bool left, float depth) {
	float x = left ? LEFT_PAGE.x + RIBBON_IN - RIBBON_LEN - depth : RIGHT_PAGE.x + RIGHT_PAGE.w - RIBBON_IN + depth;
	return {x, RIBBON_TOP - static_cast<float>(index) * RIBBON_STEP, RIBBON_LEN, RIBBON_H};
}

Rect cornerRect(int side) {
	const Rect& p = pageRect(side);
	return side == 0 ? Rect{p.x + 1, p.y + 1, CORNER, CORNER} : Rect{p.x + p.w - 1 - CORNER, p.y + 1, CORNER, CORNER};
}

Rect answerRect(int side) {
	const Rect& p = pageRect(side);
	return {p.cx() - ANSWER_W / 2, p.y + 9.f, ANSWER_W, ANSWER_H};
}

Rect photoBox(const Rect& p) { return {p.cx() - PHOTO_W / 2, p.y + p.h - PHOTO_TOP - PHOTO_H, PHOTO_W, PHOTO_H}; }

Color darker(Color c, float f) { return {c.r * f, c.g * f, c.b * f}; }

// A silk ribbon with a frayed swallowtail end (textures/ui/ribbon.png, tools/textures/ribbon.py). The texture is
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

// Monster textures as a black and white print: grey from the luminance, a little more contrast.
void photoGrey(unsigned char* data, int pixels, int components) {
	for (int i = 0; i < pixels; i++) {
		unsigned char* p = data + static_cast<ptrdiff_t>(i) * components;
		auto channel = [p](int c) { return static_cast<float>(p[c]); };
		float lum = (0.3f * channel(0) + 0.59f * channel(1) + 0.11f * channel(2)) / 255.f;
		lum = std::clamp((std::pow(lum, 0.6f) - 0.5f) * 1.25f + 0.55f, 0.f, 1.f); // opens up the dark ones (bats)
		for (int c = 0; c < 3; c++)
			p[c] = static_cast<unsigned char>(255.f * lum);
	}
}

std::string upper(std::string s) {
	for (char& ch : s)
		ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
	return s;
}

// A heading in ink, underlined, centred on cx.
void underlinedHeading(Font& font, float cx, float y, const char* str) {
	float w = font.TextWidth(str);
	beginText();
	text(font, cx - w / 2, y, str, INK_BLUE);
	beginShapes();
	line(cx - w / 2 - 1.f, y + 0.4f, cx + w / 2 + 1.f, y + 0.4f, INK_BLUE, 0.85f, 1.5f);
}

// A small hand-drawn arrow pointing up, its foot at (x, y).
void upArrow(float x, float y, Color c) {
	line(x, y, x, y + 2.4f, c, 1.f, 1.2f);
	line(x - 0.7f, y + 1.6f, x, y + 2.4f, c, 1.f, 1.2f);
	line(x + 0.7f, y + 1.6f, x, y + 2.4f, c, 1.f, 1.2f);
}

// "~40 HP": the journal writes rough numbers, the way they are learnt.
int rough(int n) { return n < 20 ? n : static_cast<int>(std::lround(n / 5.0)) * 5; }

const char* moveNote(CreatureMove move, const MonsterType& t) {
	switch (move) {
	case CreatureMove::Leap:
		return "Leaps over pits and traps.";
	case CreatureMove::Swoop:
		return "Hangs from the ceiling, swoops down to bite.";
	case CreatureMove::Ambush:
		return "Lies in wait as a treasure chest!";
	case CreatureMove::Rise:
		return "Lies in its coffin, climbs out when I come near.";
	case CreatureMove::Summon:
		switch (t.boss.summon) {
		case Summon::DigOut:
			return "Calls its brood up out of the sand.";
		case Summon::Drop:
			return "Calls its kind down from the ceiling.";
		case Summon::Coffin:
			return "The dead climb out of their coffins at its call.";
		}
		break;
	case CreatureMove::Heal:
		return "Heals as it bites.";
	case CreatureMove::Surface:
		return "Lies under the water, only its eyes show.";
	case CreatureMove::Poison:
		return "Its poison burns on long after the bite.";
	case CreatureMove::Rear:
		return "Lies coiled, rears up when I come near.";
	case CreatureMove::Spit:
		return "Spits venom from afar. A jump clears it.";
	}
	return "";
}

struct FieldNoteText {
	const char* title;
	const char* text;
};
// By FieldNote. The rules as the game has them (PlayerStats, ItemBag, the mechanisms); keep them in step.
constexpr std::array<FieldNoteText, FIELD_NOTE_COUNT> FIELD_NOTES = {{
	{"Hit points", "Every bite, blow and trap costs me health: the red bar in the corner. When it runs out, the "
				   "expedition is over. A healing potion brings some back (H drinks the right one), and a new level "
				   "fills it up."},
	{"Experience", "Every creature I kill and every riddle I answer teaches me something. With enough of it I reach "
				   "a new level: more health and more stamina, both filled up at once, and any poison gone. Every "
				   "fifth level my skin gets "
				   "tougher, every eighth my arm stronger."},
	{"Stamina", "Jumping and running (Shift) tire me out: the yellow bar. Each jump takes a bite out of it, running "
				"drains it all the time. When I walk or stand, it comes back by itself. A stamina potion (0) helps in "
				"a hurry."},
	{"Potions", "Health draughts heal, stamina draughts give me my breath back. The rare ones work for good: "
				"Aphethamine for might, Stone Skin for armour, the Elixir of Life for more health. In a hurry, H and 0 "
				"drink the right one."},
	{"Weapons", "Club, sword, spear and bow, on the keys 1 to 4. The club bludgeons, the sword slashes, the spear "
				"and the arrows pierce, and every creature takes each of them differently: I note it down. Copies of "
				"my weapons turn up in chests, and now and then a creature leaves one; with enough copies a weapon "
				"can be upgraded in the inventory (U). The old club gains the most from it."},
	{"Keys and gates", "A key opens every gate of its colour on this level. Some gates answer to a lever of their "
					   "colour instead. The gate of a boss's lair stays shut until the boss is dead."},
	{"Poison", "Some stings and bites poison me: the health bar turns green and drains on after the fight, armour "
			   "or not, and it can kill me. The same poison again only starts over; a stronger or weaker one burns "
			   "beside it. An antidote (=) cures them all, and so does a new level."},
}};

// A tick before a solved riddle's answer, from its bottom left at (x, y).
void tick(float x, float y, Color c) {
	line(x, y + 1.4f, x + 1.f, y, c, 1.f, 2.f);
	line(x + 1.f, y, x + 3.f, y + 3.f, c, 1.f, 2.f);
}

// A field of the creature form: "HP = ~40", or the name and a blank line while it is not known.
void formField(Font& font, float x, float y, const std::string& name, const std::string& value) {
	std::string label = name + " = ";
	beginText();
	text(font, x, y, (label + value).c_str(), INK_BLUE);
	beginShapes();
	if (value.empty()) {
		float from = x + font.TextWidth(label.c_str());
		line(from, y + 0.5f, from + BLANK_W, y + 0.5f, PENCIL, 0.45f, 1.f);
	}
}

} // namespace

// ---- pages -----------------------------------------------------------------

int JournalScreen::PageCount(Section s) const {
	if (s == Section::Riddles)
		return static_cast<int>(Game().journal.Riddles().size());
	if (s == Section::Creatures)
		return static_cast<int>(Game().journal.Creatures().size());
	return static_cast<int>(Game().journal.Notes().size());
}

int JournalScreen::SpreadCount(Section s) const { return std::max(1, (PageCount(s) + 1) / 2); }

int JournalScreen::FirstPage(int s) const {
	int pages = 0;
	for (int i = 0; i < s; i++)
		pages += SpreadCount(static_cast<Section>(i)) * 2;
	return pages;
}

// ---- ribbons ---------------------------------------------------------------

JournalScreen::RibbonPlace JournalScreen::PlaceOf(int ribbon, const Opening& at) const {
	int first = FirstPage(ribbon);
	int before = PagesBefore(at);
	bool left = first <= before;
	int between = left ? before - first : first - before - 2; // under the right page: from the one after it
	return {left, static_cast<float>(between) / static_cast<float>(PagesTotal()) * RIBBON_DEPTH_MAX,
			left && between == 0};
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
	int page = spread[static_cast<size_t>(section)] * 2 + side;
	return page < PageCount(section) ? page : -1;
}

int JournalScreen::RiddleOnPage(int side) const { return section == Section::Riddles ? EntryOnPage(side) : -1; }

bool JournalScreen::CanAnswer(int side) const {
	int index = RiddleOnPage(side);
	return index >= 0 && !Game().journal.Riddles()[static_cast<size_t>(index)].solved;
}

void JournalScreen::ShowToast(const std::string& text) { toast.Show(text, GameClock::now()); }

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
	if (to.section == section && to.spread == spread[static_cast<size_t>(section)])
		return;
	StartTurn(to, false);
}

void JournalScreen::StartTurn(Opening to, bool dragged) {
	Opening from = Open();
	if (pagesFailed) { // nothing to bend: straight there
		section = to.section;
		spread[static_cast<size_t>(section)] = to.spread;
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
	hovered = {};
	Game().assets.sounds.pageTurn.Play();
}

void JournalScreen::FinishTurn() {
	if (turn.target > 0) {
		section = turn.to.section;
		spread[static_cast<size_t>(section)] = turn.to.spread;
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
	turn.angle =
		turn.target > turn.angle ? std::min(turn.target, turn.angle + step) : std::max(turn.target, turn.angle - step);
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
		RibbonPlace place = PlaceOf(i, Open());
		bool underPage = !place.top && (place.left ? x > LEFT_PAGE.x : x < RIGHT_PAGE.x + RIGHT_PAGE.w);
		if (!underPage && ribbonRect(i, place.left, place.depth).contains(x, y))
			return {Target::Ribbon, i};
	}
	if (spread[static_cast<size_t>(section)] > 0 && cornerRect(0).contains(x, y))
		return {Target::PrevPage, 0};
	if (spread[static_cast<size_t>(section)] < SpreadCount(section) - 1 && cornerRect(1).contains(x, y))
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

void JournalScreen::SpecialKeyPressed(int key) {
	auto other = [this](int by) {
		auto s = static_cast<Section>((static_cast<int>(section) + SECTION_COUNT + by) % SECTION_COUNT);
		GoTo({s, spread[static_cast<size_t>(s)]});
	};
	if (key == SPECIAL_MOVE_LEFT)
		Turn(-1);
	else if (key == SPECIAL_MOVE_RIGHT)
		Turn(1);
	else if (key == SPECIAL_MOVE_UP)
		other(-1);
	else if (key == SPECIAL_MOVE_DOWN)
		other(1);
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
	hovered = HitAt(cx, cy);
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
		pressed = hovered;
		bool corner = hovered.target == Target::PrevPage || hovered.target == Target::NextPage;
		if (corner && !pagesFailed) { // grab the page by its corner
			Opening to = Open();
			to.spread += hovered.target == Target::NextPage ? 1 : -1;
			StartTurn(to, true);
			turn.pressX = cx;
			turn.pressY = cy;
			turn.cornerX = RIGHT_PAGE.w;
			pressed = {};
		}
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
	Hit was = pressed;
	pressed = {};
	if (was.target != Target::None && was == hovered) {
		Activate(was);
		hovered = {}; // the riddle screen, or the new page, has its own targets
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
	for (int s = 0; s < SECTION_COUNT; s++)
		spread[static_cast<size_t>(s)] =
			std::clamp(spread[static_cast<size_t>(s)], 0, SpreadCount(static_cast<Section>(s)) - 1);
	AdvanceTurn();

	// The pages a turn shows; a hovered corner lifts a little, showing the page behind.
	const Opening open = Open();
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
	auto slot = [&tex](PageSlot k) -> int& { return tex[static_cast<size_t>(k)]; };
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
			texturedRect(p, Game().assets.textures.journalPaper.ID(), {1, 1, 1}, side == 0 ? 1.f : 0.f, 0.f,
						 side == 0 ? 0.f : 1.f, 1.f);
			DrawPageContent(open, side, true);
		}
	} else {
		glDisable(GL_BLEND);
		texturedRect(LEFT_PAGE, slot(PageSlot::Left), {1, 1, 1});
		texturedRect(RIGHT_PAGE, slot(PageSlot::Right), {1, 1, 1});
	}
	beginShapes();
	line(SPINE, LEFT_PAGE.y, SPINE, LEFT_PAGE.y + LEFT_PAGE.h, BLACK, 0.35f, 1.f); // the gutter
	DrawRibbons(true);															   // under the turning page

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
	RenderTarget& target = pages[static_cast<size_t>(slot)];
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
			if (Riding(i) && PlaceOf(i, slot == PageSlot::Front ? turn.from : turn.to).top == top)
				DrawRibbon(i, side == 0, RidingDepth(i), static_cast<int>(turn.to.section) == i, false);
	};
	riding(false);
	glDisable(GL_BLEND);
	// The paper texture has its spine on the left: the left page draws it mirrored.
	texturedRect(p, Game().assets.textures.journalPaper.ID(), {1, 1, 1}, side == 0 ? 1.f : 0.f, 0.f,
				 side == 0 ? 0.f : 1.f, 1.f);
	DrawPageContent(at, side, live);
	riding(true);
	target.End();
	return target.TextureID();
}

void JournalScreen::DrawPageContent(Opening at, int side, bool live) {
	const Rect& p = pageRect(side);
	float top = p.y + p.h;
	int count = PageCount(at.section);
	int page = at.spread * 2 + side;
	if (page >= count) {
		if (count == 0 && side == 0) {
			beginText();
			textCentered(hand, p.cx(), p.cy(), "Nothing written yet", PENCIL, FADED);
			beginShapes();
		}
		return;
	}

	// The page number stamped in the top outer corner, the level pencilled above it.
	std::string number = std::to_string(page + 1);
	std::string xref;
	if (at.section == Section::Creatures)
		xref = "lvl " + std::to_string(Game().journal.Creatures()[static_cast<size_t>(page)].level);
	else if (at.section == Section::Riddles)
		xref = "lvl " + std::to_string(Game().journal.Riddles()[static_cast<size_t>(page)].level);
	auto outer = [&](const Font& f, const std::string& s) {
		return side == 0 ? p.x + PAGE_PAD : p.x + p.w - PAGE_PAD - f.TextWidth(s.c_str());
	};
	beginText();
	text(stamp, outer(stamp, number), top - NUMBER_TOP, number.c_str(), INK_RED);
	if (!xref.empty())
		text(handTiny, outer(handTiny, xref), top - XREF_TOP, xref.c_str(), PENCIL, FADED);
	beginShapes();

	switch (at.section) {
	case Section::Creatures:
		DrawCreature(p, page);
		break;
	case Section::FieldNotes:
		DrawFieldNote(p, page);
		break;
	case Section::Riddles: {
		Target answer = side == 0 ? Target::AnswerLeft : Target::AnswerRight;
		bool over = live && hovered.target == answer;
		DrawRiddle(p, page, over, over && pressed.target == answer);
		break;
	}
	}
}

void JournalScreen::DrawBook(const Opening& at) {
	beginShapes();
	fillRect({COVER.x + 1.f, COVER.y - 1.2f, COVER.w, COVER.h}, BLACK, BLACK, 0.5f); // shadow
	texturedRect(COVER, Game().assets.textures.journalCloth.ID(), CLOTH, 0.f, 0.f, COVER.w / CLOTH_TILE,
				 COVER.h / CLOTH_TILE);
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
			fillRect(dir < 0 ? Rect{r.x, r.y, 0.12f, r.h} : Rect{r.x + r.w - 0.12f, r.y, 0.12f, r.h}, BLACK, BLACK,
					 0.25f);
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
	if (open || hovered) { // its letter, written on the ribbon end
		beginText();
		float x = left ? r.x + 6.2f - handSmall.TextWidth(look.letter) : r.x + r.w - 6.2f;
		text(handSmall, x, r.y + 1.7f, look.letter, PAPER, 0.9f);
		beginShapes();
	}
}

void JournalScreen::DrawRibbons(bool top) {
	beginShapes();
	Section shown = turn.active ? turn.to.section : section; // a turn to another section shows its letter at once
	for (int i = 0; i < SECTION_COUNT; i++) {
		if (Riding(i))
			continue; // drawn on the turning page
		// Still, but the pages move under it: from where it lay to where it lands.
		RibbonPlace place = PlaceOf(i, turn.active ? turn.from : Open());
		if (turn.active) {
			RibbonPlace to = PlaceOf(i, turn.to);
			place.depth += (to.depth - place.depth) * turn.progress;
			place.top = turn.progress < 0.5f ? place.top : to.top;
		}
		if (place.top != top)
			continue;
		bool isHovered = hovered.target == Target::Ribbon && hovered.ribbon == i;
		DrawRibbon(i, place.left, place.depth, static_cast<int>(shown) == i, isHovered);
	}
	// The hovered ribbon's name, on a small dark label next to it on the page.
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
	text(small, label.x + 1.5f, label.y + 1.2f, name, GOLD);
	beginShapes();
}

void JournalScreen::DrawRiddle(const Rect& p, int index, bool answerHovered, bool answerHeld) {
	const JournalRiddle& r = Game().journal.Riddles()[static_cast<size_t>(index)];
	float top = p.y + p.h;
	float cx = p.cx();
	float width = p.w - 2 * PAGE_PAD;

	underlinedHeading(handHeading, cx, top - HEADING_TOP, r.theme.c_str());
	beginText();
	std::vector<std::string> lines;
	for (const std::string& q : r.question)
		for (const std::string& l : wrap(hand, q, width))
			lines.push_back(l);
	if (lines.size() > MAX_QUESTION_LINES)
		lines.resize(MAX_QUESTION_LINES);
	float y = top - QUESTION_TOP;
	for (const std::string& l : lines) {
		textCentered(hand, cx, y, l.c_str(), INK_BLUE);
		y -= QUESTION_STEP;
	}

	if (r.hintShown) {
		std::string hint = r.hint;
		if (hint.empty())
			hint = "the answer has " + std::to_string(r.answers.front().size()) + " characters";
		std::vector<std::string> hintLines = wrap(handSmall, "Hint: " + hint, width);
		y -= 1.f;
		for (size_t i = 0; i < hintLines.size() && i < MAX_HINT_LINES; i++) {
			textCentered(handSmall, cx, y, hintLines[i].c_str(), PENCIL, FADED);
			y -= 3.4f;
		}
	}

	if (r.solved) {
		std::string answer = "Answer: " + r.written;
		float w = hand.TextWidth(answer.c_str());
		text(hand, cx - w / 2 + 2.f, p.y + 13.f, answer.c_str(), INK_BLUE);
		if (r.solvedLate)
			textCentered(handSmall, cx, p.y + 8.5f, "answered later", PENCIL, FADED);
		beginShapes();
		tick(cx - w / 2 - 2.5f, p.y + 13.f, INK_RED);
	} else {
		textCentered(hand, cx, p.y + 19.f, "Answer: ?", PENCIL);
		beginShapes();
		Rect b = tile(answerRect(p.x < RIGHT_PAGE.x ? 0 : 1), TileStyle::Lapis, answerHovered, answerHeld);
		std::string label = "Answer  +" + std::to_string(r.lateXP) + " XP";
		beginText();
		textCentered(body, b.cx(), b.y + 2.1f, label.c_str(), answerHovered ? TEXT_HOVER : GOLD);
		beginShapes();
	}
}

void JournalScreen::DrawCreature(const Rect& p, int index) {
	const JournalCreature& c = Game().journal.Creatures()[static_cast<size_t>(index)];
	const MonsterType& t = Game().assets.monsterTypes[static_cast<size_t>(c.type)];
	const MonsterKind* kind = monsterKind(c.type);
	float top = p.y + p.h;
	float cx = p.cx();

	underlinedHeading(handHeading, cx, top - HEADING_TOP, c.killed ? t.name : "?");

	// A photograph once it was killed (seen up close), with its catalogue number and a caption; before that a
	// pencil sketch from afar. Each photo is pasted in a little askew.
	Rect box = photoBox(p);
	float tilt = c.killed ? static_cast<float>((c.type * 5 + index * 3) % 7 - 3) * 0.8f : 0.f;
	DrawSketch(c.type, box, c.killed, tilt);
	beginText();
	if (c.killed) {
		char id[16];
		std::snprintf(id, sizeof(id), "L%02d-%02d", c.level, index + 1);
		text(stampSmall, box.x + 0.5f, box.y + box.h + 1.f, id, INK_RED);
		const char* caption = "from the side";
		float w = handSmall.TextWidth(caption);
		text(handSmall, cx - w / 2 + 1.5f, top - CAPTION_TOP, caption, INK_BLUE);
		beginShapes();
		upArrow(cx - w / 2 - 1.f, top - CAPTION_TOP - 0.3f, INK_BLUE);
	} else {
		textCentered(handSmall, cx, top - CAPTION_TOP, "sketched from afar", PENCIL, FADED);
		beginShapes();
	}

	// The notes: a label in the margin column, the text beside it. Blue ink for what was seen, blanks for what is
	// still unknown.
	float x = p.x + PAGE_PAD;
	float textX = x + MARGIN_W;
	float textW = p.x + p.w - PAGE_PAD - textX;
	float y = top - ROWS_TOP;
	auto label = [&](const char* s, Color ink) {
		beginText();
		text(handTiny, x, y, s, ink);
		beginShapes();
	};
	auto entry = [&](const char* name, const std::string& s) {
		label(name, PENCIL);
		beginText();
		for (const std::string& l : wrap(handSmall, s, textW)) {
			text(handSmall, textX, y, l.c_str(), INK_BLUE);
			y -= ROW_STEP;
		}
		beginShapes();
	};
	std::string seen = "Level " + std::to_string(c.level) + ".";
	if (c.killed && kind != nullptr)
		seen += std::string(" ") + kind->note;
	entry("SEEN", seen);
	std::string saw = c.hitBy ? attackSentence(t.attackMix) : ""; // what its bite deals, from the first one
	for (int m = 0; m < CREATURE_MOVE_COUNT; m++)
		if (c.Saw(static_cast<CreatureMove>(m)))
			saw += std::string(saw.empty() ? "" : " ") + moveNote(static_cast<CreatureMove>(m), t);
	if (!saw.empty())
		entry("SAW", saw);

	// The form, filled in as it is learnt.
	if (c.killed)
		label("KILLED", INK_RED);
	formField(handSmall, textX, y, "HP", c.killed ? "~" + std::to_string(rough(t.maxHealth)) : "");
	formField(handSmall, textX + FORM_COLUMN, y, "HITS", c.hitBy ? "~" + std::to_string(rough(t.damage)) : "");
	y -= ROW_STEP;
	if (c.tried != 0)
		label("TRIED", PENCIL);
	for (int d = 0; d < DAMAGE_TYPE_COUNT; d++) {
		const auto type = static_cast<DamageType>(d);
		std::string value = c.Tried(type) ? resistanceWord(t.resist[static_cast<size_t>(d)]) : "";
		formField(handSmall, textX + static_cast<float>(d % 2) * FORM_COLUMN, y,
				  upper(DAMAGE_TYPE_NAMES[static_cast<size_t>(d)]), value);
		if (d % 2 == 1)
			y -= ROW_STEP;
	}
}

void JournalScreen::DrawFieldNote(const Rect& p, int index) {
	const FieldNoteText& note = FIELD_NOTES[static_cast<size_t>(Game().journal.Notes()[static_cast<size_t>(index)])];
	float top = p.y + p.h;
	underlinedHeading(handHeading, p.cx(), top - HEADING_TOP, note.title);
	beginText();
	float y = top - FIELD_NOTE_TOP;
	for (const std::string& l : wrap(hand, note.text, p.w - 2 * PAGE_PAD)) {
		text(hand, p.x + PAGE_PAD, y, l.c_str(), INK_BLUE);
		y -= FIELD_NOTE_STEP;
	}
	beginShapes();
}

// A creature's model, frame 0 of its move clip, from the side and a little above. As a pencil sketch: the outline
// only (a depth pass first, so only the front surfaces get lines). As a photo: a black and white print, lit, on a
// white-bordered card tilted by `tilt` degrees.
void JournalScreen::DrawSketch(int type, const Rect& box, bool photo, float tilt) {
	const MonsterType& t = Game().assets.monsterTypes[static_cast<size_t>(type)];
	const CharacterModel& model = t.model;
	constexpr ModelState POSE =
		ModelState::Move; // the mimic's idle clip is its disguise, the bat's hangs it upside down
	const AnimatedModel* clip = model.Clip(model.Shown(POSE));
	if (clip == nullptr)
		return;
	Texture& print = photoTextures[static_cast<size_t>(type)];
	if (photo && !print.ID()) {
		std::string file = std::string("textures/") + t.texture + ".png";
		print.LoadPNG(file.c_str(), TexFilter::Mipmapped, photoGrey);
	}

	// The sketches change depth func, culling, polygon mode, colour mask, line width, lighting and blending: all of
	// it goes back as it was, the rest of the game relies on it (GL_LEQUAL depth, game.cpp).
	glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT |
				 GL_CURRENT_BIT | GL_LIGHTING_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	glPushMatrix();
	Rect fit = box;
	if (photo) {
		// Light from the upper left, in front: set while the modelview is the page's own.
		const float lightDir[4] = {-0.5f, 0.7f, 1.f, 0.f};
		const float ambient[4] = {0.5f, 0.5f, 0.5f, 1.f};
		const float diffuse[4] = {0.85f, 0.85f, 0.85f, 1.f};
		const float none[4] = {0.f, 0.f, 0.f, 1.f};
		glLightfv(GL_LIGHT0, GL_POSITION, lightDir);
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
		glLightfv(GL_LIGHT0, GL_SPECULAR, none);
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, none);

		glTranslatef(box.cx(), box.cy(), 0);
		glRotatef(tilt, 0, 0, 1);
		glTranslatef(-box.cx(), -box.cy(), 0);
		beginShapes();
		fillRect({box.x + 0.35f, box.y - 0.45f, box.w, box.h}, BLACK, BLACK, 0.28f); // pasted on: a thin shadow
		fillRect(box, {1, 1, 1}, darker(PAPER, 0.97f), 1.f);
		Rect image = box.inset(PHOTO_BORDER);
		fillRect(image, darker(PAPER, 0.24f), darker(PAPER, 0.46f), 1.f); // a dark room, the lit floor below
		ring(image.inset(3.f), 3.f, BLACK, 0.f, 0.35f);
		fit = image.inset(0.6f);
	}

	auto [bottom, top] = clip->YRange(0);
	auto [halfX, halfZ] = clip->HalfXZ(0);
	// Turned any way, the body stays within its longest half width; the tilt adds some of its depth to the height.
	const float reach = 2.f * std::max(halfX, halfZ);
	const float lean = SKETCH_TILT * static_cast<float>(M_PI) / 180.f;
	const float tall = (top - bottom) * std::cos(lean) + reach * std::sin(lean);
	float size = std::min(fit.w / reach, fit.h / tall);
	const AnimPlayback frame0{};

	glTranslatef(fit.cx(), fit.cy() - (top + bottom) / 2 * size * std::cos(lean), 0);
	glRotatef(SKETCH_TILT, 1, 0, 0);
	glScalef(size, size, size);
	glRotatef(t.rotA + 90.f + SKETCH_YAW, 0, 1, 0);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	if (photo) {
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glEnable(GL_NORMALIZE);
		glEnable(GL_COLOR_MATERIAL);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor4f(1, 1, 1, 1);
		clip->Show(frame0, print.ID());
	} else {
		glDepthFunc(GL_LESS);
		glDisable(GL_TEXTURE_2D);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		clip->Show(frame0);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthFunc(GL_LEQUAL);
		// The outline: the back faces' edges show where the body ends (and at its folds).
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		glPolygonMode(GL_BACK, GL_LINE);
		glLineWidth(SKETCH_LINE_PX * static_cast<float>(Game().render.resY) / 720.f);
		glColor4f(PENCIL.r, PENCIL.g, PENCIL.b, 0.9f);
		clip->Show(frame0);
	}
	glPopMatrix();
	glPopAttrib();
	beginShapes();
}

void JournalScreen::DrawCorners() {
	if (turn.active)
		return;
	beginShapes();
	int open = spread[static_cast<size_t>(section)];
	if (open > 0)
		cornerArrow(cornerRect(0), true, hovered.target == Target::PrevPage ? INK_RED : PENCIL);
	if (open < SpreadCount(section) - 1)
		cornerArrow(cornerRect(1), false, hovered.target == Target::NextPage ? INK_RED : PENCIL);
}

void JournalScreen::DrawFooter() {
	beginText();
	if (float alpha = toast.Alpha(GameClock::now()); alpha > 0.f)
		textCentered(body, CANVAS_W / 2, 6.2f, toast.text.c_str(), {1.f, 0.9f, 0.6f}, alpha);
	textCentered(small, CANVAS_W / 2, 2.2f,
				 "Ribbons, Up / Down: section    Corners (click or drag), Left / Right, wheel: turn the page    "
				 "J / Esc: close",
				 {0.55f, 0.45f, 0.30f});
	beginShapes();
}
