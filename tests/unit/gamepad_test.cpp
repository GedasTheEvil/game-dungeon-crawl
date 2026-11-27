// Plan sdl3-gamepad: the pad's bindings, its [gamepad] section, its key caps, the sticks and the PlayStation glyphs.
#include "../../external/doctest/doctest.h"
#include "../../src/level/input/bindings.h"
#include "../../src/level/input/pad_stick.h"
#include "../../src/level/files/settings_ini.h"
#include "../../src/render/glyphs.h"
#include <fstream>
#include <sstream>

namespace {
Settings parse(const std::string& text, std::vector<std::string>& warnings) {
	std::istringstream in(text);
	return parseSettings(in, warnings);
}
} // namespace

TEST_CASE("gamepad: the pad buttons' names read back, the Xbox names by place") {
	for (int button = 0; button < PadButton::COUNT; button++) {
		const InputKey key = InputKey::Pad(button);
		const std::string name = keyName(key);
		if (button == 5) { // Guide: the system's, no name
			CHECK(name.empty());
			continue;
		}
		CHECK(name.starts_with("pad_"));
		CHECK(parseKeyName(name) == key);
	}
	CHECK(parseKeyName("PAD_A") == InputKey::Pad(PadButton::SOUTH));
	CHECK(keyCap(InputKey::Pad(PadButton::SOUTH)) == "A");
	CHECK(keyCap(InputKey::Pad(PadButton::SOUTH), Caps::PlayStation) == std::string(1, PadGlyph::CROSS));
	CHECK(keyCap(InputKey::Pad(PadButton::NORTH), Caps::PlayStation) == std::string(1, PadGlyph::TRIANGLE));
	CHECK(keyCap(InputKey::Pad(PadButton::LEFT_TRIGGER), Caps::Xbox) == "LT");
	CHECK(keyCap(InputKey::Pad(PadButton::LEFT_TRIGGER), Caps::PlayStation) == "L2");
	CHECK(isReservedKey(InputKey::Pad(PadButton::START)));
	CHECK_FALSE(isReservedKey(InputKey::Pad(PadButton::BACK)));
}

TEST_CASE("gamepad: the default pad buttons, one an action, none twice, Start free") {
	const Bindings controls;
	CHECK(controls.Of(BindAction::Jump).pad == InputKey::Pad(PadButton::SOUTH));
	CHECK(controls.Of(BindAction::Attack).pad == InputKey::Pad(PadButton::WEST));
	CHECK(controls.ActionFor(InputKey::Pad(PadButton::BACK)) == BindAction::Inventory);
	CHECK_FALSE(controls.ActionFor(InputKey::Pad(PadButton::START)));
	Bindings copy = controls;
	CHECK(copy.RemoveDuplicates() == 0);
	// The walk and the look are the sticks': no pad button.
	CHECK_FALSE(controls.Of(BindAction::MoveLeft).pad.Bound());
	CHECK_FALSE(controls.Of(BindAction::LookUp).pad.Bound());
}

TEST_CASE("gamepad: a pad button binds into the pad slot only, and moves from another action") {
	Bindings controls;
	CHECK(fitsSlot(InputKey::Pad(PadButton::EAST), PAD_SLOT));
	CHECK_FALSE(fitsSlot(InputKey::Pad(PadButton::EAST), 0));
	CHECK_FALSE(fitsSlot(InputKey::Char('a'), PAD_SLOT));
	CHECK(controls.Bind(BindAction::Drag, PAD_SLOT, InputKey::Pad(PadButton::EAST)) == BindAction::Interact);
	CHECK_FALSE(controls.Of(BindAction::Interact).pad.Bound());
	CHECK(controls.ActionFor(InputKey::Pad(PadButton::EAST)) == BindAction::Drag);
}

TEST_CASE("gamepad: the HUD key caps follow the pad, empty when its button is unbound") {
	Bindings controls;
	CHECK(keyCapOf(controls, BindAction::QuickHeal) == "3");
	CHECK(keyCapOf(controls, BindAction::QuickHeal, Caps::Xbox) == std::string{'D', PadGlyph::ARROW_UP});
	CHECK(keyCapOf(controls, BindAction::Jump, Caps::PlayStation) == std::string(1, PadGlyph::CROSS));
	CHECK(keyCapOf(controls, BindAction::LookLeft, Caps::Xbox).empty());
	CHECK(equipKeysCap(controls) == "1-2");
	CHECK(equipKeysCap(controls, Caps::Xbox) == "LB RB");
	CHECK(equipKeysCap(controls, Caps::PlayStation) == "L1 R1");
}

TEST_CASE("gamepad: [controls] keeps the pad buttons, [gamepad] sets them; old files keep the defaults") {
	std::vector<std::string> warnings;
	// A file from before the pad: its [controls] lines leave the pad buttons as they were.
	Settings old = parse("[controls]\njump = space\n", warnings);
	CHECK(warnings.empty());
	CHECK(old.controls.Of(BindAction::Jump).pad == InputKey::Pad(PadButton::SOUTH));
	CHECK_FALSE(old.controls.Of(BindAction::Jump).mouse.Bound());

	Settings s = parse(
		"[gamepad]\nlook_speed = 150\ndead_zone = 10\ninvert_y = on\njump = none\ndrag = pad_rt\n",
		warnings
	);
	CHECK(warnings.size() == 1); // pad_rt is the stamina skills' too
	CHECK(s.gamepad.lookSpeed == 150);
	CHECK(s.gamepad.deadZone == 10);
	CHECK(s.gamepad.invertY);
	CHECK_FALSE(s.controls.Of(BindAction::Jump).pad.Bound());
	CHECK(s.controls.Of(BindAction::Drag).pad == InputKey::Pad(PadButton::RIGHT_TRIGGER));
	CHECK(s.controls.Of(BindAction::Jump).keys[0] == InputKey::Char(' ')); // the keys untouched
	// pad_rt was the stamina skills' too: the first action in the list keeps it.
	CHECK(s.controls.ActionFor(InputKey::Pad(PadButton::RIGHT_TRIGGER)) == BindAction::Drag);

	std::istringstream again(writeSettings(s));
	std::vector<std::string> none;
	Settings back = parseSettings(again, none);
	CHECK(back.controls == s.controls);
	CHECK(back.gamepad.lookSpeed == 150);
	CHECK(back.gamepad.invertY);
	CHECK(writeSettings(back) == writeSettings(s));

	warnings.clear();
	parse("[gamepad]\njump = space\nattack = pad_start\n[controls]\njump = pad_a\n", warnings);
	CHECK(warnings.size() == 3);
}

TEST_CASE("gamepad: docs/settings.md lists every default pad button") {
	std::ifstream f("docs/settings.md");
	REQUIRE(f);
	std::stringstream text;
	text << f.rdbuf();
	const Bindings defaults;
	for (int i = 0; i < BIND_ACTION_COUNT; i++) {
		const InputKey& pad = defaults.Of(static_cast<BindAction>(i)).pad;
		if (pad.Bound())
			CHECK_MESSAGE(text.str().contains("`" + keyName(pad) + "`"), "missing: ", keyName(pad));
	}
}

TEST_CASE("gamepad: the left stick holds a direction past the dead zone, lets go a little inside") {
	PadStick stick;
	stick.Move(-0.2f, 0.f, 0.25f);
	CHECK_FALSE(stick.Held(PadStick::Dir::Left));
	stick.Move(-0.3f, 0.f, 0.25f);
	CHECK(stick.Held(PadStick::Dir::Left));
	CHECK_FALSE(stick.Held(PadStick::Dir::Right));
	stick.Move(-0.22f, 0.f, 0.25f); // inside the zone, not past the hysteresis
	CHECK(stick.Held(PadStick::Dir::Left));
	stick.Move(-0.1f, 0.9f, 0.25f);
	CHECK_FALSE(stick.Held(PadStick::Dir::Left));
	CHECK(stick.Held(PadStick::Dir::Down)); // y down
	CHECK_FALSE(stick.Held(PadStick::Dir::Up));
}

TEST_CASE("gamepad: the right stick turns the look, nothing in the dead zone, fine near it") {
	CHECK(padStickReach(0.2f, 0.25f) == 0.f);
	CHECK(padStickReach(1.f, 0.25f) == doctest::Approx(1.f));
	CHECK(padStickReach(-1.f, 0.25f) == doctest::Approx(-1.f));
	CHECK(padStickReach(0.625f, 0.25f) == doctest::Approx(0.25f)); // half way past the zone: a quarter
	const PadLookTurn full = padLookTurn(1.f, -1.f, 0.25f, 100, false, 1000);
	CHECK(full.m == doctest::Approx(PAD_LOOK_SPEED)); // right
	CHECK(full.n == doctest::Approx(PAD_LOOK_SPEED)); // pushed up: looks up
	CHECK(padLookTurn(0.f, -1.f, 0.25f, 50, true, 1000).n == doctest::Approx(-PAD_LOOK_SPEED / 2));
	const PadLookTurn idle = padLookTurn(0.1f, 0.1f, 0.25f, 300, false, 16);
	CHECK(idle.m == 0.f);
	CHECK(idle.n == 0.f);
}

TEST_CASE("gamepad: the fonts draw the PlayStation symbols after ASCII, skip other control characters") {
	CHECK(glyphOf('A') == 'A' - 32);
	CHECK(glyphOf(static_cast<unsigned char>(PadGlyph::CROSS)) == ASCII_GLYPHS);
	CHECK(glyphOf(static_cast<unsigned char>(PadGlyph::ARROW_RIGHT)) == FONT_GLYPHS - 1);
	CHECK(glyphOf('\n') == -1);
	CHECK(glyphOf(127) == -1);
	CHECK(glyphOf(200) == -1);
	CHECK(glyphCodePoint(glyphOf('~')) == '~');
	CHECK(glyphCodePoint(glyphOf(static_cast<unsigned char>(PadGlyph::CIRCLE))) == 0x25CB);
}

TEST_CASE("gamepad: the PlayStation symbols have their own colours, the other glyphs none") {
	const auto tint = [](char c) { return padGlyphTint(glyphOf(static_cast<unsigned char>(c))); };
	REQUIRE(tint(PadGlyph::CROSS) != nullptr);
	CHECK((*tint(PadGlyph::CROSS))[2] > (*tint(PadGlyph::CROSS))[0]); // blue
	CHECK((*tint(PadGlyph::CIRCLE))[0] > (*tint(PadGlyph::CIRCLE))[1]); // red
	CHECK((*tint(PadGlyph::TRIANGLE))[1] > (*tint(PadGlyph::TRIANGLE))[0]); // green
	CHECK((*tint(PadGlyph::SQUARE))[2] > (*tint(PadGlyph::SQUARE))[1]); // pink: more blue than green
	CHECK(tint(PadGlyph::ARROW_UP) == nullptr);
	CHECK(tint('A') == nullptr);
}
