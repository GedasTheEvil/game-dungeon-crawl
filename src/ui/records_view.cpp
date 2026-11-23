#include "records_view.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "../render/gl_includes.h"
#include "../level/input.h"
#include "../state/game_state.h"
#include "../render/ui_draw.h"
#include "../level/bindings.h"
#include "../level/skills.h"
#include "../graphics/texture_registry.h"
#include <array>

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
constexpr float ENDING_X = 70.f; // plan ending-choice: past a 20-letter name
constexpr float SCORE_RIGHT = 4.f; // from the row's right edge

// Detail: four columns of headings and their rows; the last one the time per level, in two halves. The columns
// scroll together, by DETAIL_LINE a step (plan endgame-stats-skills-weapon); a row is drawn only when its baseline is
// within DETAIL_TOP .. DETAIL_BOTTOM.
constexpr float COLUMN_GAP = 4.f;
constexpr float DETAIL_LEFT = DETAIL_PANEL.x + 5.f;
constexpr int DETAIL_COLUMNS = 4;
constexpr float DETAIL_COLUMN_W = (DETAIL_PANEL.w - 10.f - (DETAIL_COLUMNS - 1) * COLUMN_GAP) / DETAIL_COLUMNS;
constexpr float DETAIL_TOP = DETAIL_PANEL.y + DETAIL_PANEL.h - 8.f; // first heading's baseline
constexpr float DETAIL_BOTTOM = DETAIL_PANEL.y + 3.5f; // no row's baseline below it
constexpr float DETAIL_LINE = 3.35f;
constexpr float HEADING_GAP = 1.5f; // above a heading that is not the column's first
constexpr float HEADING_RULE_GAP = 1.2f; // below a heading, its rule's room
constexpr float ICON_LINE = 4.6f; // a skill's row: its icon beside the name
constexpr float ICON_SIZE = 3.8f;
constexpr float ICON_DROP = 1.f; // the icon's foot under the baseline
constexpr int DETAIL_VISIBLE = static_cast<int>((DETAIL_TOP - DETAIL_BOTTOM) / DETAIL_LINE) + 1; // steps shown
constexpr Rect DETAIL_BAR = {
	DETAIL_PANEL.x + DETAIL_PANEL.w - 4.f,
	DETAIL_BOTTOM - ICON_DROP,
	1.6f,
	DETAIL_TOP + 2.5f - (DETAIL_BOTTOM - ICON_DROP)
};

Rect sortRect(int order) {
	return {LIST_LEFT + static_cast<float>(order) * (SORT_W + SORT_GAP), SORT_TOP - SORT_H, SORT_W, SORT_H};
}

Rect rowRect(int visibleRow) {
	return {LIST_LEFT, HEADER_RULE - static_cast<float>(visibleRow + 1) * ROW_H, LIST_RIGHT - LIST_LEFT, ROW_H};
}

float detailColumn(int column) { return DETAIL_LEFT + static_cast<float>(column) * (DETAIL_COLUMN_W + COLUMN_GAP); }

std::string number(int n) { return std::to_string(n); }

// A detail column: headings and label / value rows, drawn top down; a row with a skill icon (icon >= 0) is taller.
struct DetailLine {
	bool heading;
	std::string label;
	std::string value;
	int icon = -1; // a cell of the skill icon atlas
};
using DetailColumns = std::array<std::vector<DetailLine>, DETAIL_COLUMNS - 1>;

bool rowShown(float y) { return y <= DETAIL_TOP + 0.01f && y >= DETAIL_BOTTOM - 0.01f; }

// How far the baseline goes down from line i to the next.
float lineStep(const DetailLine& l) {
	return l.heading ? DETAIL_LINE + HEADING_RULE_GAP : l.icon >= 0 ? ICON_LINE : DETAIL_LINE;
}

// The last row's baseline under DETAIL_TOP, unscrolled.
float columnDrop(const std::vector<DetailLine>& lines) {
	float drop = 0.f;
	for (size_t i = 0; i < lines.size(); i++) {
		if (lines[i].heading && i > 0)
			drop += HEADING_GAP;
		if (i + 1 < lines.size())
			drop += lineStep(lines[i]);
	}
	return drop;
}

// The time per level: in two halves of this many rows, at least as many as fit.
int levelRows(size_t levels) {
	const int fit = static_cast<int>(
						std::floor((DETAIL_TOP - DETAIL_LINE - HEADING_RULE_GAP - DETAIL_BOTTOM) / DETAIL_LINE)
					) +
					1;
	return std::max(fit, static_cast<int>((levels + 1) / 2));
}

void drawColumn(const std::vector<DetailLine>& lines, float x, float scrolled, Font& heading, Font& small, int icons) {
	float y = DETAIL_TOP + scrolled;
	const float right = x + DETAIL_COLUMN_W;
	for (size_t i = 0; i < lines.size(); i++) {
		const DetailLine& l = lines[i];
		if (l.heading && i > 0)
			y -= HEADING_GAP;
		if (rowShown(y) && l.heading) {
			beginText();
			text(heading, x, y, l.label.c_str(), GOLD);
			beginShapes();
			const float after = x + heading.TextWidth(l.label.c_str()) + 1.5f;
			if (after + 3.f < right) // no stub after a long heading
				line(after, y + 1.2f, right, y + 1.2f, BRONZE, 1.f, 1.f);
		} else if (rowShown(y)) {
			float left = x + 1.f;
			if (l.icon >= 0) {
				atlasIcon(
					{left, y - ICON_DROP, ICON_SIZE, ICON_SIZE},
					icons,
					l.icon,
					SKILL_ICON_COLUMNS,
					SKILL_ICON_ROWS,
					{1, 1, 1}
				);
				left += ICON_SIZE + 1.f;
			}
			beginText();
			text(small, left, y, l.label.c_str(), LABEL);
			text(small, right - small.TextWidth(l.value.c_str()), y, l.value.c_str(), GOLD);
			beginShapes();
		}
		y -= lineStep(l);
	}
}

// The time in play per level: "Levels", then the levels' numbers and times in two halves side by side.
void drawLevels(const std::vector<int>& levelMs, float x, float scrolled, Font& heading, Font& small) {
	drawColumn({{true, "Levels", ""}}, x, scrolled, heading, small, 0);
	const int rows = levelRows(levelMs.size());
	const float half = (DETAIL_COLUMN_W - 2.f) / 2;
	const float first = DETAIL_TOP + scrolled - DETAIL_LINE - HEADING_RULE_GAP;
	beginText();
	for (int level = 0; level < static_cast<int>(levelMs.size()); level++) {
		const int side = level / rows; // the left or the right half
		const float left = x + 1.f + static_cast<float>(side) * (half + 2.f);
		const float y = first - static_cast<float>(level % rows) * DETAIL_LINE;
		if (!rowShown(y))
			continue;
		const std::string time = formatPlayTime(levelMs[static_cast<size_t>(level)]);
		text(small, left, y, number(level + 1).c_str(), LABEL);
		text(small, left + half - small.TextWidth(time.c_str()), y, time.c_str(), GOLD);
	}
	if (levelMs.empty() && rowShown(first))
		text(small, x + 1.f, first, "None", LABEL);
	beginShapes();
}

std::vector<DetailLine>
countLines(const char* title, const std::vector<std::pair<std::string, int>>& counts, int total) {
	std::vector<DetailLine> lines = {{true, std::string(title) + " (" + number(total) + ")", ""}};
	if (counts.empty())
		lines.push_back({false, "None", ""});
	for (const auto& [name, count] : counts)
		lines.push_back({false, name, number(count)});
	return lines;
}

// The icon cell of a skill named as in a record ("Stun", "Stun II"); -1 for a name this build does not know.
int skillIconOf(const std::string& name) {
	const std::string base = name.ends_with(" II") ? name.substr(0, name.size() - 3) : name;
	for (int i = 0; i < SKILL_COUNT; i++)
		if (base == SKILLS[static_cast<size_t>(i)].name)
			return skillIcon(static_cast<SkillId>(i));
	return -1;
}

// The run's first three columns: Run, Fights and the favourite weapon; Slain; the ending, Explored, Potions, Skills.
DetailColumns detailColumns(const RunRecord& r) {
	const bool real = !r.placeholder;
	auto value = [real](int n) { return real ? number(n) : std::string("-"); };
	std::vector<DetailLine> run = {
		{true, "Run", ""},
		{false, "Time", real ? formatPlayTime(r.playMs) : "-"},
		{false, "Player level", number(r.playerLevel)},
		{false, "XP", value(r.xp)},
		{false, "Max health", value(r.maxHp)},
		{false, "Stamina", value(r.stamina)},
		{false, "Armour", value(r.armor)},
		{false, "Might", value(r.might)},
		{true, "Fights", ""},
		{false, "Damage dealt", value(r.damageDealt)},
		{false, "Damage taken", value(r.damageTaken)},
		{false, "Biggest hit", value(r.biggestHit)},
		{false, "Times poisoned", value(r.poisoned)},
		{false, "Times cursed", value(r.cursed)},
		{false, "Deaths", value(r.deaths)},
		{true, "Favourite weapon", ""}, // plan endgame-stats-skills-weapon: the most damage dealt
	};
	if (!real || r.favoriteWeapon.empty())
		run.push_back({false, real ? "None" : "-", ""});
	else
		run.push_back({false, r.favoriteWeapon, number(r.favoriteWeaponDealt)});
	std::vector<DetailLine> found = {
		{true, "Ending", ""}, // plan ending-choice
		{false, real ? endingName(r.ending) : "-", ""},
		{true, "Explored (" + value(explorationScore(r)) + ")", ""},
		{false, "Inscriptions", value(r.inscriptions)},
		{false, "Treasures", value(r.treasures)},
		{false, "Riddles", value(r.riddles)},
		{false, "Secrets", real ? number(r.secrets) + " / " + number(campaignSecretTotal()) : "-"},
	};
	for (const DetailLine& l : countLines("Potions", r.potions, r.PotionsTotal()))
		found.push_back(l);
	found.push_back({true, "Skills (" + number(static_cast<int>(r.skills.size())) + ")", ""});
	if (r.skills.empty())
		found.push_back({false, "None", ""});
	for (const std::string& skill : r.skills)
		found.push_back({false, skill, "", skillIconOf(skill)});
	return {run, countLines("Slain", r.slain, r.SlainTotal()), found};
}

} // namespace

std::vector<RunRecord> RecordsView::shown() const { return world.leaderboard.Shown(); }

void RecordsView::Open(int markedRecord) {
	order = RecordOrder::Time;
	CloseDetail();
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

std::string RecordsView::Hint() const {
	if (detail == NONE)
		return "Click a run for its details    Up / Down, Enter    Esc: back";
	const std::vector<RunRecord> records = shown();
	if (detail >= static_cast<int>(records.size()) || records[static_cast<size_t>(detail)].placeholder)
		return "Esc: back to the list";
	const RunRecord& r = records[static_cast<size_t>(detail)];
	const std::string scroll = DetailBar().Shown() ? "    Up / Down, PgUp / PgDn: scroll" : "";
	return r.date + "    version " + r.version + scroll + "    Esc: back"; // the run's date and build
}

ui::Rect RecordsView::Panel() const { return detail == NONE ? LIST_PANEL : DETAIL_PANEL; }

ui::ScrollBar RecordsView::DetailBar() const {
	const std::vector<RunRecord> records = shown();
	if (detail == NONE || detail >= static_cast<int>(records.size()))
		return {DETAIL_BAR, 0, DETAIL_VISIBLE};
	const RunRecord& r = records[static_cast<size_t>(detail)];
	float drop = DETAIL_LINE + HEADING_RULE_GAP + static_cast<float>(levelRows(r.levelMs.size()) - 1) * DETAIL_LINE;
	for (const std::vector<DetailLine>& column : detailColumns(r))
		drop = std::max(drop, columnDrop(column));
	const float over = drop - (DETAIL_TOP - DETAIL_BOTTOM); // how far the lowest row is under the panel's foot
	const int extra = over > 0.f ? static_cast<int>(std::ceil(over / DETAIL_LINE - 0.01f)) : 0;
	return {DETAIL_BAR, DETAIL_VISIBLE + extra, DETAIL_VISIBLE};
}

void RecordsView::openDetail(int index) {
	detail = index;
	detailScroll = 0;
	detailDrag.Up();
}

void RecordsView::CloseDetail() {
	detail = NONE;
	detailScroll = 0;
	detailDrag.Up();
}

bool RecordsView::BarButton(bool down, float cx, float cy) {
	if (!down) {
		const bool took = detailDrag.Dragging();
		detailDrag.Up();
		return took;
	}
	return detail != NONE && detailDrag.Down(DetailBar(), cx, cy, detailScroll);
}

void RecordsView::BarDrag(float cy) { detailDrag.Move(DetailBar(), cy, detailScroll); }

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
	if (detail != NONE) {
		detailScroll = std::clamp(detailScroll + rows, 0, DetailBar().MaxScroll());
		return;
	}
	const int count = static_cast<int>(shown().size());
	first = std::clamp(first + rows, 0, std::max(0, count - VISIBLE_ROWS));
}

void RecordsView::SpecialKey(int key) {
	if (detail != NONE) { // the details scroll: a row, or a page
		const int page = DETAIL_VISIBLE - 1;
		Scroll(
			key == SPECIAL_MOVE_UP		   ? -1
			: key == SPECIAL_MOVE_DOWN	   ? 1
			: key == SpecialKey::PAGE_UP   ? -page
			: key == SpecialKey::PAGE_DOWN ? page
										   : 0
		);
		return;
	}
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
		openDetail(ranked[static_cast<size_t>(selected)]);
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
			openDetail(ranked[static_cast<size_t>(rank)]);
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
		Rect r = tile(
			sortRect(o),
			active ? TileStyle::Lapis : TileStyle::Stone,
			isHovered,
			isHovered && pressed == SORT_BASE + o
		);
		beginText();
		textCentered(
			heading,
			r.cx(),
			r.y + 1.4f,
			RECORD_ORDER_NAMES[static_cast<size_t>(o)],
			active		? GOLD
			: isHovered ? TEXT_HOVER
						: LABEL
		);
		beginShapes();
	}

	const Rect header = rowRect(-1);
	line(header.x, HEADER_RULE, header.x + header.w, HEADER_RULE, BRONZE, 1.f, 1.f);
	const char* scoreName = order == RecordOrder::Time ? "Time" : order == RecordOrder::Xp ? "XP" : "Explored";
	beginText();
	text(small, header.x + RANK_X, HEADER_Y, "Rank", LABEL);
	text(small, header.x + NAME_X, HEADER_Y, "Name", LABEL);
	text(small, header.x + ENDING_X, HEADER_Y, "Ending", LABEL);
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
		text(
			body,
			band.x + ENDING_X,
			band.y + 0.9f,
			r.placeholder					   ? "-"
			: r.ending == RunEnding::Renounced ? "Renounced"
											   : "Immortal",
			isHovered ? TEXT_HOVER : LABEL_DIM
		);
		text(
			body,
			band.x + band.w - SCORE_RIGHT - body.TextWidth(score.c_str()),
			band.y + 0.9f,
			score.c_str(),
			isHovered ? TEXT_HOVER : GOLD
		);
		beginShapes();
	}

	if (static_cast<int>(ranked.size()) > VISIBLE_ROWS) { // which part of a long board shows
		char range[48];
		std::snprintf(
			range,
			sizeof(range),
			"%d-%d of %d",
			first + 1,
			std::min(first + VISIBLE_ROWS, static_cast<int>(ranked.size())),
			static_cast<int>(ranked.size())
		);
		beginText();
		text(small, LIST_RIGHT - small.TextWidth(range), SORT_TOP - SORT_H + 2.f, range, LABEL_DIM);
		beginShapes();
	}
}

void RecordsView::drawDetail(Font& heading, Font& small) {
	const std::vector<RunRecord> records = shown();
	if (detail >= static_cast<int>(records.size()))
		return;
	const RunRecord& r = records[static_cast<size_t>(detail)];
	const ScrollBar bar = DetailBar();
	detailScroll = std::clamp(detailScroll, 0, bar.MaxScroll());
	const float scrolled = static_cast<float>(detailScroll) * DETAIL_LINE;
	const int icons = world.assets.textures.skillIcons.ID();
	const DetailColumns columns = detailColumns(r);
	for (size_t c = 0; c < columns.size(); c++)
		drawColumn(columns[c], detailColumn(static_cast<int>(c)), scrolled, heading, small, icons);
	drawLevels(r.levelMs, detailColumn(DETAIL_COLUMNS - 1), scrolled, heading, small);
	if (bar.Shown()) { // the scroll bar: a track and a lapis thumb, lighter while dragged
		fillRect(bar.track, WELL, WELL, 0.9f);
		fillRect(bar.Thumb(detailScroll), detailDrag.Dragging() ? LAPIS_HOVER_TOP : LAPIS, LAPIS_DARK, 1.f);
	}
}
