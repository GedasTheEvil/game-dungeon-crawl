#include "skills_view.h"
#include "skills_layout.h"
#include "../core/timer.h"
#include "../graphics/gl_includes.h"
#include "../input/input.h"
#include "../state/game_state.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <string>

using namespace ui;
using namespace SkillsLayout;

namespace {
// ---- layout ----
constexpr Rect POINTS_ICON = {7.f, 76.6f, 5.6f, 5.6f};
constexpr float POINTS_TEXT_X = 14.f;
constexpr float CARD_WELL_INSET = 1.f;
constexpr float CARD_NAME_Y = 7.4f; // baselines above the card's bottom
constexpr float CARD_EFFECT_Y = 2.8f;
constexpr float CARD_MARK = 1.2f; // the learnt / sealed mark in the top right corner
constexpr Rect DETAIL_ICON = {DETAIL_PANEL.x + DETAIL_PANEL.w / 2 - 10.f, 60.f, 20.f, 20.f};
constexpr float DETAIL_NAME_Y = 54.f;
constexpr float DETAIL_RULE_Y = 51.6f;
constexpr float DETAIL_PAD = 4.f;
constexpr float DETAIL_EFFECT_Y = 46.8f;
constexpr float DETAIL_EFFECT_STEP = 4.4f;
constexpr float DETAIL_LORE_STEP = 3.6f;
constexpr float DETAIL_STATUS_Y = 26.f;

constexpr Color DIM_TEXT = {0.40f, 0.33f, 0.23f}; // a sealed card
constexpr Color DIM_ICON = {0.35f, 0.30f, 0.25f};
constexpr Color POOR_ICON = {0.62f, 0.56f, 0.50f}; // no heka to learn it
constexpr Color WAX = {0.62f, 0.17f, 0.08f};
constexpr Color TOAST = {1.f, 0.9f, 0.6f};
constexpr float PULSE_RATE = 0.004f; // radians per ms

Rect visibleArea() { return ui::visibleArea(CANVAS_W, CANVAS_H, Game().render.resX, Game().render.resY); }

void toCanvas(int mouseX, int mouseY, float& x, float& y) {
	ui::toCanvas(visibleArea(), Game().render.resX, Game().render.resY, mouseX, mouseY, x, y);
}

const Heka& heka() { return Game().player->stats.heka; }

void skillIconAt(const Rect& r, int cell, Color tint) {
	atlasIcon(r, Game().assets.textures.skillIcons.ID(), cell, SKILL_ICON_COLUMNS, SKILL_ICON_ROWS, tint);
	beginShapes();
}

// The papyrus scroll of the details (docs/ui.md, Papyrus scroll).
void papyrus(const Rect& r) {
	texturedRect(r, Game().assets.textures.papyrus.ID(), {1, 1, 1}, 0.04f, 0.07f, 0.96f, 0.93f);
	beginShapes();
	strokeRect(r, BRONZE, 1.f, 3.f);
	cornerStuds(r);
}
} // namespace

// ---- input -----------------------------------------------------------------

void SkillsScreen::Select(int card) {
	if (card != selected)
		confirming = false;
	selected = std::clamp(card, 0, SKILL_COUNT - 1);
}

void SkillsScreen::MoveSelection(int dx, int dy) {
	const int column = selected % COLUMNS + dx;
	const int row = selected / COLUMNS - dy; // up is the row before
	if (column < 0 || column >= COLUMNS || row < 0 || row >= ROWS || row * COLUMNS + column >= SKILL_COUNT)
		return;
	Select(row * COLUMNS + column);
}

// Learn: the first press asks, the second learns. Refused (learnt, sealed, no heka): the toast tells why.
void SkillsScreen::PressLearn() {
	const auto id = static_cast<SkillId>(selected);
	const SkillDef& def = skillDef(id);
	Heka& points = Game().player->stats.heka;
	const int now = GameClock::now();
	if (points.Has(id))
		return;
	if (!points.CanLearn(id)) {
		toast.Show(def.sealed ? "Its heka sleeps: not in this tomb yet" : "No heka to spend: slay a guardian", now);
		return;
	}
	if (!confirming) {
		confirming = true;
		return;
	}
	confirming = false;
	points.Learn(id);
	if (const std::optional<FieldNote> note = skillNote(id))
		Game().journal.LearnNote(*note);
	std::string learnt = std::string("Learnt: ") + def.name;
	if (id == SkillId::Hieroglyphs) {
		const std::vector<Boon> boons = Game().journal.TranslateInscriptions();
		const int copies = static_cast<int>(boons.size());
		if (copies > 0) {
			// Every copy gives the full XP at the level now, no penalty for learning late (plan inscription-xp).
			const int xp = copies * Game().player->stats.InscriptionXP();
			learnt += ". " + std::to_string(copies) + (copies == 1 ? " inscription" : " inscriptions") +
					  " translated, +" + std::to_string(xp) + " XP";
			for (const Boon& b : boons) // plan inscription-boons
				if (b.Any())
					learnt += ", " + Game().player->stats.Grant(b);
			Game().player->stats.AddXP(xp, Game().events);
		}
	}
	Game().assets.sounds.amulet_s.Play();
	toast.Show(learnt, now);
}

void SkillsScreen::UpdateHover(float x, float y) {
	const int card = cardAt(x, y);
	pointer.Move(
		card >= 0					  ? Pick{Target::Card, card}
		: LEARN_BUTTON.contains(x, y) ? Pick{Target::LearnButton}
									  : Pick{}
	);
}

void SkillsScreen::MouseMotion(int x, int y) {
	float cx = 0.f;
	float cy = 0.f;
	toCanvas(x, y, cx, cy);
	UpdateHover(cx, cy);
}

// A card is selected when the button goes down on it; Learn fires when the button comes up over it.
void SkillsScreen::MouseFunction(int button, int state, int x, int y) {
	if (button != MOUSE_LEFT_BUTTON)
		return;
	float cx = 0.f;
	float cy = 0.f;
	toCanvas(x, y, cx, cy);
	UpdateHover(cx, cy);
	const Pick at = pointer.hovered;
	if (state == GLUT_DOWN) {
		pointer.Down(at, button);
		if (at.target == Target::Card)
			Select(at.card);
		else if (at.target == Target::None)
			confirming = false;
		return;
	}
	if (pointer.Up(at, button).target == Target::LearnButton)
		PressLearn();
}

void SkillsScreen::KeyPressed(unsigned char key) { Navigate(navKeyOf(InputKey::Char(key), Game().settings.controls)); }

void SkillsScreen::SpecialKeyPressed(int key) { Navigate(navKeyOf(InputKey::Special(key), Game().settings.controls)); }

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

void SkillsScreen::Draw() {
	if (!fontsLoaded) {
		loadScreenFonts(title, heading, body, small, 7.f);
		fontsLoaded = true;
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	const Rect area = visibleArea();
	glOrtho(area.x, area.x + area.w, area.y, area.y + area.h, -200, 200);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);

	backdrop(area, Game().assets.textures.loadingBackground.ID());
	titleBar(title, CANVAS_W / 2, "Heka", SCREEN_TABS_TITLE_REACH);
	panel(CARDS_PANEL, 0.9f);
	DrawPoints();
	for (int card = 0; card < SKILL_COUNT; card++)
		DrawCard(card);
	DrawDetails();
	DrawFooter();
	ScreenTabs::Draw();

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glColor3f(1, 1, 1);
}

// The heka mark and the heka left to spend over the cards; the skills learnt on the right.
void SkillsScreen::DrawPoints() {
	const int free = heka().Free();
	skillIconAt(POINTS_ICON, HEKA_ICON, free > 0 ? Color{1, 1, 1} : POOR_ICON);
	char learnt[32];
	snprintf(learnt, sizeof(learnt), "Learnt %d of %d", heka().Spent(), SKILL_COUNT);
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
	const bool learnt = heka().Has(id);
	const bool sealed = def.sealed && !learnt; // a scenario's `learn` can learn a sealed one
	const bool isSelected = card == selected;
	const Pick pick = {Target::Card, card};
	const bool hovered = !sealed && pointer.hovered == pick;
	const bool held = hovered && pointer.Held(pick);
	const TileStyle style = learnt ? TileStyle::Lapis : (sealed ? TileStyle::Disabled : TileStyle::Stone);

	if (isSelected) {
		const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GameClock::now()) * PULSE_RATE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		ring(cardRect(card), 2.6f, GOLD, 0.35f + 0.25f * pulse, 0.f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	const Rect r = tile(cardRect(card), style, hovered, held);
	if (isSelected)
		strokeRect(r, GOLD, 1.f, 3.f);

	const Rect well = iconWell(r, CARD_WELL_INSET);
	fillRect(well, learnt ? LAPIS_DARK : WELL, learnt ? LAPIS_DARK : WELL, 1.f);
	strokeRect(well, hovered ? GOLD : GOLD_DIM, sealed ? 0.5f : 1.f, 1.f);
	const Color tint = sealed ? DIM_ICON : (learnt || heka().Free() > 0 ? Color{1, 1, 1} : POOR_ICON);
	skillIconAt(well.inset(0.4f), skillIcon(id), tint);

	const float markX = r.x + r.w - 2.4f;
	const float markY = r.y + r.h - 2.4f;
	if (learnt) // a gold seal
		diamond(markX, markY, CARD_MARK, GOLD_BRIGHT, 1.f);
	else if (sealed) // red wax
		ellipse(markX, markY, CARD_MARK, CARD_MARK, WAX, 0.8f);

	const float textX = well.x + well.w + 1.8f;
	const Color nameColor = sealed ? DIM_TEXT : (hovered ? TEXT_HOVER : (learnt ? GOLD : LABEL));
	beginText();
	text(body, textX, r.y + CARD_NAME_Y, def.name, nameColor);
	text(small, textX, r.y + CARD_EFFECT_Y, def.effect, sealed ? DIM_TEXT : (learnt ? GOLD_DIM : LABEL_DIM));
	beginShapes();
}

// The selected skill on papyrus: its icon, name, effect and lore, what it takes, and the Learn button.
void SkillsScreen::DrawDetails() {
	const auto id = static_cast<SkillId>(selected);
	const SkillDef& def = skillDef(id);
	const bool learnt = heka().Has(id);
	const bool canLearn = heka().CanLearn(id);
	const Rect& p = DETAIL_PANEL;

	papyrus(p);
	skillIconAt(DETAIL_ICON, skillIcon(id), def.sealed && !learnt ? DIM_ICON : Color{1, 1, 1});
	line(p.x + DETAIL_PAD, DETAIL_RULE_Y, p.x + p.w - DETAIL_PAD, DETAIL_RULE_Y, INK_FADED, 1.f, 1.f);
	diamond(p.cx(), DETAIL_RULE_Y, 0.8f, INK_RED, 1.f);

	beginText();
	textCentered(heading, p.cx(), DETAIL_NAME_Y, def.name, INK);
	float y = DETAIL_EFFECT_Y;
	for (const std::string& l : wrap(body, def.effect, p.w - 2 * DETAIL_PAD)) {
		text(body, p.x + DETAIL_PAD, y, l.c_str(), INK);
		y -= DETAIL_EFFECT_STEP;
	}
	y -= 1.f;
	for (const std::string& l : wrap(small, skillLore(id), p.w - 2 * DETAIL_PAD)) {
		text(small, p.x + DETAIL_PAD, y, l.c_str(), INK_FADED);
		y -= DETAIL_LORE_STEP;
	}

	const char* status = learnt		  ? "Learnt"
						 : def.sealed ? "Sealed: not in this tomb yet"
						 : !canLearn  ? "Needs 1 heka: slay a guardian"
						 : confirming ? "For good: no unlearning"
									  : "Costs 1 heka";
	const Color statusColor = learnt ? INK_GREEN : (def.sealed ? INK_FADED : (!canLearn || confirming ? INK_RED : INK));
	textCentered(small, p.cx(), DETAIL_STATUS_Y, status, statusColor);
	beginShapes();
	if (learnt)
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
		confirming ? "Sure? Learn" : "Learn",
		!canLearn ? INK_FADED : (hovered ? TEXT_HOVER : GOLD)
	);
	beginShapes();
}

void SkillsScreen::DrawFooter() {
	constexpr float CENTRE = CANVAS_W / 2;
	beginText();
	if (float alpha = toast.Alpha(GameClock::now()); alpha > 0.f)
		textCentered(body, CENTRE, 7.2f, toast.text.c_str(), TOAST, alpha);
	textCentered(small, CENTRE, 2.2f, "Click: select    Enter: learn    Arrows: browse    K / Esc: close", LABEL_DIM);
	beginShapes();
}
