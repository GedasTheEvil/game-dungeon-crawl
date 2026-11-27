#include "menu.h"
#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include "../../level/ui/pad_layout.h"
#include "menu_internal.h"

// Options > Gamepad (plan gamepad-options-tab): a flat drawing of the pad, each button's action on a label at the
// end of a line from it. A label steps its button through the actions; the stick rows and the reset sit under it.

namespace {
using namespace menu_internal;
using namespace PadLayout;

constexpr Color PAD_FILL = {0.13f, 0.10f, 0.075f};
constexpr Color PART_FILL = {0.07f, 0.055f, 0.04f};
constexpr float OUTLINE = 0.5f; // the gold edge round the body and the parts

// The body: rects and ellipses drawn as one shape (all of them grown by the outline in gold first, then all filled).
struct Blob {
	bool round;
	float x, y, a, b; // a rect's x, y, w, h; an ellipse's centre and radii
};
constexpr std::array<Blob, 6> BODY = {{
	{false, 66.f, 44.f, 28.f, 16.f},
	{true, 66.f, 53.f, 12.f, 7.5f},
	{true, 94.f, 53.f, 12.f, 7.5f},
	{true, 59.5f, 44.5f, 6.5f, 9.5f},
	{true, 100.5f, 44.5f, 6.5f, 9.5f},
	{true, CX, 46.f, 15.f, 7.f},
}};

void drawBlob(const Blob& blob, float grow, Color c) {
	if (blob.round)
		disc(blob.x, blob.y, blob.a + grow, blob.b + grow, c, 1.f);
	else
		fillRect({blob.x - grow, blob.y - grow, blob.a + 2 * grow, blob.b + 2 * grow}, c, c, 1.f);
}

void drawBody() {
	for (const Blob& blob : BODY)
		drawBlob(blob, OUTLINE, GOLD_DIM);
	for (const Blob& blob : BODY)
		drawBlob(blob, 0.f, PAD_FILL);
}

// A trigger or a shoulder button: a dark key with a gold edge, its cap in it.
void drawTopButton(const Rect& r) {
	fillRect(r, PAD_FILL, PART_FILL, 1.f);
	strokeRect(r, GOLD_DIM, 1.f, 1.f);
}

void drawRound(Point p, float r) {
	disc(p.x, p.y, r + OUTLINE * 0.7f, r + OUTLINE * 0.7f, GOLD_DIM, 1.f);
	disc(p.x, p.y, r, r, PART_FILL, 1.f);
}

void drawDpad() {
	const float reach = DPAD_ARM + DPAD_ARM_W / 2;
	const Rect across = {DPAD.x - reach, DPAD.y - DPAD_ARM_W / 2, 2 * reach, DPAD_ARM_W};
	const Rect down = {DPAD.x - DPAD_ARM_W / 2, DPAD.y - reach, DPAD_ARM_W, 2 * reach};
	const float o = OUTLINE * 0.7f;
	fillRect({across.x - o, across.y - o, across.w + 2 * o, across.h + 2 * o}, GOLD_DIM, GOLD_DIM, 1.f);
	fillRect({down.x - o, down.y - o, down.w + 2 * o, down.h + 2 * o}, GOLD_DIM, GOLD_DIM, 1.f);
	fillRect(across, PART_FILL, PART_FILL, 1.f);
	fillRect(down, PART_FILL, PART_FILL, 1.f);
}

void drawStick(Point p) {
	drawRound(p, STICK_R);
	disc(p.x, p.y, STICK_R * 0.62f, STICK_R * 0.62f, GOLD_DIM, 1.f);
	disc(p.x, p.y, STICK_R * 0.55f, STICK_R * 0.55f, PAD_FILL, 1.f);
}

Rect around(Point p, float half) { return {p.x - half, p.y - half, 2 * half, 2 * half}; }
} // namespace

void MainMenu::DrawGamepad() {
	beginShapes();
	drawBody();
	// The lines first: they come out from under the parts. The hovered one bright.
	const int hoveredButton = IsPadTarget(pointer.hovered) ? pointer.hovered - PAD_BASE : NONE;
	for (const Callout& c : callouts()) {
		const bool lit = c.Bindable() && c.button == hoveredButton;
		const Color colour = lit ? GOLD_BRIGHT : c.Bindable() ? GOLD_DIM : LABEL_DIM;
		const float alpha = c.Bindable() ? 0.9f : 0.6f;
		const float width = lit ? 1.5f : 1.f;
		const Point end = lineEnd(c);
		Point from = c.anchor;
		if (c.elbow) {
			line(from.x, from.y, from.x, end.y, colour, alpha, width);
			from.y = end.y;
		}
		line(from.x, from.y, end.x, end.y, colour, alpha, width);
	}
	drawTopButton(LEFT_TRIGGER);
	drawTopButton(RIGHT_TRIGGER);
	drawTopButton(LEFT_SHOULDER);
	drawTopButton(RIGHT_SHOULDER);
	drawDpad();
	for (const Callout& c : callouts())
		if (c.button >= PadButton::SOUTH && c.button <= PadButton::NORTH)
			drawRound(c.anchor, FACE_R);
	drawRound(PadLayout::BACK_BUTTON, SMALL_BUTTON_R);
	drawRound(START_BUTTON, SMALL_BUTTON_R);
	drawStick(LEFT_STICK);
	drawStick(RIGHT_STICK);
	if (const Callout* lit = hoveredButton == NONE ? nullptr : calloutOf(hoveredButton))
		ellipse(lit->anchor.x, lit->anchor.y, 3.5f, 3.5f, GOLD_BRIGHT, 0.55f);

	// The caps on the parts big enough for them: the face buttons (✕ ○ □ △ in their colours, or A B X Y), the
	// shoulders and the triggers.
	beginText();
	for (const int button :
		 {PadButton::SOUTH,
		  PadButton::EAST,
		  PadButton::WEST,
		  PadButton::NORTH,
		  PadButton::LEFT_SHOULDER,
		  PadButton::RIGHT_SHOULDER,
		  PadButton::LEFT_TRIGGER,
		  PadButton::RIGHT_TRIGGER}) {
		const Callout* c = calloutOf(button);
		const std::string cap = keyCap(InputKey::Pad(button), world.padFamily);
		textCenteredIn(small, around(c->anchor, 1.5f), cap.c_str(), button == hoveredButton ? GOLD_BRIGHT : GOLD);
	}
	beginShapes();

	const auto all = callouts();
	for (int i = 0; i < static_cast<int>(all.size()); i++)
		DrawPadLabel(i);

	const bool resetHovered = pointer.hovered == RESET_PAD;
	const Rect reset = tile(padResetButton(), TileStyle::Stone, resetHovered, pointer.Held(RESET_PAD));
	beginText();
	textCenteredIn(body, reset, "Reset pad to defaults", resetHovered ? GOLD_BRIGHT : LABEL);
	beginShapes();
}

// A bindable button's label is a cell like the Controls tab's keys: its cap by the drawing, its action ("-": none)
// outside. A fixed part's is dim text, no cell.
void MainMenu::DrawPadLabel(int index) {
	const Callout& c = callouts()[static_cast<size_t>(index)];
	const Rect r = labelRect(c);
	const int target = PAD_BASE + c.button;
	const bool isHovered = c.Bindable() && pointer.hovered == target;
	const std::string cap = c.button == NO_BUTTON ? "" : keyCap(InputKey::Pad(c.button), world.padFamily);
	const float capW = std::max(7.f, small.TextWidth(cap.c_str()) + 2.f); // "Options", "Share"
	const Rect capBox = {c.right ? r.x : r.x + r.w - capW, r.y, capW, r.h};

	std::string action;
	if (c.Bindable()) {
		const std::optional<BindAction> bound = world.settings.controls.ActionFor(InputKey::Pad(c.button));
		action = bound ? bindActionLabel(*bound) : "-";
		const bool held = pointer.Held(target);
		const Rect cell = held ? Rect{r.x, r.y - TILE_SINK, r.w, r.h} : r;
		if (!held)
			fillRect({r.x + 0.4f, r.y - 0.5f, r.w, r.h}, BLACK, BLACK, 0.5f);
		fillRect(cell, {0.19f, 0.15f, 0.10f}, {0.10f, 0.08f, 0.055f}, 1.f);
		strokeRect(cell, isHovered ? GOLD : GOLD_DIM, 1.f, isHovered ? 1.5f : 1.f);
		line(
			c.right ? capBox.x + capBox.w : capBox.x,
			cell.y + 0.4f,
			c.right ? capBox.x + capBox.w : capBox.x,
			cell.y + cell.h - 0.4f,
			GOLD_DIM,
			0.6f,
			1.f
		);
	} else
		action = c.fixed;

	beginText();
	textCenteredIn(small, capBox, cap.c_str(), c.Bindable() ? GOLD : LABEL_DIM);
	const Color colour = !c.Bindable() ? LABEL_DIM : action == "-" ? LABEL_DIM : isHovered ? GOLD_BRIGHT : LABEL;
	if (c.right)
		textMiddle(small, capBox.x + capBox.w + 1.5f, r.cy(), action.c_str(), colour);
	else
		textMiddle(small, capBox.x - 1.5f - small.TextWidth(action.c_str()), r.cy(), action.c_str(), colour);
	beginShapes();
}

void MainMenu::StepPad(int button, int step) {
	Bindings& controls = world.settings.controls;
	const InputKey key = InputKey::Pad(button);
	if (padStepping != button) {
		padStepping = button;
		for (int a = 0; a < BIND_ACTION_COUNT; a++)
			padBefore[static_cast<size_t>(a)] = controls.Of(static_cast<BindAction>(a)).pad;
	}
	// 0 is none, then the actions in their order, round and round.
	constexpr int CHOICES = BIND_ACTION_COUNT + 1;
	const std::optional<BindAction> now = controls.ActionFor(key);
	const int at = now ? std::to_underlying(*now) + 1 : 0;
	const int next = (at + step + CHOICES) % CHOICES;
	for (int a = 0; a < BIND_ACTION_COUNT; a++) {
		const auto action = static_cast<BindAction>(a);
		Binding binding = controls.Of(action);
		const InputKey& before = padBefore[static_cast<size_t>(a)];
		binding.pad = before == key ? InputKey{} : before;
		controls.Set(action, binding);
	}
	if (next > 0) {
		const auto action = static_cast<BindAction>(next - 1);
		const InputKey taken = controls.Of(action).pad;
		controls.Bind(action, PAD_SLOT, key);
		if (taken.Bound())
			ShowToast(keyCap(taken, world.padFamily) + " is free now");
	}
	world.applySettings(true);
}

void MainMenu::ClearPad(int button) {
	padStepping = NONE;
	if (const std::optional<BindAction> action = world.settings.controls.ActionFor(InputKey::Pad(button))) {
		world.settings.controls.Clear(*action, PAD_SLOT);
		world.applySettings(true);
	}
}

void MainMenu::ResetPad() {
	padStepping = NONE;
	const Bindings defaults;
	for (int a = 0; a < BIND_ACTION_COUNT; a++) {
		const auto action = static_cast<BindAction>(a);
		Binding binding = world.settings.controls.Of(action);
		binding.pad = defaults.Of(action).pad;
		world.settings.controls.Set(action, binding);
	}
	world.applySettings(true);
	ShowToast("Pad buttons reset to the defaults");
}
