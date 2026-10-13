#include "records_view.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <GL/gl.h>
#include "../graphics/gl_includes.h"
#include "../input/input.h"
#include "../state/game_state.h"
#include "ui_draw.h"

namespace {
using namespace ui;

// ---- layout ----------------------------------------------------------------

constexpr Rect LIST_PANEL = {12, 20, 136, 64};
constexpr Rect DETAIL_PANEL = {6, 20, 148, 64};

constexpr float SORT_W = 26.f;
constexpr float SORT_H = 7.f;
constexpr float SORT_GAP = 2.f;
constexpr float SORT_TOP = LIST_PANEL.y + LIST_PANEL.h - 4.f;
constexpr float LIST_LEFT = LIST_PANEL.x + 5.f;
constexpr float LIST_RIGHT = LIST_PANEL.x + LIST_PANEL.w - 5.f;
constexpr float HEADER_Y = SORT_TOP - SORT_H - 4.2f; // baseline of the column names
constexpr float HEADER_RULE = HEADER_Y - 1.1f;
constexpr float ROW_H = 4.3f;
constexpr float RANK_X = 3.f; // from the row's left edge
constexpr float NAME_X = 16.f;
constexpr float SCORE_RIGHT = 4.f; // from the row's right edge

// Detail: four columns of headings and their rows; the last one the time per level, in two halves.
constexpr float COLUMN_GAP = 4.f;
constexpr float DETAIL_LEFT = DETAIL_PANEL.x + 5.f;
constexpr int DETAIL_COLUMNS = 4;
constexpr float DETAIL_COLUMN_W = (DETAIL_PANEL.w - 10.f - (DETAIL_COLUMNS - 1) * COLUMN_GAP) / DETAIL_COLUMNS;
constexpr float DETAIL_TOP = DETAIL_PANEL.y + DETAIL_PANEL.h - 8.f; // first heading's baseline
constexpr float DETAIL_BOTTOM = DETAIL_PANEL.y + 3.5f; // no row's baseline below it
constexpr float DETAIL_LINE = 3.35f;

Rect sortRect(int order) {
	return {LIST_LEFT + static_cast<float>(order) * (SORT_W + SORT_GAP), SORT_TOP - SORT_H, SORT_W, SORT_H};
}

Rect rowRect(int visibleRow) {
	return {LIST_LEFT, HEADER_RULE - static_cast<float>(visibleRow + 1) * ROW_H, LIST_RIGHT - LIST_LEFT, ROW_H};
}

float detailColumn(int column) { return DETAIL_LEFT + static_cast<float>(column) * (DETAIL_COLUMN_W + COLUMN_GAP); }

std::string number(int n) { return std::to_string(n); }

// A detail column: headings and label / value rows, drawn top down.
struct DetailLine {
	bool heading;
	std::string label;
	std::string value;
};

void drawColumn(const std::vector<DetailLine>& lines, float x, Font& heading, Font& small) {
	float y = DETAIL_TOP;
	const float right = x + DETAIL_COLUMN_W;
	for (size_t i = 0; i < lines.size(); i++) {
		const DetailLine& l = lines[i];
		if (y < DETAIL_BOTTOM) { // out of room: say how many lines did not fit
			beginText();
			text(small, x + 1.f, y + DETAIL_LINE, ("+ " + number(static_cast<int>(lines.size() - i)) + " more").c_str(),
				 LABEL_DIM);
			beginShapes();
			return;
		}
		if (l.heading) {
			if (i > 0)
				y -= 1.5f;
			beginText();
			text(heading, x, y, l.label.c_str(), GOLD);
			beginShapes();
			const float after = x + heading.TextWidth(l.label.c_str()) + 1.5f;
			if (after < right)
				line(after, y + 1.2f, right, y + 1.2f, BRONZE, 1.f, 1.f);
			y -= DETAIL_LINE + 1.2f;
			continue;
		}
		beginText();
		text(small, x + 1.f, y, l.label.c_str(), LABEL);
		text(small, right - small.TextWidth(l.value.c_str()), y, l.value.c_str(), GOLD);
		beginShapes();
		y -= DETAIL_LINE;
	}
}

// The time in play per level: "Levels", then the levels' numbers and times in two halves side by side.
void drawLevels(const std::vector<int>& levelMs, float x, Font& heading, Font& small) {
	drawColumn({{true, "Levels", ""}}, x, heading, small);
	const int rows = static_cast<int>(std::floor((DETAIL_TOP - DETAIL_LINE - 1.2f - DETAIL_BOTTOM) / DETAIL_LINE)) + 1;
	const float half = (DETAIL_COLUMN_W - 2.f) / 2;
	beginText();
	for (int level = 0; level < static_cast<int>(levelMs.size()) && level < 2 * rows; level++) {
		const int side = level / rows; // the left or the right half
		const float left = x + 1.f + static_cast<float>(side) * (half + 2.f);
		const float y = DETAIL_TOP - DETAIL_LINE - 1.2f - static_cast<float>(level % rows) * DETAIL_LINE;
		const std::string time = formatPlayTime(levelMs[static_cast<size_t>(level)]);
		text(small, left, y, number(level + 1).c_str(), LABEL);
		text(small, left + half - small.TextWidth(time.c_str()), y, time.c_str(), GOLD);
	}
	if (levelMs.empty())
		text(small, x + 1.f, DETAIL_TOP - DETAIL_LINE - 1.2f, "None", LABEL);
	beginShapes();
}

std::vector<DetailLine> countLines(const char* title, const std::vector<std::pair<std::string, int>>& counts,
								   int total) {
	std::vector<DetailLine> lines = {{true, std::string(title) + " (" + number(total) + ")", ""}};
	if (counts.empty())
		lines.push_back({false, "None", ""});
	for (const auto& [name, count] : counts)
		lines.push_back({false, name, number(count)});
	return lines;
}
} // namespace

std::vector<RunRecord> RecordsView::shown() const { return Game().leaderboard.Shown(); }

void RecordsView::Open(int markedRecord) {
	order = RecordOrder::Time;
	detail = NONE;
	first = 0;
	selected = NONE;
	marked = markedRecord;
	if (marked == NONE)
		return;
	const std::vector<RunRecord> records = shown();
	const std::vector<int> ranked = rankRecords(records, order);
	const auto at = std::find(ranked.begin(), ranked.end(), marked);
	if (at != ranked.end())
		select(static_cast<int>(at - ranked.begin()), static_cast<int>(records.size()));
}

std::string RecordsView::Caption() const {
	if (detail == NONE)
		return "Hall of Records";
	const std::vector<RunRecord> records = shown();
	return detail < static_cast<int>(records.size()) ? records[static_cast<size_t>(detail)].name : "";
}

const char* RecordsView::Hint() const {
	return detail == NONE ? "Click a run for its details    Up / Down, Enter    Esc: back" : "Esc: back to the list";
}

ui::Rect RecordsView::Panel() const { return detail == NONE ? LIST_PANEL : DETAIL_PANEL; }

void RecordsView::select(int rank, int count) {
	if (count <= 0)
		return;
	selected = std::clamp(rank, 0, count - 1);
	if (selected < first)
		first = selected;
	if (selected >= first + VISIBLE_ROWS)
		first = selected - VISIBLE_ROWS + 1;
}

void RecordsView::Scroll(int rows) {
	if (detail != NONE)
		return;
	const int count = static_cast<int>(shown().size());
	first = std::clamp(first + rows, 0, std::max(0, count - VISIBLE_ROWS));
}

void RecordsView::SpecialKey(int key) {
	if (detail != NONE)
		return;
	const int count = static_cast<int>(shown().size());
	if (key == SPECIAL_MOVE_UP)
		select(selected == NONE ? 0 : selected - 1, count);
	else if (key == SPECIAL_MOVE_DOWN)
		select(selected == NONE ? 0 : selected + 1, count);
}

void RecordsView::Key(unsigned char key) {
	if (detail != NONE || key != KEY_ENTER || selected == NONE)
		return;
	const std::vector<int> ranked = rankRecords(shown(), order);
	if (selected < static_cast<int>(ranked.size()))
		detail = ranked[static_cast<size_t>(selected)];
}

int RecordsView::TargetAt(float cx, float cy) const {
	if (detail != NONE)
		return NONE;
	for (int o = 0; o < RECORD_ORDER_COUNT; o++)
		if (sortRect(o).contains(cx, cy))
			return SORT_BASE + o;
	const int rows = std::min(VISIBLE_ROWS, static_cast<int>(shown().size()) - first);
	for (int row = 0; row < rows; row++)
		if (rowRect(row).contains(cx, cy))
			return ROW_BASE + row;
	return NONE;
}

void RecordsView::Activate(int target) {
	if (target >= ROW_BASE) {
		const std::vector<int> ranked = rankRecords(shown(), order);
		const int rank = first + target - ROW_BASE;
		if (rank < static_cast<int>(ranked.size())) {
			selected = rank;
			detail = ranked[static_cast<size_t>(rank)];
		}
		return;
	}
	order = static_cast<RecordOrder>(target - SORT_BASE);
	first = 0;
	selected = NONE;
}

// ---- drawing ---------------------------------------------------------------

void RecordsView::Draw(Font& heading, Font& body, Font& small, int hovered, int pressed) {
	if (detail == NONE)
		drawList(heading, body, small, hovered, pressed);
	else
		drawDetail(heading, small);
}

void RecordsView::drawList(Font& heading, Font& body, Font& small, int hovered, int pressed) {
	for (int o = 0; o < RECORD_ORDER_COUNT; o++) {
		const bool active = static_cast<int>(order) == o;
		const bool isHovered = hovered == SORT_BASE + o && !active;
		Rect r = tile(sortRect(o), active ? TileStyle::Lapis : TileStyle::Stone, isHovered,
					  isHovered && pressed == SORT_BASE + o);
		beginText();
		textCentered(heading, r.cx(), r.y + 1.4f, RECORD_ORDER_NAMES[static_cast<size_t>(o)],
					 active		 ? GOLD
					 : isHovered ? TEXT_HOVER
								 : LABEL);
		beginShapes();
	}

	const Rect header = rowRect(-1);
	line(header.x, HEADER_RULE, header.x + header.w, HEADER_RULE, BRONZE, 1.f, 1.f);
	const char* scoreName = order == RecordOrder::Time ? "Time" : order == RecordOrder::Xp ? "XP" : "Explored";
	beginText();
	text(small, header.x + RANK_X, HEADER_Y, "Rank", LABEL);
	text(small, header.x + NAME_X, HEADER_Y, "Name", LABEL);
	text(small, header.x + header.w - SCORE_RIGHT - small.TextWidth(scoreName), HEADER_Y, scoreName, LABEL);
	beginShapes();

	const std::vector<RunRecord> records = shown();
	const std::vector<int> ranked = rankRecords(records, order);
	const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GameClock::now()) * 0.004f);
	for (int row = 0; row < VISIBLE_ROWS && first + row < static_cast<int>(ranked.size()); row++) {
		const int rank = first + row;
		const int index = ranked[static_cast<size_t>(rank)];
		const RunRecord& r = records[static_cast<size_t>(index)];
		const Rect band = rowRect(row);
		const bool isHovered = hovered == ROW_BASE + row;
		const bool isMarked = index == marked && !r.placeholder;
		if (rank % 2 == 0)
			fillRect(band, {1, 1, 1}, {1, 1, 1}, 0.035f);
		if (isMarked) // the run just recorded: a breathing lapis band
			fillRect(band, LAPIS_HOVER_TOP, LAPIS_DARK, 0.45f + 0.25f * pulse);
		if (isHovered)
			fillRect(band, STONE_HOVER_TOP, STONE_HOVER_BOTTOM, 0.8f);
		if (rank == selected || isHovered)
			strokeRect(band, isHovered ? GOLD_BRIGHT : GOLD, 1.f, 1.5f);

		const Color c = isHovered ? TEXT_HOVER : isMarked ? GOLD_BRIGHT : LABEL;
		const std::string score = scoreText(r, order);
		beginText();
		text(body, band.x + RANK_X, band.y + 0.9f, r.placeholder ? "-" : number(rank + 1).c_str(), c);
		text(body, band.x + NAME_X, band.y + 0.9f, r.name.c_str(), c);
		text(body, band.x + band.w - SCORE_RIGHT - body.TextWidth(score.c_str()), band.y + 0.9f, score.c_str(),
			 isHovered ? TEXT_HOVER : GOLD);
		beginShapes();
	}

	if (static_cast<int>(ranked.size()) > VISIBLE_ROWS) { // which part of a long board shows
		char range[48];
		std::snprintf(range, sizeof(range), "%d-%d of %d", first + 1,
					  std::min(first + VISIBLE_ROWS, static_cast<int>(ranked.size())), static_cast<int>(ranked.size()));
		beginText();
		text(small, LIST_RIGHT - small.TextWidth(range), SORT_TOP - SORT_H + 2.f, range, LABEL_DIM);
		beginShapes();
	}
}

void RecordsView::drawDetail(Font& heading, Font& small) const {
	const std::vector<RunRecord> records = shown();
	if (detail >= static_cast<int>(records.size()))
		return;
	const RunRecord& r = records[static_cast<size_t>(detail)];
	const bool real = !r.placeholder;
	auto value = [real](int n) { return real ? number(n) : std::string("-"); };

	const std::vector<DetailLine> run = {
		{true, "Run", ""},
		{false, "Time", real ? formatPlayTime(r.playMs) : "-"},
		{false, "Player level", number(r.playerLevel)},
		{false, "XP", value(r.xp)},
		{false, "Max health", value(r.maxHp)},
		{false, "Stamina", value(r.stamina)},
		{false, "Armour", value(r.armor)},
		{false, "Might", value(r.might)},
		{false, "Date", real ? r.date : "-"},
		{false, "Version", real ? r.version : "-"},
		{true, "Fights", ""},
		{false, "Damage dealt", value(r.damageDealt)},
		{false, "Damage taken", value(r.damageTaken)},
		{false, "Biggest hit", value(r.biggestHit)},
	};
	std::vector<DetailLine> found = {
		{true, "Explored (" + value(explorationScore(r)) + ")", ""},
		{false, "Inscriptions", value(r.inscriptions)},
		{false, "Treasures", value(r.treasures)},
		{false, "Riddles", value(r.riddles)},
	};
	for (const DetailLine& l : countLines("Potions", r.potions, r.PotionsTotal()))
		found.push_back(l);
	found.push_back({true, "Skills (" + number(static_cast<int>(r.skills.size())) + ")", ""});
	if (r.skills.empty())
		found.push_back({false, "None", ""});
	for (const std::string& skill : r.skills)
		found.push_back({false, skill, ""});

	drawColumn(run, detailColumn(0), heading, small);
	drawColumn(countLines("Slain", r.slain, r.SlainTotal()), detailColumn(1), heading, small);
	drawColumn(found, detailColumn(2), heading, small);
	drawLevels(r.levelMs, detailColumn(3), heading, small);
}
