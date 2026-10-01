#include "journal_view.h"
#include "../graphics/gl_includes.h"
#include "../input/input.h"
#include "../state/game_state.h"
#include "../world/journal.h"
#include "../world/monster_kinds.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <vector>

using namespace ui;

namespace {

// ---- layout ----------------------------------------------------------------

constexpr Rect COVER = {9, 10, 138, 75};
constexpr Rect LEFT_PAGE = {12, 12.5f, 66, 70};
constexpr Rect RIGHT_PAGE = {78, 12.5f, 66, 70};
constexpr float PAGE_PAD = 6.f;
constexpr float SPINE_SHADE = 4.f; // width of the shadow either side of the spine

constexpr float RIBBON_X = 141.f; // the ribbons lie over the page edge, then hang out of the book
constexpr float RIBBON_LEN = 14.f;
constexpr float RIBBON_OPEN_LEN = 16.f; // the open section's ribbon sticks out further
constexpr float RIBBON_H = 6.f;
constexpr float RIBBON_TOP = 76.f;
constexpr float RIBBON_STEP = 9.f;
constexpr float RIBBON_NOTCH = 2.f;

constexpr float CORNER = 7.f; // page turn corners
constexpr float ANSWER_W = 36.f;
constexpr float ANSWER_H = 7.f;

constexpr const char* HAND_FONT = "fonts/kalam.png"; // Kalam (OFL, fonts/kalam-OFL.txt)
constexpr float FADED = 0.7f;						 // alpha of pencil notes that matter less (level, hint)

constexpr float SKETCH_TOP = 21.f; // below the page top
constexpr float SKETCH_H = 20.f;
constexpr float SKETCH_PAD_X = 9.f;
constexpr float SKETCH_YAW = -25.f;	   // from the side, turned a little towards the reader
constexpr float SKETCH_TILT = 12.f;	   // seen a little from above
constexpr float SKETCH_WASH = 0.75f;   // how dark the texture's darkest part shades the paper
constexpr float SKETCH_LINE_PX = 1.6f; // at 720 rows
constexpr float NOTES_TOP = 45.5f;	   // first note's baseline below the page top
constexpr float NOTE_STEP = 3.5f;
constexpr size_t MAX_NOTE_LINES = 2; // of the description

constexpr float QUESTION_STEP = 4.4f;
constexpr size_t MAX_QUESTION_LINES = 7;
constexpr size_t MAX_HINT_LINES = 2;

struct SectionLook {
	const char* name;
	Color colour;
};
constexpr std::array<SectionLook, JournalScreen::SECTION_COUNT> SECTIONS = {{
	{"Creatures", INK_RED},
	{"Riddles", LAPIS},
	{"Field notes", GOLD_DIM},
}};

Rect visibleArea() { return ui::visibleArea(CANVAS_W, CANVAS_H, Game().render.resX, Game().render.resY); }

const Rect& pageRect(int side) { return side == 0 ? LEFT_PAGE : RIGHT_PAGE; }

Rect ribbonRect(int index, bool open) {
	return {RIBBON_X, RIBBON_TOP - static_cast<float>(index) * RIBBON_STEP, open ? RIBBON_OPEN_LEN : RIBBON_LEN,
			RIBBON_H};
}

Rect cornerRect(int side) {
	const Rect& p = pageRect(side);
	return side == 0 ? Rect{p.x + 1, p.y + 1, CORNER, CORNER} : Rect{p.x + p.w - 1 - CORNER, p.y + 1, CORNER, CORNER};
}

Rect answerRect(int side) {
	const Rect& p = pageRect(side);
	return {p.cx() - ANSWER_W / 2, p.y + 9.f, ANSWER_W, ANSWER_H};
}

Color darker(Color c, float f) { return {c.r * f, c.g * f, c.b * f}; }

// A ribbon with a swallowtail end.
void ribbon(const Rect& r, Color c) {
	float tail = r.x + r.w - RIBBON_NOTCH;
	fillRect({r.x + 0.4f, r.y - 0.6f, r.w - RIBBON_NOTCH, r.h}, BLACK, BLACK, 0.35f); // shadow
	fillRect({r.x, r.y, tail - r.x, r.h}, c, darker(c, 0.7f), 1.f);
	triangle(tail, r.y + r.h, r.x + r.w, r.y + r.h, tail, r.y + r.h / 2, c, 1.f);
	triangle(tail, r.y, r.x + r.w, r.y, tail, r.y + r.h / 2, darker(c, 0.7f), 1.f);
	fillRect({r.x, r.y + r.h - 0.6f, tail - r.x, 0.6f}, {1, 1, 1}, {1, 1, 1}, 0.12f); // sheen along the top
}

// Page turn arrow in a page corner; `left` points to the previous spread.
void cornerArrow(const Rect& r, bool left, Color c) {
	float cy = r.cy();
	float tip = left ? r.x + 1.f : r.x + r.w - 1.f;
	float base = left ? r.x + r.w - 2.f : r.x + 2.f;
	triangle(tip, cy, base, cy + 2.f, base, cy - 2.f, c, 1.f);
}

// Monster textures as a pencil wash: the darker a texel, the more pencil grey it lays on the paper (drawn with a
// multiply blend, white leaves the paper as it is).
void pencilWash(unsigned char* data, int pixels, int components) {
	for (int i = 0; i < pixels; i++) {
		unsigned char* p = data + static_cast<ptrdiff_t>(i) * components;
		auto channel = [p](int c) { return static_cast<float>(p[c]); };
		float lum = (0.3f * channel(0) + 0.59f * channel(1) + 0.11f * channel(2)) / 255.f;
		lum = std::clamp((lum - 0.5f) * 1.3f + 0.6f, 0.f, 1.f); // more contrast, a lighter middle
		float ink = SKETCH_WASH * (1.f - lum);
		const float pencil[3] = {PENCIL.r, PENCIL.g, PENCIL.b};
		for (int c = 0; c < 3; c++)
			p[c] = static_cast<unsigned char>(255.f * (1.f - ink * (1.f - pencil[c])));
	}
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
	}
	return "";
}

// A tick before a solved riddle's answer, from its bottom left at (x, y).
void tick(float x, float y, Color c) {
	line(x, y + 1.4f, x + 1.f, y, c, 1.f, 2.f);
	line(x + 1.f, y, x + 3.f, y + 3.f, c, 1.f, 2.f);
}

} // namespace

// ---- pages -----------------------------------------------------------------

int JournalScreen::PageCount(Section s) const {
	if (s == Section::Riddles)
		return static_cast<int>(Game().journal.Riddles().size());
	if (s == Section::Creatures)
		return static_cast<int>(Game().journal.Creatures().size());
	return 0; // field notes come in a later stage
}

int JournalScreen::SpreadCount(Section s) const { return std::max(1, (PageCount(s) + 1) / 2); }

void JournalScreen::Turn(int by) {
	int& open = spread[static_cast<size_t>(section)];
	open = std::clamp(open + by, 0, SpreadCount(section) - 1);
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

// ---- input -----------------------------------------------------------------

JournalScreen::Hit JournalScreen::HitAt(float x, float y) const {
	for (int i = 0; i < SECTION_COUNT; i++)
		if (ribbonRect(i, static_cast<int>(section) == i).contains(x, y))
			return {Target::Ribbon, i};
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
		section = static_cast<Section>(hit.ribbon);
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
	if (key == SPECIAL_MOVE_LEFT)
		Turn(-1);
	else if (key == SPECIAL_MOVE_RIGHT)
		Turn(1);
	else if (key == SPECIAL_MOVE_UP)
		section = static_cast<Section>((static_cast<int>(section) + SECTION_COUNT - 1) % SECTION_COUNT);
	else if (key == SPECIAL_MOVE_DOWN)
		section = static_cast<Section>((static_cast<int>(section) + 1) % SECTION_COUNT);
}

void JournalScreen::MouseMotion(int x, int y) {
	float cx = 0.f;
	float cy = 0.f;
	toCanvas(visibleArea(), Game().render.resX, Game().render.resY, x, y, cx, cy);
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
	if (state == GLUT_DOWN) {
		pressed = hovered;
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
		fontsLoaded = true;
	}
	spread[static_cast<size_t>(section)] =
		std::clamp(spread[static_cast<size_t>(section)], 0, SpreadCount(section) - 1);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	Rect area = visibleArea();
	glOrtho(area.x, area.x + area.w, area.y, area.y + area.h, -200, 200);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);

	backdrop(area, Game().assets.textures.loadingBackground.ID());
	titleBar(title, CANVAS_W / 2, "Journal", 58.f);
	DrawBook();
	DrawPage(0);
	DrawPage(1);
	DrawSketches();
	DrawCorners();
	DrawRibbons();
	DrawFooter();

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glColor3f(1, 1, 1);
}

void JournalScreen::DrawBook() {
	beginShapes();
	fillRect({COVER.x + 1.f, COVER.y - 1.2f, COVER.w, COVER.h}, BLACK, BLACK, 0.5f); // shadow
	fillRect(COVER, BRONZE, STONE_BOTTOM, 1.f);
	strokeRect(COVER, STONE_BOTTOM, 1.f, 3.f);
	strokeRect(COVER.inset(1.f), GOLD_DIM, 0.6f, 1.f);

	// The page block under the open pages: a few page edges showing at the bottom.
	for (int k = 3; k >= 1; k--) {
		auto d = static_cast<float>(k) * 0.35f;
		fillRect({LEFT_PAGE.x - d, LEFT_PAGE.y - d, LEFT_PAGE.w + d, LEFT_PAGE.h}, INK_FADED, INK_FADED, 0.9f);
		fillRect({RIGHT_PAGE.x, RIGHT_PAGE.y - d, RIGHT_PAGE.w + d, RIGHT_PAGE.h}, INK_FADED, INK_FADED, 0.9f);
	}
	const auto papyrus = Game().assets.textures.papyrus.ID();
	texturedRect(LEFT_PAGE, papyrus, {1, 1, 1}, 0.06f, 0.09f, 0.5f, 0.93f);
	texturedRect(RIGHT_PAGE, papyrus, {1, 1, 1}, 0.5f, 0.09f, 0.94f, 0.93f);
	beginShapes();

	// The pages curve down into the spine.
	constexpr int STRIPS = 8;
	float spine = RIGHT_PAGE.x;
	for (int k = 0; k < STRIPS; k++) {
		float w = SPINE_SHADE / STRIPS;
		float alpha = 0.28f * (1.f - static_cast<float>(k) / STRIPS);
		float off = static_cast<float>(k) * w;
		fillRect({spine - off - w, LEFT_PAGE.y, w, LEFT_PAGE.h}, BLACK, BLACK, alpha);
		fillRect({spine + off, RIGHT_PAGE.y, w, RIGHT_PAGE.h}, BLACK, BLACK, alpha);
	}
	line(spine, LEFT_PAGE.y, spine, LEFT_PAGE.y + LEFT_PAGE.h, INK, 0.6f, 1.f);
}

void JournalScreen::DrawRibbons() {
	beginShapes();
	for (int i = 0; i < SECTION_COUNT; i++) {
		bool open = static_cast<int>(section) == i;
		Rect r = ribbonRect(i, open);
		Color c = SECTIONS[static_cast<size_t>(i)].colour;
		bool isHovered = hovered.target == Target::Ribbon && hovered.ribbon == i;
		ribbon(r, isHovered ? darker(c, 1.25f) : c);
		if (open)
			fillRect({r.x, r.y, 1.f, r.h}, BLACK, BLACK, 0.25f); // tucked in between the open pages
	}
	// The hovered ribbon's name, on a small dark label next to it on the page.
	if (hovered.target != Target::Ribbon)
		return;
	const char* name = SECTIONS[static_cast<size_t>(hovered.ribbon)].name;
	Rect r = ribbonRect(hovered.ribbon, static_cast<int>(section) == hovered.ribbon);
	float w = small.TextWidth(name) + 3.f;
	Rect label = {r.x - w - 1.f, r.y + 0.8f, w, 4.4f};
	fillRect(label, PANEL_TOP, PANEL_BOTTOM, 0.95f);
	strokeRect(label, GOLD_DIM, 1.f, 1.f);
	beginText();
	text(small, label.x + 1.5f, label.y + 1.2f, name, GOLD);
	beginShapes();
}

void JournalScreen::DrawPage(int side) {
	const Rect& p = pageRect(side);
	float top = p.y + p.h;
	const char* name = SECTIONS[static_cast<size_t>(section)].name;

	// Running head and page number, like a printed notebook.
	beginText();
	if (side == 0)
		text(small, p.x + PAGE_PAD, top - 5.f, name, INK_FADED);
	else
		text(small, p.x + p.w - PAGE_PAD - small.TextWidth(name), top - 5.f, name, INK_FADED);
	int page = spread[static_cast<size_t>(section)] * 2 + side;
	if (page < PageCount(section)) {
		std::string number = std::to_string(page + 1);
		textCentered(small, p.cx(), p.y + 3.f, number.c_str(), INK_FADED);
	}

	if (PageCount(section) == 0 && side == 0)
		textCentered(hand, p.cx(), p.cy(), "Nothing written yet", PENCIL, FADED);
	beginShapes();

	if (section == Section::Creatures && EntryOnPage(side) >= 0)
		DrawCreature(p, EntryOnPage(side));
	int riddle = RiddleOnPage(side);
	if (riddle >= 0) {
		Target answer = side == 0 ? Target::AnswerLeft : Target::AnswerRight;
		DrawRiddle(p, riddle, hovered.target == answer, hovered.target == answer && pressed.target == answer);
	}
}

void JournalScreen::DrawRiddle(const Rect& p, int index, bool answerHovered, bool answerHeld) {
	const JournalRiddle& r = Game().journal.Riddles()[static_cast<size_t>(index)];
	float top = p.y + p.h;
	float cx = p.cx();
	float width = p.w - 2 * PAGE_PAD;

	beginText();
	textCentered(handHeading, cx, top - 13.f, r.theme.c_str(), PENCIL);
	std::string found = "Level " + std::to_string(r.level);
	textCentered(handSmall, cx, top - 17.5f, found.c_str(), PENCIL, FADED);

	std::vector<std::string> lines;
	for (const std::string& q : r.question)
		for (const std::string& l : wrap(hand, q, width))
			lines.push_back(l);
	if (lines.size() > MAX_QUESTION_LINES)
		lines.resize(MAX_QUESTION_LINES);
	float y = top - 25.f;
	for (const std::string& l : lines) {
		textCentered(hand, cx, y, l.c_str(), PENCIL);
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
		text(hand, cx - w / 2 + 2.f, p.y + 13.f, answer.c_str(), INK_GREEN);
		if (r.solvedLate)
			textCentered(handSmall, cx, p.y + 8.5f, "answered later", PENCIL, FADED);
		beginShapes();
		tick(cx - w / 2 - 2.5f, p.y + 13.f, INK_GREEN);
	} else {
		textCentered(hand, cx, p.y + 19.f, "Answer: ?", PENCIL);
		beginShapes();
	}

	line(p.x + PAGE_PAD + 4, top - 20.f, p.x + p.w - PAGE_PAD - 4, top - 20.f, INK_FADED, 0.8f, 1.f);
	diamond(cx, top - 20.f, 0.6f, INK_RED, 1.f);

	if (!r.solved) {
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
	float width = p.w - 2 * PAGE_PAD;

	beginText();
	textCentered(handHeading, cx, top - 13.f, c.killed ? t.name : "?", PENCIL);
	std::string found = "Level " + std::to_string(c.level);
	textCentered(handSmall, cx, top - 17.5f, found.c_str(), PENCIL, FADED);

	// Notes under the sketch: what it is (after a kill), the numbers, the moves seen; "?" for what is still unknown.
	std::vector<std::pair<std::string, float>> notes; // text, alpha
	if (c.killed && kind != nullptr)
		for (const std::string& l : wrap(handSmall, kind->note, width))
			if (notes.size() < MAX_NOTE_LINES)
				notes.emplace_back(l, 1.f);
	std::string hp = c.killed ? "~" + std::to_string(rough(t.maxHealth)) + " HP" : "HP ?";
	std::string hit = c.hitBy ? "hits for ~" + std::to_string(rough(t.damage)) : "its hit ?";
	notes.emplace_back(hp + "     " + hit, 1.f);
	for (int m = 0; m < CREATURE_MOVE_COUNT; m++)
		if (c.Saw(static_cast<CreatureMove>(m)))
			notes.emplace_back(moveNote(static_cast<CreatureMove>(m), t), 1.f);
	float y = top - NOTES_TOP;
	for (const auto& [line, alpha] : notes) {
		textCentered(handSmall, cx, y, line.c_str(), PENCIL, alpha);
		y -= NOTE_STEP;
	}
	beginShapes();
	line(p.x + PAGE_PAD + 4, top - 20.f, p.x + p.w - PAGE_PAD - 4, top - 20.f, INK_FADED, 0.8f, 1.f);
	diamond(cx, top - 20.f, 0.6f, INK_RED, 1.f);
}

// The model of each creature on the open pages, drawn flat in pencil: an outline, and once it was killed (seen up
// close) a wash of its texture. A depth pass first, so only the front surfaces get lines and wash.
void JournalScreen::DrawSketches() {
	if (section != Section::Creatures)
		return;
	glClear(GL_DEPTH_BUFFER_BIT);
	for (int side = 0; side < 2; side++) {
		int index = EntryOnPage(side);
		if (index < 0)
			continue;
		const JournalCreature& c = Game().journal.Creatures()[static_cast<size_t>(index)];
		const Rect& p = pageRect(side);
		DrawSketch(c.type, {p.x + SKETCH_PAD_X, p.y + p.h - SKETCH_TOP - SKETCH_H, p.w - 2 * SKETCH_PAD_X, SKETCH_H},
				   c.killed);
	}
	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glLineWidth(1);
	beginShapes();
}

void JournalScreen::DrawSketch(int type, const Rect& box, bool washed) {
	const MonsterType& t = Game().assets.monsterTypes[static_cast<size_t>(type)];
	const CharacterModel& model = t.model;
	constexpr ModelState POSE =
		ModelState::Move; // the mimic's idle clip is its disguise, the bat's hangs it upside down
	const AnimatedModel* clip = model.Clip(model.Shown(POSE));
	if (clip == nullptr)
		return;
	if (washed && !sketchTextures[static_cast<size_t>(type)].ID()) {
		std::string file = std::string("textures/") + t.texture + ".png";
		sketchTextures[static_cast<size_t>(type)].LoadPNG(file.c_str(), TexFilter::Mipmapped, pencilWash);
	}

	auto [bottom, top] = clip->YRange(0);
	auto [halfX, halfZ] = clip->HalfXZ(0);
	// Turned any way, the body stays within its longest half width; the tilt adds some of its depth to the height.
	const float reach = 2.f * std::max(halfX, halfZ);
	const float tilt = SKETCH_TILT * static_cast<float>(M_PI) / 180.f;
	const float tall = (top - bottom) * std::cos(tilt) + reach * std::sin(tilt);
	float size = std::min(box.w / reach, box.h / tall);
	const AnimPlayback frame0{};

	glPushMatrix();
	glTranslatef(box.cx(), box.cy() - (top + bottom) / 2 * size * std::cos(tilt), 0);
	glRotatef(SKETCH_TILT, 1, 0, 0);
	glScalef(size, size, size);
	glRotatef(t.rotA + 90.f + SKETCH_YAW, 0, 1, 0);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	clip->Show(frame0);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthFunc(GL_LEQUAL);

	if (washed) {
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_DST_COLOR, GL_ZERO); // multiply onto the paper
		glColor4f(1, 1, 1, 1);
		clip->Show(frame0, sketchTextures[static_cast<size_t>(type)].ID());
		glDisable(GL_TEXTURE_2D);
	}

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
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glPopMatrix();
}

void JournalScreen::DrawCorners() {
	beginShapes();
	int open = spread[static_cast<size_t>(section)];
	if (open > 0)
		cornerArrow(cornerRect(0), true, hovered.target == Target::PrevPage ? INK_RED : INK);
	if (open < SpreadCount(section) - 1)
		cornerArrow(cornerRect(1), false, hovered.target == Target::NextPage ? INK_RED : INK);
}

void JournalScreen::DrawFooter() {
	beginText();
	if (float alpha = toast.Alpha(GameClock::now()); alpha > 0.f)
		textCentered(body, CANVAS_W / 2, 6.2f, toast.text.c_str(), {1.f, 0.9f, 0.6f}, alpha);
	textCentered(small, CANVAS_W / 2, 2.2f,
				 "Ribbons, Up / Down: section    Corners, Left / Right, wheel: turn the page    J / Esc: close",
				 {0.55f, 0.45f, 0.30f});
	beginShapes();
}
