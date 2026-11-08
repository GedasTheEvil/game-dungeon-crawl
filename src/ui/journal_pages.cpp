// Each section's page drawing of the journal; the book, ribbons, page turns and input are in journal_view.cpp.
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
#include "../core/paths.h"

using namespace ui;
using namespace journal_view;

namespace {

// ---- layout ----------------------------------------------------------------

constexpr float PAGE_PAD = 6.f;
constexpr float FADED = 0.7f; // alpha of pencil notes that matter less
constexpr float NEW_IN = 4.5f; // a new entry's mark: in from the spine, below the page top, its dot's radius
constexpr float NEW_TOP = 4.f;
constexpr float NEW_DOT = 1.1f;

constexpr float NUMBER_TOP = 5.8f; // page number baseline below the page top
constexpr float XREF_TOP = 2.9f; // the pencilled level above it
constexpr float HEADING_TOP = 11.5f;
constexpr float PHOTO_TOP = 16.f;
constexpr float PHOTO_W = 40.f;
constexpr float PHOTO_H = 17.f;
constexpr float PHOTO_BORDER = 1.1f;
constexpr float CAPTION_TOP = 36.6f;
constexpr float ROWS_TOP = 40.8f; // first note's baseline below the page top
constexpr float ROW_STEP = 3.2f;
constexpr float CREATURE_NOTE_ROOM = 6 * ROW_STEP; // a creature's SEEN lines, above its form
constexpr float TINY_ROW_STEP = 2.7f; // the notes in the tiny hand, when they would not fit the room
constexpr float MARGIN_W = 10.f; // the label column on the left of the notes
constexpr float FORM_COLUMN = 22.f;
constexpr float BLANK_W = 7.f; // the line of a field not filled in yet

constexpr float SKETCH_YAW = -25.f; // from the side, turned a little towards the reader
constexpr float SKETCH_TILT = 12.f; // seen a little from above
constexpr float SKETCH_LINE_PX = 1.6f; // at 720 rows

constexpr float QUESTION_TOP = 19.f;
constexpr float QUESTION_STEP = 4.4f;
constexpr float FIELD_NOTE_TOP = 19.f;
constexpr float FIELD_NOTE_STEP = 4.6f;
constexpr size_t MAX_QUESTION_LINES = 7;
constexpr size_t MAX_HINT_LINES = 2;
constexpr float GLYPHS_TOP = 14.f; // the copied hieroglyphs: their box's top below the page top, and its side
constexpr float GLYPHS_SIDE = 17.f;
constexpr float TRANSLATION_TOP = 35.5f;
constexpr float TRANSLATION_STEP = 4.2f;
constexpr float TRANSLATION_SMALL_STEP = 3.5f;
constexpr size_t MAX_TRANSLATION_LINES = 6; // in the hand font; a longer one is written smaller
constexpr size_t MAX_TRANSLATION_SMALL_LINES = 7;
constexpr float COMMENT_BOTTOM = 8.5f; // the pencilled comment's first baseline above the page bottom
constexpr float COMMENT_STEP = 3.2f;
constexpr size_t MAX_COMMENT_LINES = 2;
constexpr float DIARY_TOP = 18.f; // the first line's baseline below the page top
constexpr float DIARY_STEP = 4.f;
constexpr float DIARY_SMALL_STEP = 3.4f;
constexpr float DIARY_HEIGHT = 46.f; // room for the lines, down to above the close hint
constexpr float LOG_TOP = 19.f; // the log's first baseline below the page top
constexpr float LOG_STEP = 3.4f;
constexpr float LOG_GAP = 1.4f; // between two status lines
constexpr float LOG_BOTTOM = 6.f; // the last baseline above the page bottom
// The expedition's torn page pasted in (media/textures/ui/expedition_page.png, tools/textures/expedition_page.py):
// its height on the page, its shape, and where its rules and margin line lie, as fractions of the image.
constexpr float TORN_H = 60.f;
constexpr float TORN_ASPECT = 256.f / 320.f;
constexpr float TORN_BOTTOM = 3.f; // above the page's bottom edge
constexpr float TORN_RULE_TOP = 46.f / 320.f;
constexpr float TORN_RULE_STEP = 22.f / 320.f;
constexpr float TORN_MARGIN = 52.f / 256.f;
constexpr float TORN_RIGHT = 14.f / 256.f; // the writing stops short of the right edge
constexpr float TORN_BASELINE = 1.6f; // the font's baseline lies this far above the bottom of its line
constexpr float GIFT_BOTTOM = COMMENT_BOTTOM - COMMENT_STEP; // a translated boon's line, under the comment

constexpr Color SEPIA = {0.36f, 0.20f, 0.09f}; // the earlier expedition's ink: someone else's hand

Rect photoBox(const Rect& p) { return {p.cx() - PHOTO_W / 2, p.y + p.h - PHOTO_TOP - PHOTO_H, PHOTO_W, PHOTO_H}; }

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

// ---- drawing ---------------------------------------------------------------

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
	else if (at.section == Section::Inscriptions)
		xref = "lvl " + std::to_string(Game().journal.Inscriptions()[static_cast<size_t>(page)].level);
	else if (at.section == Section::Expedition)
		xref = "lvl " + std::to_string(Game().journal.Pages()[static_cast<size_t>(page)].level);
	else if (at.section == Section::Log)
		xref = "lvl " + std::to_string(Game().messages.Level());
	auto outer = [&](const Font& f, const std::string& s) {
		return side == 0 ? p.x + PAGE_PAD : p.x + p.w - PAGE_PAD - f.TextWidth(s.c_str());
	};
	beginText();
	text(stamp, outer(stamp, number), top - NUMBER_TOP, number.c_str(), INK_RED);
	if (!xref.empty())
		text(handTiny, outer(handTiny, xref), top - XREF_TOP, xref.c_str(), PENCIL, FADED);
	beginShapes();
	// A new entry: a red dot and "new" in the top inner corner, until the spread is left.
	if (at.section != Section::Log && Game().journal.IsNew(static_cast<JournalPart>(at.section), page)) {
		float dotX = side == 0 ? p.x + p.w - NEW_IN : p.x + NEW_IN;
		newMark(dotX, top - NEW_TOP, NEW_DOT);
		beginText();
		const float w = handSmall.TextWidth("new");
		text(
			handSmall,
			side == 0 ? dotX - NEW_DOT - 0.8f - w : dotX + NEW_DOT + 0.8f,
			top - NEW_TOP - 1.f,
			"new",
			NEW_RED
		);
		beginShapes();
		if (live && std::find(shownNew.begin(), shownNew.end(), std::pair{at.section, page}) == shownNew.end())
			shownNew.emplace_back(at.section, page);
	}

	switch (at.section) {
	case Section::Diary:
		DrawDiary(p, page);
		break;
	case Section::Creatures:
		DrawCreature(p, page);
		break;
	case Section::FieldNotes:
		DrawFieldNote(p, page);
		break;
	case Section::Inscriptions:
		DrawInscription(p, page);
		break;
	case Section::Expedition:
		DrawExpedition(p, page);
		break;
	case Section::Log:
		DrawLog(p, page);
		break;
	case Section::Riddles: {
		Target answer = side == 0 ? Target::AnswerLeft : Target::AnswerRight;
		bool over = live && pointer.hovered.target == answer;
		DrawRiddle(p, page, over, over && pointer.pressed.target == answer);
		break;
	}
	}
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
		textCenteredIn(body, b, label.c_str(), answerHovered ? TEXT_HOVER : GOLD);
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
	std::string seen = "Level " + std::to_string(c.level) + ".";
	if (c.killed && kind != nullptr)
		seen += std::string(" ") + monsterNote(*kind);

	// A long note would push the form off the page: then the tiny hand, the rows closer (plan human-monster-notes).
	Font* font = &handSmall;
	float step = ROW_STEP;
	auto lineCount = [&](const Font& f) { return wrap(f, seen, textW).size(); };
	if (static_cast<float>(lineCount(handSmall)) * ROW_STEP > CREATURE_NOTE_ROOM) {
		font = &handTiny;
		step = std::min(TINY_ROW_STEP, CREATURE_NOTE_ROOM / static_cast<float>(lineCount(handTiny)));
	}
	auto entry = [&](const char* name, const std::string& s) {
		label(name, PENCIL);
		beginText();
		for (const std::string& l : wrap(*font, s, textW)) {
			text(*font, textX, y, l.c_str(), INK_BLUE);
			y -= step;
		}
		beginShapes();
	};
	entry("SEEN", seen);

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
		formField(
			handSmall,
			textX + static_cast<float>(d % 2) * FORM_COLUMN,
			y,
			upper(DAMAGE_TYPE_NAMES[static_cast<size_t>(d)]),
			value
		);
		if (d % 2 == 1)
			y -= ROW_STEP;
	}
	// Under BLUNT, once the venom amulet's poison was tried on it.
	formField(handSmall, textX, y, "POISON", c.TriedPoison() ? poisonResistanceWord(t.poisonResistPercent) : "");
}

void JournalScreen::DrawFieldNote(const Rect& p, int index) {
	const FieldNoteText& note = fieldNoteText(Game().journal.Notes()[static_cast<size_t>(index)]);
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

// A dated heading, the entry in ink under it, smaller when it would not fit. While a new game shows it, the last
// page tells the key that closes the book, in pencil at the bottom.
void JournalScreen::DrawDiary(const Rect& p, int index) {
	const DiaryText& page = diaryText(Game().journal.Diary()[static_cast<size_t>(index)]);
	float top = p.y + p.h;
	float width = p.w - 2 * PAGE_PAD;
	underlinedHeading(handHeading, p.cx(), top - HEADING_TOP, page.title);
	auto wrapAll = [&](const Font& font) {
		std::vector<std::string> lines;
		for (const char* paragraph : page.paragraphs) {
			if (paragraph == nullptr)
				break;
			if (!lines.empty())
				lines.emplace_back(); // a blank line between paragraphs
			for (const std::string& l : wrap(font, paragraph, width))
				lines.push_back(l);
		}
		return lines;
	};
	Font* font = &hand;
	float step = DIARY_STEP;
	std::vector<std::string> lines = wrapAll(hand);
	if (static_cast<float>(lines.size()) * step > DIARY_HEIGHT) {
		font = &handSmall;
		step = DIARY_SMALL_STEP;
		lines = wrapAll(handSmall);
	}
	beginText();
	float y = top - DIARY_TOP;
	for (const std::string& l : lines) {
		text(*font, p.x + PAGE_PAD, y, l.c_str(), INK_BLUE);
		y -= l.empty() ? step / 2 : step;
	}
	if (closesTo != nullptr && index == PageCount(Section::Diary) - 1) {
		std::string hint = keyCapOf(BindAction::Journal) + " or Esc " + closesTo;
		textCentered(handSmall, p.cx(), p.y + COMMENT_BOTTOM - COMMENT_STEP, hint.c_str(), PENCIL, FADED);
	}
	beginShapes();
}

// The status lines of this level (plan message-log), newest first, wrapped to the page, as many as fit on each.
void JournalScreen::PaginateLog() {
	logPages.clear();
	const float width = LEFT_PAGE.w - 2 * PAGE_PAD;
	const float room = LEFT_PAGE.h - LOG_TOP - LOG_BOTTOM;
	float used = 0;
	const std::deque<StatusMessage>& log = Game().messages.Log();
	for (auto it = log.rbegin(); it != log.rend(); ++it) {
		LogEntry entry{{}, it->tone};
		for (const std::string& line : StatusBox::SplitLines(it->Shown()))
			for (const std::string& l : wrap(handSmall, line, width))
				entry.lines.push_back(l);
		const float h = LOG_STEP * static_cast<float>(entry.lines.size()) + LOG_GAP;
		if (logPages.empty() || used + h > room + LOG_GAP) {
			logPages.emplace_back();
			used = 0;
		}
		used += h;
		logPages.back().push_back(entry);
	}
}

void JournalScreen::DrawLog(const Rect& p, int index) {
	const float top = p.y + p.h;
	underlinedHeading(handHeading, p.cx(), top - HEADING_TOP, index == 0 ? "Lately" : "Earlier");
	beginText();
	float y = top - LOG_TOP;
	for (const LogEntry& e : logPages[static_cast<size_t>(index)]) {
		const Color ink = e.tone == StatusTone::Danger ? INK_RED : e.tone == StatusTone::Good ? INK_GREEN : INK_BLUE;
		for (const std::string& l : e.lines) {
			text(handSmall, p.x + PAGE_PAD, y, l.c_str(), ink);
			y -= LOG_STEP;
		}
		y -= LOG_GAP;
	}
	beginShapes();
}

// Someone else's notebook page, torn out and pasted in a little askew: their heading on the first rule, their lines
// on the rules under it, in sepia ink at the margin line.
void JournalScreen::DrawExpedition(const Rect& p, int index) {
	const ExpeditionPage& found = Game().journal.Pages()[static_cast<size_t>(index)];
	const ExpeditionPageText& page = expeditionPageText(found.number);
	const float w = TORN_H * TORN_ASPECT;
	const Rect sheet = {p.cx() - w / 2, p.y + TORN_BOTTOM, w, TORN_H};
	const float tilt = static_cast<float>((found.number * 3) % 5 - 2) * 0.7f;
	glPushMatrix();
	glTranslatef(sheet.cx(), sheet.cy(), 0);
	glRotatef(tilt, 0, 0, 1);
	glTranslatef(-sheet.cx(), -sheet.cy(), 0);
	beginShapes();
	texturedRect(
		{sheet.x + 0.4f, sheet.y - 0.5f, sheet.w, sheet.h},
		Game().assets.textures.expeditionPage.ID(),
		BLACK,
		0.f,
		1.f,
		1.f,
		0.f
	); // pasted on: a thin shadow
	texturedRect(sheet, Game().assets.textures.expeditionPage.ID(), {1, 1, 1}, 0.f, 1.f, 1.f, 0.f);

	const float top = sheet.y + sheet.h;
	const float step = TORN_RULE_STEP * sheet.h;
	const float x = sheet.x + TORN_MARGIN * sheet.w + 1.2f;
	const float width = sheet.x + sheet.w * (1.f - TORN_RIGHT) - x;
	float y = top - TORN_RULE_TOP * sheet.h - TORN_BASELINE; // the letters stand on the rule
	beginText();
	text(hand, x, y, page.heading, SEPIA);
	for (const std::string& l : wrap(handSmall, page.text, width)) {
		y -= step;
		if (y < sheet.y + step)
			break; // past the last rule
		text(handSmall, x, y, l.c_str(), SEPIA);
	}
	beginShapes();
	glPopMatrix();
}

// A creature's model, frame 0 of its move clip, from the side and a little above. As a pencil sketch: the outline
// only (a depth pass first, so only the front surfaces get lines). As a photo: a black and white print, lit, on a
// white-bordered card tilted by `tilt` degrees.
// The hieroglyphs copied from the wall (composed like the wall's), the translation in ink under them, the
// archaeologist's comment pencilled at the bottom.
void JournalScreen::DrawInscription(const Rect& p, int index) {
	const JournalInscription& e = Game().journal.Inscriptions()[static_cast<size_t>(index)];
	float top = p.y + p.h;
	float width = p.w - 2 * PAGE_PAD;
	// A copy made without the hieroglyph skill: the glyphs and a pencilled line, no title, no translation.
	underlinedHeading(handHeading, p.cx(), top - HEADING_TOP, e.translated ? e.title.c_str() : "Hieroglyphs");

	// Composed like the wall's (plan composed-inscriptions): a name cartouche from its copied signs, a text from its
	// style and G: line.
	const InscriptionText* source = isInscriptionId(e.id) ? Game().inscriptions.Find(e.id) : nullptr;
	if (source != nullptr || e.id == NAME_CARTOUCHE_ID) {
		const DecorSet& decor = Game().assets.decor;
		const float paper[3] = {PAPER.r, PAPER.g, PAPER.b};
		glEnable(GL_TEXTURE_2D);
		drawInscription(
			p.cx() - GLYPHS_SIDE / 2,
			top - GLYPHS_TOP - GLYPHS_SIDE,
			GLYPHS_SIDE,
			0.f,
			source != nullptr ? source->style : e.frame,
			source != nullptr ? source->glyphs : e.glyphs,
			{decor.inscriptionPlates, decor.hieroglyphTex, decor.hieroglyphs},
			paper
		);
		glDisable(GL_TEXTURE_2D);
		beginShapes();
	}

	auto wrapAll = [&](const Font& font) {
		std::vector<std::string> lines;
		if (!e.translated)
			return lines;
		for (const std::string& t : e.text)
			for (const std::string& l : wrap(font, t, width))
				lines.push_back(l);
		return lines;
	};
	Font* font = &hand;
	float step = TRANSLATION_STEP;
	std::vector<std::string> lines = wrapAll(hand);
	if (lines.size() > MAX_TRANSLATION_LINES) {
		font = &handSmall;
		step = TRANSLATION_SMALL_STEP;
		lines = wrapAll(handSmall);
		lines.resize(std::min(lines.size(), MAX_TRANSLATION_SMALL_LINES));
	}
	beginText();
	float y = top - TRANSLATION_TOP;
	for (const std::string& l : lines) {
		textCentered(*font, p.cx(), y, l.c_str(), INK_BLUE);
		y -= step;
	}
	const std::string& pencilled = e.translated ? e.note : "No idea what it says. Copied it anyway.";
	std::vector<std::string> comment = wrap(handSmall, pencilled, width);
	comment.resize(std::min(comment.size(), MAX_COMMENT_LINES));
	y = p.y + COMMENT_BOTTOM + COMMENT_STEP * (static_cast<float>(comment.size()) - 1.f);
	for (const std::string& l : comment) {
		textCentered(handSmall, p.cx(), y, l.c_str(), PENCIL, FADED);
		y -= COMMENT_STEP;
	}
	// No hint of a boon before the translation (plan inscription-boons).
	if (e.translated && e.boon.Any())
		textCentered(handSmall, p.cx(), p.y + GIFT_BOTTOM, ("Gift: " + boonText(e.boon)).c_str(), INK_RED);
	beginShapes();
}

void JournalScreen::DrawSketch(int type, const Rect& box, bool photo, float tilt) {
	const MonsterType& t = Game().assets.monsterTypes[static_cast<size_t>(type)];
	const CharacterModel& model = Game().assets.monsterModels[static_cast<size_t>(type)];
	constexpr ModelState POSE =
		ModelState::Move; // the mimic's idle clip is its disguise, the bat's hangs it upside down
	const AnimatedModel* clip = model.Clip(model.Info().Shown(POSE));
	if (clip == nullptr)
		return;
	Texture& print = photoTextures[static_cast<size_t>(type)];
	if (photo && !print.ID()) {
		std::string file = std::string(MEDIA_DIR "textures/") + t.texture + ".png";
		print.LoadPNG(file.c_str(), TexFilter::Mipmapped, photoGrey);
	}

	// The sketches change depth func, culling, polygon mode, colour mask, line width, lighting and blending: all of
	// it goes back as it was, the rest of the game relies on it (GL_LEQUAL depth, game.cpp).
	glPushAttrib(
		GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT |
		GL_LIGHTING_BIT
	);
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
