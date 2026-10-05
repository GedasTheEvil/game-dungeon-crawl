#include "../../external/doctest/doctest.h"
#include "../../src/input/input.h"
#include "../../src/state/settings_ini.h"
#include <fstream>
#include <sstream>

namespace {
Settings parse(const std::string& text, std::vector<std::string>& warnings) {
	std::istringstream in(text);
	return parseSettings(in, warnings);
}

Settings parse(const std::string& text) {
	std::vector<std::string> warnings;
	Settings settings = parse(text, warnings);
	CHECK(warnings.empty());
	return settings;
}
} // namespace

TEST_CASE("an empty file gives the defaults") {
	Settings s = parse("");
	CHECK(s.display.width == 800);
	CHECK(s.graphics.motionEffects);
	CHECK_FALSE(s.graphics.toon);
	CHECK(s.sound.music == 80);
	CHECK(s.controls == Bindings{});
}

TEST_CASE("sections, keys, comments and booleans") {
	Settings s = parse("; comment\n# another\n\n[display]\n window_width = 1280 \nwindow_height=720\nfullscreen = on\n"
					   "[Graphics]\nmotion_effects = 0\ntoon = true\nblood = off\nlight_flicker = no\n"
					   "[sound]\nmusic_volume = 0\neffects_volume = 55\r\n");
	CHECK(s.display.width == 1280);
	CHECK(s.display.height == 720);
	CHECK(s.display.fullscreen);
	CHECK_FALSE(s.graphics.motionEffects);
	CHECK(s.graphics.toon);
	CHECK_FALSE(s.graphics.blood);
	CHECK_FALSE(s.graphics.lightFlicker);
	CHECK(s.sound.music == 0);
	CHECK(s.sound.effects == 55);
}

TEST_CASE("a bad value warns and keeps the default") {
	std::vector<std::string> warnings;
	Settings s = parse("[sound]\nmusic_volume = 101\neffects_volume = loud\n[graphics]\nblood = maybe\n"
					   "[display]\nwindow_width = 10\n[controls]\njump = nokey\nattack = a, b, c\n",
					   warnings);
	CHECK(warnings.size() == 6);
	CHECK(s.sound.music == 80);
	CHECK(s.sound.effects == 100);
	CHECK(s.graphics.blood);
	CHECK(s.display.width == 800);
	CHECK(s.controls.Of(BindAction::Jump).keys[0] == InputKey::Char(' '));
	CHECK(s.controls.Of(BindAction::Attack).keys[0] == InputKey::Char('v'));
}

TEST_CASE("unknown keys and sections and broken lines warn") {
	std::vector<std::string> warnings;
	parse("[display]\nbrightness = 3\n[cheats]\ngod = on\nno equals sign\n[open\n", warnings);
	CHECK(warnings.size() == 5);
}

TEST_CASE("what is written reads back the same") {
	Settings s;
	s.display = {1600, 900, true};
	s.graphics = {false, true, false, false};
	s.sound = {35, 0};
	s.controls.Bind(BindAction::Jump, 0, InputKey::Char(','));
	s.controls.Bind(BindAction::Attack, MOUSE_SLOT, InputKey::Mouse(2));
	s.controls.Clear(BindAction::Journal, 0);
	s.controls.Bind(BindAction::Map, 1, InputKey::Special(SPECIAL_SHIFT_RIGHT));

	std::string text = writeSettings(s);
	CHECK(text.find("jump = comma\n") != std::string::npos);
	CHECK(text.find("journal = none\n") != std::string::npos);
	Settings back = parse(text);
	CHECK(writeSettings(back) == text);
	CHECK(back.controls == s.controls);
	CHECK(back.display.fullscreen);
	CHECK(back.sound.music == 35);
}

TEST_CASE("bindings") {
	Bindings b;

	SUBCASE("letters match in both cases") {
		CHECK(b.ActionFor(InputKey::Char('A')) == BindAction::MoveLeft);
		CHECK(b.ActionFor(InputKey::Char('a')) == BindAction::MoveLeft);
		CHECK(b.ActionFor(InputKey::Special(SPECIAL_INTERACT)) == BindAction::Interact);
		CHECK(b.ActionFor(InputKey::Mouse(MOUSE_RIGHT_BUTTON)) == BindAction::Jump);
		CHECK_FALSE(b.ActionFor(InputKey::Char('q')).has_value());
	}

	SUBCASE("the defaults use the game's key codes") {
		CHECK(b.Of(BindAction::LookUp).keys[0] == InputKey::Special(SPECIAL_CAMERA_UP));
		CHECK(b.Of(BindAction::LookLeft).keys[0] == InputKey::Special(SPECIAL_CAMERA_LEFT));
		CHECK(b.Of(BindAction::Sprint).keys[0] == InputKey::Special(SPECIAL_SHIFT_LEFT));
		CHECK(b.Of(BindAction::ClimbDown).keys[1] == InputKey::Special(SPECIAL_MOVE_DOWN));
		CHECK(b.Of(BindAction::Attack).mouse == InputKey::Mouse(MOUSE_LEFT_BUTTON));
	}

	SUBCASE("a key bound elsewhere moves") {
		std::optional<BindAction> from = b.Bind(BindAction::Jump, 1, InputKey::Char('E'));
		CHECK(from == BindAction::Interact);
		CHECK(b.ActionFor(InputKey::Char('e')) == BindAction::Jump);
		CHECK_FALSE(b.Of(BindAction::Interact).keys[0].Bound());
		CHECK(b.Of(BindAction::Interact).keys[1] == InputKey::Special(SPECIAL_INTERACT));
	}

	SUBCASE("a key moves between the slots of one action") {
		CHECK_FALSE(b.Bind(BindAction::MoveLeft, 1, InputKey::Char('a')).has_value());
		CHECK_FALSE(b.Of(BindAction::MoveLeft).keys[0].Bound());
		CHECK(b.Of(BindAction::MoveLeft).keys[1] == InputKey::Char('a'));
	}

	SUBCASE("a duplicate in the file stays with the first action") {
		std::vector<std::string> warnings;
		Settings s = parse("[controls]\njump = a\n", warnings);
		CHECK(warnings.size() == 1);
		CHECK(s.controls.ActionFor(InputKey::Char('a')) == BindAction::MoveLeft);
		CHECK_FALSE(s.controls.Of(BindAction::Jump).keys[0].Bound());
	}
}

TEST_CASE("key names") {
	CHECK(parseKeyName("Space") == InputKey::Char(' '));
	CHECK(parseKeyName("F12") == InputKey::Special(12));
	CHECK(parseKeyName("page_down") == InputKey::Special(SPECIAL_CAMERA_DOWN));
	CHECK(parseKeyName("mouse_middle") == InputKey::Mouse(MOUSE_MIDDLE_BUTTON));
	CHECK_FALSE(parseKeyName("f13").has_value());
	CHECK_FALSE(parseKeyName("f1x").has_value());
	CHECK_FALSE(parseKeyName("").has_value());
	CHECK(keyCap(InputKey::Char('h')) == "H");
	CHECK(keyCap(InputKey::Special(SPECIAL_CAMERA_UP)) == "PgUp");
	for (int c = 0; c < 128; c++)
		if (std::string name = keyName(InputKey::Char(static_cast<unsigned char>(c))); !name.empty())
			CHECK(parseKeyName(name) == InputKey::Char(static_cast<unsigned char>(c)));

	SUBCASE("Esc and F1-F11 cannot be bound") {
		Binding binding;
		CHECK_FALSE(parseBinding("f1", binding).empty());
		CHECK(isReservedKey(InputKey::Char(KEY_ESCAPE)));
		CHECK_FALSE(isReservedKey(InputKey::Special(12)));
		CHECK(parseBinding("f12, mouse_left", binding).empty());
		CHECK_FALSE(parseBinding("mouse_left, mouse_right", binding).empty());
		CHECK(parseBinding("none", binding).empty());
		CHECK(binding.First() == nullptr);
	}
}

TEST_CASE("docs/settings.md lists every action with its default") {
	std::ifstream f("docs/settings.md");
	REQUIRE(f);
	std::stringstream text;
	text << f.rdbuf();
	const Bindings defaults;
	for (int i = 0; i < BIND_ACTION_COUNT; i++) {
		auto action = static_cast<BindAction>(i);
		std::string row =
			std::string("| `") + bindActionName(action) + "` | `" + formatBinding(defaults.Of(action)) + "` |";
		CHECK_MESSAGE(text.str().find(row) != std::string::npos, "missing: ", row);
	}
}
