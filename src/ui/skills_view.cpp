#include "skills_view.h"
#include "../level/ui/skills_layout.h"
#include "../core/timer.h"
#include "../render/gl/gl_includes.h"
#include "../level/input/keys.h"
#include "../level/rules/player_actions.h"
#include "../state/game_state.h"
#include "../state/ui_context.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

using namespace ui;
using namespace SkillsLayout;

namespace {
// ---- layout ----
constexpr Rect POINTS_ICON = {7.f, 76.6f, 5.6f, 5.6f};
constexpr float POINTS_TEXT_X = 14.f;
constexpr float CARD_WELL_INSET = 1.f;
constexpr float CARD_NAME_Y = 7.3f; // baselines above the card's bottom
constexpr float CARD_EFFECT_Y = 4.2f; // the first effect line; a second one sits CARD_EFFECT_STEP lower
constexpr float CARD_EFFECT_STEP = 2.9f;
constexpr float CARD_TEXT_PAD = 1.8f; // the text's gap to the icon well and to the card's right edge
constexpr float CARD_MARK = 1.2f; // the learnt / sealed mark in the top right corner
constexpr Rect DETAIL_ICON = {DETAIL_PANEL.x + DETAIL_PANEL.w / 2 - 10.f, 60.f, 20.f, 20.f};
constexpr float DETAIL_NAME_Y = 54.f;
constexpr float DETAIL_RULE_Y = 51.6f;
constexpr float DETAIL_PAD = 4.f;
constexpr float DETAIL_EFFECT_Y = 46.8f;
constexpr float DETAIL_EFFECT_STEP = 4.4f;
constexpr float DETAIL_LORE_STEP = 3.6f;
constexpr float DETAIL_STATUS_Y = 26.f;
// The effect and lore scroll by lines between these baselines, clear of the rule and the status line; their bar at the
// papyrus's right edge.
constexpr float DETAIL_TEXT_BOTTOM = 29.6f;
constexpr float DETAIL_TEXT_H = DETAIL_EFFECT_Y - DETAIL_TEXT_BOTTOM;
constexpr Rect DETAIL_BAR =
	{DETAIL_PANEL.x + DETAIL_PANEL.w - 2.6f, DETAIL_TEXT_BOTTOM - 1.f, 0.8f, DETAIL_TEXT_H + 4.f};

constexpr Color DIM_TEXT = {0.40f, 0.33f, 0.23f}; // a sealed card
constexpr Color DIM_ICON = {0.35f, 0.30f, 0.25f};
constexpr Color POOR_ICON = {0.62f, 0.56f, 0.50f}; // no heka to learn it
constexpr Color WAX = {0.62f, 0.17f, 0.08f};
constexpr Color TOAST = {1.f, 0.9f, 0.6f};
constexpr float PULSE_RATE = 0.004f; // radians per ms

Rect visibleArea(const RenderSettings& render) { return ui::visibleArea(CANVAS_W, CANVAS_H, render.resX, render.resY); }

void toCanvas(const RenderSettings& render, int mouseX, int mouseY, float& x, float& y) {
	ui::toCanvas(visibleArea(render), render.resX, render.resY, mouseX, mouseY, x, y);
}

const Heka& heka(const ScreenWorld& world) { return world.player.stats.heka; }

void skillIconAt(const ScreenWorld& world, const Rect& r, int cell, Color tint) {
	atlasIcon(r, world.assets.textures.skillIcons.ID(), cell, SKILL_ICON_COLUMNS, SKILL_ICON_ROWS, tint);
	beginShapes();
}

// The papyrus scroll of the details (docs/ui.md, Papyrus scroll).
void papyrus(const ScreenWorld& world, const Rect& r) {
	texturedRect(r, world.assets.textures.papyrus.ID(), {1, 1, 1}, 0.04f, 0.07f, 0.96f, 0.93f);
	beginShapes();
	strokeRect(r, BRONZE, 1.f, 3.f);
	cornerStuds(r);
}
} // namespace

// ---- input -----------------------------------------------------------------

void SkillsScreen::Select(int card) {
	if (card != selected)
		confirming = false;
	const int was = selected;
	selected = std::clamp(card, 0, SKILL_COUNT - 1);
	scroll = scrollTo(selected, scroll);
	if (selected != was) {
		detailScroll = 0;
		detailDrag.Up();
	}
}

void SkillsScreen::ScrollDetail(int lines) {
	detailScroll = std::clamp(detailScroll + lines, 0, DetailBar().MaxScroll());
}

void SkillsScreen::MoveSelection(int dx, int dy) {
	const int column = selected % COLUMNS + dx;
	const int row = selected / COLUMNS - dy; // up is the row before
	if (column < 0 || column >= COLUMNS || row < 0 || row >= ROWS || row * COLUMNS + column >= SKILL_COUNT)
		return;
	Select(row * COLUMNS + column);
}

// Learn, or on a learnt upgradable skill its rank II (plan skill-upgrades): the first press asks, the second spends.
// Refused (sealed, no heka): the toast tells why.
void SkillsScreen::PressLearn() {
	const auto id = static_cast<SkillId>(selected);
	const SkillDef& def = skillDef(id);
	Heka& points = world.player.stats.heka;
	const int now = GameClock::now();
	if (points.Has(id)) {
		if (points.Rank(id) == 2 || !upgradable(id))
			return;
		if (!points.CanUpgrade(id)) {
			toast.Show("No heka to spend: slay a guardian", now);
			return;
		}
		if (!confirming) {
			confirming = true;
			return;
		}
		confirming = false;
		if (const std::optional<std::string> upgraded = upgradeSkill(world.player.stats, world.events, id))
			toast.Show(*upgraded, now);
		detailScroll = 0; // rank II's text
		return;
	}
	if (!points.CanLearn(id)) {
		toast.Show(def.sealed ? "Its heka sleeps: not in this tomb yet" : "No heka to spend: slay a guardian", now);
		return;
	}
	if (!confirming) {
		confirming = true;
		return;
	}
	confirming = false;
	if (const std::optional<std::string> learnt = learnSkill(world.player.stats, world.journal, world.events, id))
		toast.Show(*learnt, now);
}

void SkillsScreen::UpdateHover(float x, float y) {
	const int card = cardAt(x, y, scroll);
	pointer.Move(
		card >= 0					  ? Pick{Target::Card, card}
		: LEARN_BUTTON.contains(x, y) ? Pick{Target::LearnButton}
									  : Pick{}
	);
}

void SkillsScreen::MouseMotion(int x, int y) {
	float cx = 0.f;
	float cy = 0.f;
	toCanvas(world.render, x, y, cx, cy);
	scrollDrag.Move(scrollBar(), cy, scroll);
	detailDrag.Move(DetailBar(), cy, detailScroll);
	UpdateHover(cx, cy);
}

// A card is selected when the button goes down on it; Learn fires when the button comes up over it. The scroll bar
// takes a press on it: the thumb drags, the track pages.
void SkillsScreen::MouseFunction(int button, int state, int x, int y) {
	float cx = 0.f;
	float cy = 0.f;
	toCanvas(world.render, x, y, cx, cy);
	if (button == MOUSE_WHEEL_UP || button == MOUSE_WHEEL_DOWN) {
		const int step = button == MOUSE_WHEEL_UP ? -1 : 1;
		if (state == MOUSE_DOWN && DETAIL_PANEL.contains(cx, cy))
			ScrollDetail(step);
		else if (state == MOUSE_DOWN)
			scroll = std::clamp(scroll + step, 0, MAX_SCROLL);
		UpdateHover(cx, cy);
		return;
	}
	if (button != MOUSE_LEFT_BUTTON)
		return;
	UpdateHover(cx, cy);
	const Pick at = pointer.hovered;
	if (state == MOUSE_DOWN) {
		if (scrollDrag.Down(scrollBar(), cx, cy, scroll) || detailDrag.Down(DetailBar(), cx, cy, detailScroll)) {
			pointer.Down(Pick{}, button);
			UpdateHover(cx, cy);
			return;
		}
		pointer.Down(at, button);
		if (at.target == Target::Card)
			Select(at.card);
		else if (at.target == Target::None)
			confirming = false;
		return;
	}
	scrollDrag.Up();
	detailDrag.Up();
	if (pointer.Up(at, button).target == Target::LearnButton)
		PressLearn();
}

void SkillsScreen::KeyPressed(unsigned char key) { Navigate(navKeyOf(InputKey::Char(key), world.settings.controls)); }

// PgUp / PgDn page the papyrus text; the rest browse the cards.
void SkillsScreen::SpecialKeyPressed(int key) {
	if (key == SpecialKey::PAGE_UP || key == SpecialKey::PAGE_DOWN) {
		const int page = std::max(1, DetailBar().visible - 1);
		ScrollDetail(key == SpecialKey::PAGE_UP ? -page : page);
		return;
	}
	Navigate(navKeyOf(InputKey::Special(key), world.settings.controls));
}

void SkillsScreen::Navigate(NavKey key) {
	switch (key) {
	case NavKey::Activate:
		PressLearn();
		return;
	case NavKey::Left:
		MoveSelection(-1, 0);
		return;
	case NavKey::Right:
		MoveSelection(1, 0);
		return;
	case NavKey::Up:
		MoveSelection(0, 1);
		return;
	case NavKey::Down:
		MoveSelection(0, -1);
		return;
	case NavKey::None:
		return;
	}
}

// ---- drawing ---------------------------------------------------------------

void SkillsScreen::LoadFonts() { ensureScreenFonts(title, heading, body, small, 7.f, fontGeneration); }

// The effect under a card's name (plan heka-card-effect-two-lines), rank II's once upgraded (plan skill-upgrades):
// wrapped to the card's text width, from the icon well to the right edge.
std::vector<std::string> SkillsScreen::EffectLines(int card, bool rankTwo) {
	LoadFonts();
	const Rect r = cardRect(card, scroll);
	const Rect well = iconWell(r, CARD_WELL_INSET);
	const SkillDef& def = skillDef(static_cast<SkillId>(card));
	return wrap(
		small,
		rankTwo && def.rankTwo != nullptr ? def.rankTwo : def.effect,
		r.x + r.w - CARD_TEXT_PAD - (well.x + well.w + CARD_TEXT_PAD)
	);
}

int SkillsScreen::MostEffectLines() {
	size_t most = 0;
	for (int card = 0; card < SKILL_COUNT; card++)
		most = std::max({most, EffectLines(card, false).size(), EffectLines(card, true).size()});
	return static_cast<int>(most);
}

std::vector<SkillsScreen::DetailLine> SkillsScreen::DetailLines() {
	LoadFonts();
	const auto id = static_cast<SkillId>(selected);
	const SkillDef& def = skillDef(id);
	const bool deepen = heka(world).Has(id) && upgradable(id); // rank II is shown
	const float width = DETAIL_PANEL.w - 2 * DETAIL_PAD;
	std::vector<DetailLine> lines;
	float drop = 0.f;
	const std::string effect = deepen ? std::string("II: ") + def.rankTwo : def.effect;
	for (std::string& l : wrap(body, effect, width)) {
		lines.push_back({std::move(l), false, drop});
		drop += DETAIL_EFFECT_STEP;
	}
	drop += 1.f;
	for (std::string& l : wrap(small, deepen ? skillRankTwoLore(id) : skillLore(id), width)) {
		lines.push_back({std::move(l), true, drop});
		drop += DETAIL_LORE_STEP;
	}
	return lines;
}

// A line a row: the last scroll is the first line from which the rest fit.
ui::ScrollBar SkillsScreen::DetailBar() {
	const std::vector<DetailLine> lines = DetailLines();
	const int rows = static_cast<int>(lines.size());
	int last = 0;
	while (rows > 0 && lines.back().drop - lines[static_cast<size_t>(last)].drop > DETAIL_TEXT_H)
		last++;
	return {DETAIL_BAR, rows, std::max(1, rows - last)};
}

void SkillsScreen::Draw() {
	LoadFonts();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	const Rect area = visibleArea(world.render);
	beginCanvas(area);
	glDisable(GL_DEPTH_TEST);

	backdrop(area, world.assets.textures.loadingBackground.ID());
	titleBar(title, CANVAS_W / 2, "Heka", SCREEN_TABS_TITLE_REACH);
	panel(CARDS_PANEL, 0.9f);
	DrawPoints();
	for (int card = 0; card < SKILL_COUNT; card++)
		if (cardVisible(card, scroll))
			DrawCard(card);
	DrawScrollBar();
	DrawDetails();
	DrawFooter();
	ScreenTabs::Draw(ui);

	endScreen();
}

// The heka mark and the heka left to spend over the cards; the skills learnt on the right.
void SkillsScreen::DrawPoints() {
	const int free = heka(world).Free();
	skillIconAt(world, POINTS_ICON, HEKA_ICON, free > 0 ? Color{1, 1, 1} : POOR_ICON);
	char learnt[32];
	snprintf(learnt, sizeof(learnt), "Learnt %d of %d", heka(world).Learnt(), SKILL_COUNT);
	const std::string points = free > 0 ? "Heka to spend: " + std::to_string(free) : "No heka to spend";
	beginText();
	text(heading, POINTS_TEXT_X, POINTS_BASELINE, points.c_str(), free > 0 ? GOLD : LABEL_DIM);
	const float right = CARDS_PANEL.x + CARDS_PANEL.w - 3.f;
	text(small, right - small.TextWidth(learnt), POINTS_BASELINE, learnt, LABEL_DIM);
	beginShapes();
}

// Lapis once learnt, stone while it can be, a dark disabled tile while sealed (still readable). The selected one
// breathes a gold halo.
void SkillsScreen::DrawCard(int card) {
	const auto id = static_cast<SkillId>(card);
	const SkillDef& def = skillDef(id);
	const bool learnt = heka(world).Has(id);
	const bool sealed = def.sealed && !learnt; // a scenario's `learn` can learn a sealed one
	const bool isSelected = card == selected;
	const Pick pick = {Target::Card, card};
	const bool hovered = !sealed && pointer.hovered == pick;
	const bool held = hovered && pointer.Held(pick);
	const TileStyle style = learnt ? TileStyle::Lapis : (sealed ? TileStyle::Disabled : TileStyle::Stone);

	if (isSelected) {
		const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GameClock::now()) * PULSE_RATE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		ring(cardRect(card, scroll), 2.6f, GOLD, 0.35f + 0.25f * pulse, 0.f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	const Rect r = tile(cardRect(card, scroll), style, hovered, held);
	if (isSelected)
		strokeRect(r, GOLD, 1.f, 3.f);

	const Rect well = iconWell(r, CARD_WELL_INSET);
	fillRect(well, learnt ? LAPIS_DARK : WELL, learnt ? LAPIS_DARK : WELL, 1.f);
	strokeRect(well, hovered ? GOLD : GOLD_DIM, sealed ? 0.5f : 1.f, 1.f);
	const Color tint = sealed ? DIM_ICON : (learnt || heka(world).Free() > 0 ? Color{1, 1, 1} : POOR_ICON);
	skillIconAt(world, well.inset(0.4f), skillIcon(id), tint);

	const float markX = r.x + r.w - 2.4f;
	const float markY = r.y + r.h - 2.4f;
	if (learnt) // a gold seal
		diamond(markX, markY, CARD_MARK, GOLD_BRIGHT, 1.f);
	else if (sealed) // red wax
		ellipse(markX, markY, CARD_MARK, CARD_MARK, WAX, 0.8f);

	const float textX = well.x + well.w + CARD_TEXT_PAD;
	const Color nameColor = sealed ? DIM_TEXT : (hovered ? TEXT_HOVER : (learnt ? GOLD : LABEL));
	beginText();
	text(body, textX, r.y + CARD_NAME_Y, def.name, nameColor);
	// The rank (plan skill-upgrades), left of the seal; bright once a heka can deepen it.
	if (learnt && upgradable(id)) {
		const char* rank = heka(world).Rank(id) == 2 ? "II" : "I";
		text(
			small,
			markX - CARD_MARK - 1.f - small.TextWidth(rank),
			markY - 1.f,
			rank,
			heka(world).CanUpgrade(id) ? GOLD_BRIGHT : GOLD
		);
	}
	float y = r.y + CARD_EFFECT_Y;
	for (const std::string& l : EffectLines(card, heka(world).Rank(id) == 2)) {
		text(small, textX, y, l.c_str(), sealed ? DIM_TEXT : (learnt ? GOLD_DIM : LABEL_DIM));
		y -= CARD_EFFECT_STEP;
	}
	beginShapes();
}

// Right of the cards, when they do not all fit: the track over the rows shown, the thumb where they are (as the
// inventory's).
void SkillsScreen::DrawScrollBar() {
	if (MAX_SCROLL == 0)
		return;
	const ui::ScrollBar bar = scrollBar();
	fillRect(bar.track, {0.09f, 0.07f, 0.05f}, {0.06f, 0.045f, 0.03f}, 1.f);
	fillRect(bar.Thumb(scroll), scrollDrag.Dragging() ? GOLD_BRIGHT : GOLD, GOLD_DIM, 1.f);
}

// The selected skill on papyrus: its icon, name, effect and lore, what it takes, and the Learn button. A learnt
// upgradable one (plan skill-upgrades): rank II's effect and lore, and the Rank II button until it has it.
void SkillsScreen::DrawDetails() {
	const auto id = static_cast<SkillId>(selected);
	const SkillDef& def = skillDef(id);
	const int rank = heka(world).Rank(id);
	const bool learnt = rank > 0;
	const bool deepen = learnt && upgradable(id); // rank II is shown
	const bool canLearn = learnt ? heka(world).CanUpgrade(id) : heka(world).CanLearn(id);
	const Rect& p = DETAIL_PANEL;

	papyrus(world, p);
	skillIconAt(world, DETAIL_ICON, skillIcon(id), def.sealed && !learnt ? DIM_ICON : Color{1, 1, 1});
	line(p.x + DETAIL_PAD, DETAIL_RULE_Y, p.x + p.w - DETAIL_PAD, DETAIL_RULE_Y, INK_FADED, 1.f, 1.f);
	diamond(p.cx(), DETAIL_RULE_Y, 0.8f, INK_RED, 1.f);

	beginText();
	textCentered(heading, p.cx(), DETAIL_NAME_Y, def.name, INK);
	// The effect and the lore from the first line scrolled to, as many as fit over the status line.
	const ui::ScrollBar bar = DetailBar();
	detailScroll = std::clamp(detailScroll, 0, bar.MaxScroll());
	const std::vector<DetailLine> lines = DetailLines();
	const float top = lines.empty() ? 0.f : lines[static_cast<size_t>(detailScroll)].drop;
	for (size_t i = static_cast<size_t>(detailScroll); i < lines.size() && lines[i].drop - top <= DETAIL_TEXT_H; i++)
		text(
			lines[i].lore ? small : body,
			p.x + DETAIL_PAD,
			DETAIL_EFFECT_Y - (lines[i].drop - top),
			lines[i].text.c_str(),
			lines[i].lore ? INK_FADED : INK
		);
	if (bar.Shown()) { // a faded ink track, the thumb in ink
		beginShapes();
		fillRect(bar.track, INK_FADED, INK_FADED, 0.25f);
		fillRect(bar.Thumb(detailScroll), detailDrag.Dragging() ? INK : INK_FADED, INK_FADED, 1.f);
		beginText();
	}

	const bool done = rank == 2 || (learnt && !deepen); // nothing more to buy
	const char* status = rank == 2				 ? "Rank II learnt"
						 : done					 ? "Learnt"
						 : def.sealed && !learnt ? "Sealed: not in this tomb yet"
						 : !canLearn
							 ? (learnt ? "Rank I learnt. Rank II needs 1 heka" : "Needs 1 heka: slay a guardian")
						 : confirming ? "For good: no unlearning"
									  : (learnt ? "Rank I learnt. Rank II costs 1 heka" : "Costs 1 heka");
	const Color statusColor = done ? INK_GREEN
								   : (def.sealed && !learnt ? INK_FADED : (!canLearn || confirming ? INK_RED : INK));
	textCentered(small, p.cx(), DETAIL_STATUS_Y, status, statusColor);
	beginShapes();
	if (done)
		return;

	const bool hovered = canLearn && pointer.hovered.target == Target::LearnButton;
	const bool held = hovered && pointer.Held({Target::LearnButton});
	const Rect b = tile(LEARN_BUTTON, canLearn ? TileStyle::Lapis : TileStyle::PapyrusDisabled, hovered, held);
	if (confirming) {
		const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GameClock::now()) * PULSE_RATE * 2.f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		ring(b, 1.6f, GOLD, 0.2f + 0.3f * pulse, 0.f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	beginText();
	textCenteredIn(
		canLearn ? heading : body,
		b,
		learnt ? (confirming ? "Sure? Rank II" : "Rank II") : (confirming ? "Sure? Learn" : "Learn"),
		!canLearn ? INK_FADED : (hovered ? TEXT_HOVER : GOLD)
	);
	beginShapes();
}

void SkillsScreen::DrawFooter() {
	constexpr float CENTRE = CANVAS_W / 2;
	const bool paged = DetailBar().Shown();
	beginText();
	if (float alpha = toast.Alpha(GameClock::now()); alpha > 0.f)
		textCentered(body, CENTRE, 7.2f, toast.text.c_str(), TOAST, alpha);
	textCentered(
		small,
		CENTRE,
		2.2f,
		paged ? "Click: select    Enter: learn    Arrows: browse    Wheel, PgUp / PgDn: scroll    K / Esc: close"
			  : "Click: select    Enter: learn    Arrows: browse    Wheel: scroll    K / Esc: close",
		LABEL_DIM
	);
	beginShapes();
}
