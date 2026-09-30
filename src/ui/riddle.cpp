#include "riddle.h"
#include "../state/game_state.h"
#include "../core/logger.h"
#include "../core/timer.h"
#include "../input/input.h"
#include <GL/gl.h>
#include "../graphics/gl_includes.h"
#include "ui_draw.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

// Same 160 x 100 canvas (y up) as the inventory: the gate on the left, the riddle on a papyrus scroll on the right.
namespace {

using namespace ui;

constexpr Rect GATE_PANEL = {6, 15, 68, 68};
constexpr Rect SCROLL = {80, 13, 74, 72};
constexpr Rect ANSWER_BOX = {86, 22, 62, 7.5f};
constexpr float QUESTION_TOP = 65.f;
constexpr float QUESTION_STEP = 4.4f;
constexpr int MAX_QUESTION_LINES = 6;
constexpr size_t MAX_TYPED = 32;
constexpr int HINT_AFTER_MISSES = 2;
constexpr int WRONG_MS = 1600;
constexpr int SHAKE_MS = 400;
constexpr int CARET_BLINK_MS = 500;
constexpr unsigned char KEY_BACKSPACE = 8;
constexpr unsigned char KEY_DELETE = 127;

Rect visibleArea() { return ui::visibleArea(CANVAS_W, CANVAS_H, Game().render.resX, Game().render.resY); }

std::string trim(const std::string& s) {
	size_t first = s.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
		return "";
	return s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}

// Words of `text` in lines no wider than `width`; a single longer word gets a line of its own.
std::vector<std::string> wrap(const Font& font, const std::string& text, float width) {
	std::vector<std::string> lines;
	std::string line;
	size_t pos = 0;
	while (pos < text.size()) {
		size_t end = text.find(' ', pos);
		if (end == std::string::npos)
			end = text.size();
		std::string word = text.substr(pos, end - pos);
		pos = end + 1;
		if (word.empty())
			continue;
		std::string candidate = line;
		if (!candidate.empty())
			candidate += ' ';
		candidate += word;
		if (!line.empty() && font.TextWidth(candidate.c_str()) > width) {
			lines.push_back(line);
			line = word;
		} else {
			line = candidate;
		}
	}
	if (!line.empty())
		lines.push_back(line);
	return lines;
}
} // namespace

// ---- riddle files ----------------------------------------------------------

std::string NormalizeAnswer(const std::string& text) {
	std::string lower;
	for (char c : text)
		lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	lower = trim(lower);
	for (const char* article : {"a ", "an ", "the "}) {
		std::string prefix(article);
		if (lower.size() > prefix.size() && lower.compare(0, prefix.size(), prefix) == 0) {
			lower = lower.substr(prefix.size());
			break;
		}
	}
	std::string out;
	for (char c : lower)
		if (std::isalnum(static_cast<unsigned char>(c)))
			out += c;
	return out;
}

void ParseRiddles(std::istream& in, const std::string& source, std::vector<RiddleEntry>& out,
				  std::vector<std::string>& errors) {
	std::string theme = std::filesystem::path(source).stem().string();
	RiddleEntry current;
	int lineNo = 0;
	auto error = [&](const std::string& message) {
		errors.push_back(source + ":" + std::to_string(lineNo) + ": " + message);
	};
	auto finish = [&]() {
		if (!current.question.empty() && current.answers.empty())
			errors.push_back(current.source + ": riddle has no A: line, skipped");
		else if (!current.question.empty())
			out.push_back(current);
		current = RiddleEntry{};
	};

	std::string raw;
	while (std::getline(in, raw)) {
		lineNo++;
		std::string line = trim(raw);
		if (line.empty()) {
			finish();
			continue;
		}
		if (line[0] == '#')
			continue;
		// The fonts only have printable ASCII.
		for (char& c : line)
			if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) > 126) {
				error("non-ASCII character, shown as '?'");
				c = '?';
			}

		size_t colon = line.find(':');
		std::string key = colon == std::string::npos ? "" : line.substr(0, colon);
		std::string value = colon == std::string::npos ? "" : trim(line.substr(colon + 1));
		if (key == "theme") {
			finish();
			theme = value;
		} else if (key == "Q") {
			if (!current.answers.empty())
				finish();
			if (current.question.empty()) {
				current.theme = theme;
				current.source = source + ":" + std::to_string(lineNo);
			}
			current.question.push_back(value);
		} else if (key == "A" || key == "H") {
			if (current.question.empty()) {
				error(key + ": before any Q: line, ignored");
				continue;
			}
			if (key == "H") {
				current.hint = value;
				continue;
			}
			std::string normalized = NormalizeAnswer(value);
			if (normalized.empty())
				error("answer has no letters or digits, ignored");
			else if (value.size() > MAX_TYPED)
				error("answer longer than " + std::to_string(MAX_TYPED) + " characters, ignored");
			else
				current.answers.push_back(normalized);
		} else {
			error("expected theme:, Q:, A: or H:, line ignored");
		}
	}
	finish();
}

// ---- riddle screen ---------------------------------------------------------

Riddle::Riddle() {
	loadScreenFonts(title, heading, body, small, 7.f);
	Load("riddles");
}

size_t Riddle::Load(const std::string& path) {
	std::vector<std::string> files;
	std::error_code ec;
	if (std::filesystem::is_directory(path, ec)) {
		for (const auto& entry : std::filesystem::directory_iterator(path, ec))
			if (entry.path().extension() == ".txt")
				files.push_back(entry.path().string());
		std::sort(files.begin(), files.end()); // directory order is not stable across file systems
	} else {
		files.push_back(path);
	}

	riddles.clear();
	deck.clear();
	std::vector<std::string> errors;
	for (const std::string& file : files) {
		std::ifstream in(file);
		if (!in) {
			errors.push_back(file + ": cannot be read");
			continue;
		}
		ParseRiddles(in, file, riddles, errors);
	}
	for (const std::string& message : errors)
		LOG_WARNINGF("ui", "Riddles: %s", message.c_str());
	if (riddles.empty()) {
		LOG_ERRORF("ui", "No riddles found in %s, riddle gates ask a fallback one", path.c_str());
		riddles.push_back({"The Gate", {"How many hounds guard this gate?"}, {"2", "two"}, "", "built-in"});
	}
	LOG_INFO("ui", "Loaded " + std::to_string(riddles.size()) + " riddles from " + std::to_string(files.size()) +
					   " files in " + path);
	return riddles.size();
}

void Riddle::Ask() {
	if (deck.empty()) {
		// rand(), not a private generator: scenario tests seed it and get the same riddles every run.
		for (size_t i = 0; i < riddles.size(); i++)
			deck.push_back(i);
		for (size_t i = deck.size(); i > 1; i--)
			std::swap(deck[i - 1], deck[static_cast<size_t>(rand()) % i]);
		// A fresh deck should not start with the riddle that was just asked.
		if (deck.size() > 1 && deck.back() == selected)
			std::swap(deck.front(), deck.back());
	}
	selected = deck.back();
	deck.pop_back();
	answer.clear();
	misses = 0;
	wrongAtMs = -100000;
}

bool Riddle::CheckAnswer() const {
	const std::vector<std::string>& accepted = riddles[selected].answers;
	return std::find(accepted.begin(), accepted.end(), NormalizeAnswer(answer)) != accepted.end();
}

void Riddle::KeyboardF(unsigned char key, int mouseX, int mouseY) {
	(void)mouseX;
	(void)mouseY;

	if (key == KEY_ESCAPE) { // walk away: the gate stays open, the reward is lost
		Game().ui.screen = Screen::Gameplay;
		Game().ShowStatus("The riddle stays unanswered");
	} else if (key == KEY_ENTER) {
		if (NormalizeAnswer(answer).empty())
			return;
		if (CheckAnswer()) {
			Game().ui.screen = Screen::Gameplay;
			int xp = Game().player->stats.RiddleXP();
			Game().ShowStatus("Riddle answered, got %d XP", xp);
			Game().player->stats.AddXP(xp);
		} else {
			misses++;
			wrongAtMs = GameClock::now();
			answer.clear();
		}
	} else if (key == KEY_BACKSPACE || key == KEY_DELETE) {
		if (!answer.empty())
			answer.pop_back();
	} else if (key >= 32 && key <= 126 && answer.size() < MAX_TYPED) {
		answer += static_cast<char>(key);
	}
}

// ---- drawing ---------------------------------------------------------------

void Riddle::Draw() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	Rect area = visibleArea();
	glOrtho(area.x, area.x + area.w, area.y, area.y + area.h, -200, 200);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);

	DrawBackground();
	DrawScroll();
	DrawAnswer();

	beginText();
	textCentered(small, CANVAS_W / 2, 2.2f,
				 "Type the answer    Enter: answer    Backspace: erase    Esc: walk away (no reward)",
				 {0.55f, 0.45f, 0.30f});

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glColor3f(1, 1, 1);
}

void Riddle::DrawBackground() {
	backdrop(visibleArea(), Game().assets.textures.loadingBackground.ID());
	titleBar(title, CANVAS_W / 2, "Riddle of the Gate", 72.f);

	// The two hounds at the gate; the empty black bottom of the render is cropped off.
	fillRect({GATE_PANEL.x + 0.8f, GATE_PANEL.y - 1.f, GATE_PANEL.w, GATE_PANEL.h}, BLACK, BLACK, 0.45f);
	texturedRect(GATE_PANEL, Game().assets.textures.riddleBackground.ID(), {1, 1, 1}, 0.12f, 0.25f, 0.88f, 1.f);
	beginShapes();
	ring(GATE_PANEL.inset(8.f), 8.f, BLACK, 0.f, 0.5f);
	strokeRect(GATE_PANEL, BRONZE, 1.f, 3.f);
	strokeRect(GATE_PANEL.inset(1.1f), GOLD_DIM, 0.8f, 1.f);
	cornerStuds(GATE_PANEL);

	// Papyrus scroll for the riddle, framed like the inventory details.
	texturedRect(SCROLL, Game().assets.textures.papyrus.ID(), {1, 1, 1}, 0.04f, 0.07f, 0.96f, 0.93f);
	beginShapes();
	strokeRect(SCROLL, BRONZE, 1.f, 3.f);
	cornerStuds(SCROLL);
}

void Riddle::DrawScroll() {
	const RiddleEntry& r = riddles[selected];
	float cx = SCROLL.cx();

	beginText();
	textCentered(heading, cx, 77.f, r.theme.c_str(), INK);
	char reward[32];
	snprintf(reward, sizeof(reward), "Reward %d XP", Game().player->stats.RiddleXP());
	textCentered(small, cx, 72.5f, reward, INK_RED);

	std::vector<std::string> lines;
	for (const std::string& q : r.question)
		for (const std::string& l : wrap(body, q, SCROLL.w - 10))
			lines.push_back(l);
	if (lines.size() > MAX_QUESTION_LINES)
		lines.resize(MAX_QUESTION_LINES);
	// Short riddles sit in the middle of the space above the hint, long ones start at the top.
	float y = QUESTION_TOP - static_cast<float>(MAX_QUESTION_LINES - lines.size()) * QUESTION_STEP / 2;
	for (const std::string& l : lines) {
		textCentered(body, cx, y, l.c_str(), INK);
		y -= QUESTION_STEP;
	}

	if (misses >= HINT_AFTER_MISSES) {
		std::string hint = r.hint;
		if (hint.empty())
			hint = "the answer has " + std::to_string(r.answers.front().size()) + " characters";
		std::vector<std::string> hintLines = wrap(small, "Hint: " + hint, SCROLL.w - 12);
		float hy = 35.f + (hintLines.size() > 1 ? 1.7f : 0.f);
		for (size_t i = 0; i < hintLines.size() && i < 2; i++)
			textCentered(small, cx, hy - 3.4f * static_cast<float>(i), hintLines[i].c_str(), INK_FADED);
	}

	beginShapes();
	line(SCROLL.x + 8, 70.5f, SCROLL.x + SCROLL.w - 8, 70.5f, INK_FADED, 0.8f, 1.f);
	diamond(cx, 70.5f, 0.6f, INK_RED, 1.f);
	line(SCROLL.x + 8, 40.2f, SCROLL.x + SCROLL.w - 8, 40.2f, INK_FADED, 0.5f, 1.f);
}

void Riddle::DrawAnswer() {
	int age = GameClock::now() - wrongAtMs;
	bool wrong = age >= 0 && age < WRONG_MS;
	float shake = 0.f;
	if (age >= 0 && age < SHAKE_MS)
		shake = std::sin(static_cast<float>(age) * 0.07f) * 1.2f *
				(1.f - static_cast<float>(age) / static_cast<float>(SHAKE_MS));
	Rect box = ANSWER_BOX;
	box.x += shake;

	// Dark ink well with a gold rim, glowing red for a moment after a wrong answer.
	if (wrong) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		ring(box, 2.f, INK_RED, 0.5f * (1.f - static_cast<float>(age) / static_cast<float>(WRONG_MS)), 0.f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	fillRect({box.x + 0.5f, box.y - 0.7f, box.w, box.h}, BLACK, BLACK, 0.3f);
	fillRect(box, PANEL_BOTTOM, PANEL_TOP, 1.f);
	strokeRect(box, wrong ? INK_RED : GOLD, 1.f, 2.f);
	strokeRect(box.inset(0.8f), GOLD_DIM, 0.6f, 1.f);

	beginText();
	text(small, box.x, box.y + box.h + 1.2f, "Your answer", INK_FADED);
	float textX = box.x + 2.5f;
	float textY = box.y + 2.f;
	text(body, textX, textY, answer.c_str(), GOLD_BRIGHT);
	if ((GameClock::now() / CARET_BLINK_MS) % 2 == 0)
		text(body, textX + body.TextWidth(answer.c_str()) + 0.3f, textY, "_", GOLD);
	if (wrong) {
		float alpha =
			age > WRONG_MS / 2 ? static_cast<float>(WRONG_MS - age) / (static_cast<float>(WRONG_MS) / 2) : 1.f;
		textCentered(body, SCROLL.cx(), 16.f, "Wrong. The hounds stay silent.", INK_RED, alpha);
	}
	beginShapes();
}
