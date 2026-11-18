// SDL's keys as the game's (plan sdl3-window-input): by place on the keyboard, read as on a US layout, so the bindings
// and the [controls] names stay as they were.
#include "../../external/doctest/doctest.h"
#include "../../src/input/keymap.h"
#include "../../src/level/input.h"

namespace {
InputKey at(SDL_Scancode scancode, bool numLock = true) {
	const std::optional<InputKey> key = Keymap::physical(scancode, numLock);
	REQUIRE(key);
	return *key;
}
} // namespace

TEST_CASE("keymap: letters, digits and the punctuation by place, lower case") {
	CHECK(at(SDL_SCANCODE_A) == InputKey::Char('a'));
	CHECK(at(SDL_SCANCODE_W) == InputKey::Char('w'));
	CHECK(at(SDL_SCANCODE_Z) == InputKey::Char('z'));
	CHECK(at(SDL_SCANCODE_1) == InputKey::Char('1'));
	CHECK(at(SDL_SCANCODE_9) == InputKey::Char('9'));
	CHECK(at(SDL_SCANCODE_0) == InputKey::Char('0'));
	CHECK(at(SDL_SCANCODE_EQUALS) == InputKey::Char('='));
	CHECK(at(SDL_SCANCODE_SPACE) == InputKey::Char(KEY_SPACE));
	CHECK(at(SDL_SCANCODE_RETURN) == InputKey::Char(KEY_ENTER));
	CHECK(at(SDL_SCANCODE_ESCAPE) == InputKey::Char(KEY_ESCAPE));
	CHECK(at(SDL_SCANCODE_TAB) == InputKey::Char(KEY_TAB));
	CHECK(at(SDL_SCANCODE_BACKSPACE) == InputKey::Char(KEY_BACKSPACE));
	CHECK(at(SDL_SCANCODE_DELETE) == InputKey::Char(KEY_DELETE));
}

TEST_CASE("keymap: the special keys keep their codes and [controls] names") {
	CHECK(at(SDL_SCANCODE_F1) == InputKey::Special(SPECIAL_TOGGLE_CARTOON));
	CHECK(at(SDL_SCANCODE_F5) == InputKey::Special(SPECIAL_QUICK_SAVE));
	CHECK(at(SDL_SCANCODE_F12) == InputKey::Special(SpecialKey::F12));
	CHECK(at(SDL_SCANCODE_LEFT) == InputKey::Special(SPECIAL_MOVE_LEFT));
	CHECK(at(SDL_SCANCODE_UP) == InputKey::Special(SPECIAL_MOVE_UP));
	CHECK(keyName(at(SDL_SCANCODE_PAGEUP)) == "page_up");
	CHECK(keyName(at(SDL_SCANCODE_LSHIFT)) == "shift");
	CHECK(keyName(at(SDL_SCANCODE_RSHIFT)) == "right_shift");
	CHECK(keyName(at(SDL_SCANCODE_RALT)) == "right_alt");
	CHECK(!Keymap::physical(SDL_SCANCODE_CAPSLOCK, true));
}

TEST_CASE("keymap: the keypad gives digits with Num Lock, else its navigation keys") {
	CHECK(at(SDL_SCANCODE_KP_8, true) == InputKey::Char('8'));
	CHECK(at(SDL_SCANCODE_KP_0, true) == InputKey::Char('0'));
	CHECK(at(SDL_SCANCODE_KP_8, false) == InputKey::Special(SPECIAL_MOVE_UP));
	CHECK(at(SDL_SCANCODE_KP_4, false) == InputKey::Special(SPECIAL_MOVE_LEFT));
	CHECK(at(SDL_SCANCODE_KP_5, false) == InputKey::Special(SpecialKey::BEGIN));
	CHECK(at(SDL_SCANCODE_KP_PERIOD, false) == InputKey::Special(SPECIAL_KEYPAD_DELETE));
	CHECK(at(SDL_SCANCODE_KP_ENTER, false) == InputKey::Char(KEY_ENTER));
	CHECK(at(SDL_SCANCODE_KP_PLUS, false) == InputKey::Char('+'));
}

TEST_CASE("keymap: a typed character is printable ASCII or a control key, else none") {
	CHECK(Keymap::typed(SDLK_A) == 'a');
	CHECK(Keymap::typed('A') == 'A');
	CHECK(Keymap::typed(SDLK_EXCLAIM) == '!');
	CHECK(Keymap::typed(SDLK_RETURN) == KEY_ENTER);
	CHECK(Keymap::typed(SDLK_F1) == 0);
	CHECK(Keymap::typed(0x105) == 0); // a Lithuanian letter: not in the papyrus font
}
