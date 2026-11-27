#include "keymap.h"
#include <utility>

namespace {
// The US layout's characters of the main block's punctuation keys, in scancode order (SDL_SCANCODE_RETURN on).
struct CharKey {
	SDL_Scancode scancode;
	unsigned char c;
};
constexpr CharKey CHARS[] = {
	{SDL_SCANCODE_RETURN, 13},		 {SDL_SCANCODE_ESCAPE, 27},		  {SDL_SCANCODE_BACKSPACE, 8},
	{SDL_SCANCODE_TAB, 9},			 {SDL_SCANCODE_SPACE, ' '},		  {SDL_SCANCODE_MINUS, '-'},
	{SDL_SCANCODE_EQUALS, '='},		 {SDL_SCANCODE_LEFTBRACKET, '['}, {SDL_SCANCODE_RIGHTBRACKET, ']'},
	{SDL_SCANCODE_BACKSLASH, '\\'},	 {SDL_SCANCODE_NONUSHASH, '#'},	  {SDL_SCANCODE_SEMICOLON, ';'},
	{SDL_SCANCODE_APOSTROPHE, '\''}, {SDL_SCANCODE_GRAVE, '`'},		  {SDL_SCANCODE_COMMA, ','},
	{SDL_SCANCODE_PERIOD, '.'},		 {SDL_SCANCODE_SLASH, '/'},		  {SDL_SCANCODE_NONUSBACKSLASH, '<'},
	{SDL_SCANCODE_DELETE, 127},		 {SDL_SCANCODE_KP_DIVIDE, '/'},	  {SDL_SCANCODE_KP_MULTIPLY, '*'},
	{SDL_SCANCODE_KP_MINUS, '-'},	 {SDL_SCANCODE_KP_PLUS, '+'},	  {SDL_SCANCODE_KP_ENTER, 13},
};

struct SpecialScancode {
	SDL_Scancode scancode;
	int code;
};
constexpr SpecialScancode SPECIALS[] = {
	{SDL_SCANCODE_LEFT, SpecialKey::LEFT},
	{SDL_SCANCODE_UP, SpecialKey::UP},
	{SDL_SCANCODE_RIGHT, SpecialKey::RIGHT},
	{SDL_SCANCODE_DOWN, SpecialKey::DOWN},
	{SDL_SCANCODE_PAGEUP, SpecialKey::PAGE_UP},
	{SDL_SCANCODE_PAGEDOWN, SpecialKey::PAGE_DOWN},
	{SDL_SCANCODE_HOME, SpecialKey::HOME},
	{SDL_SCANCODE_END, SpecialKey::END},
	{SDL_SCANCODE_INSERT, SpecialKey::INSERT},
	{SDL_SCANCODE_NUMLOCKCLEAR, SpecialKey::NUM_LOCK},
	{SDL_SCANCODE_LSHIFT, SpecialKey::SHIFT_LEFT},
	{SDL_SCANCODE_RSHIFT, SpecialKey::SHIFT_RIGHT},
	{SDL_SCANCODE_LCTRL, SpecialKey::CTRL_LEFT},
	{SDL_SCANCODE_RCTRL, SpecialKey::CTRL_RIGHT},
	{SDL_SCANCODE_LALT, SpecialKey::ALT_LEFT},
	{SDL_SCANCODE_RALT, SpecialKey::ALT_RIGHT},
};

// The keypad without Num Lock: 1-9 and 0 in scancode order, then the period.
constexpr int KEYPAD_NAVIGATION[] = {
	SpecialKey::END,
	SpecialKey::DOWN,
	SpecialKey::PAGE_DOWN,
	SpecialKey::LEFT,
	SpecialKey::BEGIN,
	SpecialKey::RIGHT,
	SpecialKey::HOME,
	SpecialKey::UP,
	SpecialKey::PAGE_UP,
	SpecialKey::INSERT,
	SpecialKey::KEYPAD_DELETE,
};
constexpr char KEYPAD_DIGITS[] = "1234567890.";

bool within(SDL_Scancode scancode, SDL_Scancode first, SDL_Scancode last) {
	return std::to_underlying(scancode) >= std::to_underlying(first) &&
		   std::to_underlying(scancode) <= std::to_underlying(last);
}

int offset(SDL_Scancode scancode, SDL_Scancode first) {
	return static_cast<int>(std::to_underlying(scancode)) - static_cast<int>(std::to_underlying(first));
}
} // namespace

namespace Keymap {

std::optional<InputKey> physical(SDL_Scancode scancode, bool numLock) {
	if (within(scancode, SDL_SCANCODE_A, SDL_SCANCODE_Z))
		return InputKey::Char(static_cast<unsigned char>('a' + offset(scancode, SDL_SCANCODE_A)));
	if (within(scancode, SDL_SCANCODE_1, SDL_SCANCODE_9))
		return InputKey::Char(static_cast<unsigned char>('1' + offset(scancode, SDL_SCANCODE_1)));
	if (scancode == SDL_SCANCODE_0)
		return InputKey::Char('0');
	if (within(scancode, SDL_SCANCODE_F1, SDL_SCANCODE_F12))
		return InputKey::Special(SpecialKey::F1 + offset(scancode, SDL_SCANCODE_F1));
	if (within(scancode, SDL_SCANCODE_KP_1, SDL_SCANCODE_KP_PERIOD)) {
		const auto at = static_cast<size_t>(offset(scancode, SDL_SCANCODE_KP_1));
		return numLock ? InputKey::Char(static_cast<unsigned char>(KEYPAD_DIGITS[at]))
					   : InputKey::Special(KEYPAD_NAVIGATION[at]);
	}
	for (const CharKey& key : CHARS)
		if (key.scancode == scancode)
			return InputKey::Char(key.c);
	for (const SpecialScancode& key : SPECIALS)
		if (key.scancode == scancode)
			return InputKey::Special(key.code);
	return std::nullopt;
}

static_assert(SDL_GAMEPAD_BUTTON_SOUTH == PadButton::SOUTH && SDL_GAMEPAD_BUTTON_NORTH == PadButton::NORTH);
static_assert(SDL_GAMEPAD_BUTTON_BACK == PadButton::BACK && SDL_GAMEPAD_BUTTON_START == PadButton::START);
static_assert(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER == PadButton::LEFT_SHOULDER);
static_assert(SDL_GAMEPAD_BUTTON_DPAD_RIGHT == PadButton::DPAD_RIGHT);

std::optional<int> padButton(SDL_GamepadButton button) {
	const int code = std::to_underlying(button);
	if (code < 0 || code > PadButton::DPAD_RIGHT || button == SDL_GAMEPAD_BUTTON_GUIDE)
		return std::nullopt;
	return code; // PadButton keeps SDL's numbers
}

Caps padCaps(SDL_GamepadType type) {
	switch (type) {
	case SDL_GAMEPAD_TYPE_PS3:
	case SDL_GAMEPAD_TYPE_PS4:
	case SDL_GAMEPAD_TYPE_PS5:
		return Caps::PlayStation;
	default:
		return Caps::Xbox;
	}
}

unsigned char typed(SDL_Keycode key) {
	if ((key >= ' ' && key <= '~') || key == SDLK_RETURN || key == SDLK_ESCAPE || key == SDLK_TAB ||
		key == SDLK_BACKSPACE || key == SDLK_DELETE)
		return static_cast<unsigned char>(key);
	return 0;
}

} // namespace Keymap
