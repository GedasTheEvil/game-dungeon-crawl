#ifndef BINDINGS_H
#define BINDINGS_H

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

// The player's key bindings (Options > Controls, [controls] and [gamepad] in saves/settings.ini): each action has up to
// two keys, one mouse button and one gamepad button. GL-free, so the unit tests and the settings parser use it too.

// The special keys' codes (InputKey::Kind::Special): F1-F12 are 1-12, the others from 100 on. They are GLUT's numbers
// (the game ran on GLUT before plan sdl3-window-input); src/input/keymap.cpp maps SDL's keys to them. input.h names the
// ones the screens read.
namespace SpecialKey {
constexpr int F1 = 1;
constexpr int F11 = 11;
constexpr int F12 = 12;
constexpr int LEFT = 100;
constexpr int UP = 101;
constexpr int RIGHT = 102;
constexpr int DOWN = 103;
constexpr int PAGE_UP = 104;
constexpr int PAGE_DOWN = 105;
constexpr int HOME = 106;
constexpr int END = 107;
constexpr int INSERT = 108;
constexpr int NUM_LOCK = 109;
constexpr int BEGIN = 110; // the keypad's 5 without Num Lock
constexpr int KEYPAD_DELETE = 111;
constexpr int SHIFT_LEFT = 112;
constexpr int SHIFT_RIGHT = 113;
constexpr int CTRL_LEFT = 114;
constexpr int CTRL_RIGHT = 115;
constexpr int ALT_LEFT = 116;
constexpr int ALT_RIGHT = 117;
} // namespace SpecialKey

// A gamepad's buttons (plan sdl3-gamepad) by their place on the pad: South is A on an Xbox pad, the cross on a
// PlayStation pad. 0-14 are SDL_GamepadButton's numbers (Guide left out: the system's), the triggers past them count
// as buttons once pressed half way (src/input/keymap.cpp maps SDL's to these).
namespace PadButton {
constexpr int SOUTH = 0;
constexpr int EAST = 1;
constexpr int WEST = 2;
constexpr int NORTH = 3;
constexpr int BACK = 4;
constexpr int START = 6;
constexpr int LEFT_STICK = 7;
constexpr int RIGHT_STICK = 8;
constexpr int LEFT_SHOULDER = 9;
constexpr int RIGHT_SHOULDER = 10;
constexpr int DPAD_UP = 11;
constexpr int DPAD_DOWN = 12;
constexpr int DPAD_LEFT = 13;
constexpr int DPAD_RIGHT = 14;
constexpr int LEFT_TRIGGER = 15;
constexpr int RIGHT_TRIGGER = 16;
constexpr int COUNT = 17;
} // namespace PadButton

// Which names the key caps show: the keyboard's, or a gamepad's of that family (the last input came from it).
enum class Caps : std::uint8_t { Keys, Xbox, PlayStation };

// The pad's symbols in a text: control characters the fonts draw as the PlayStation face buttons' cross, circle,
// square and triangle, and as the d-pad's filled arrows (render/glyphs.h).
namespace PadGlyph {
constexpr char CROSS = '\x01';
constexpr char CIRCLE = '\x02';
constexpr char SQUARE = '\x03';
constexpr char TRIANGLE = '\x04';
constexpr char ARROW_UP = '\x05';
constexpr char ARROW_DOWN = '\x06';
constexpr char ARROW_LEFT = '\x07';
constexpr char ARROW_RIGHT = '\x08';
} // namespace PadGlyph

// One input: an ASCII key (stored lower case, so both cases match), a special key (SpecialKey: arrows, F-keys, PgUp,
// Shift), a mouse button (0 left, 1 middle, 2 right) or a gamepad button (PadButton).
struct InputKey {
	enum class Kind : std::uint8_t { None, Char, Special, Mouse, Pad };
	Kind kind = Kind::None;
	int code = 0;

	static InputKey Char(unsigned char c);
	static InputKey Special(int key) { return {Kind::Special, key}; }
	static InputKey Mouse(int button) { return {Kind::Mouse, button}; }
	static InputKey Pad(int button) { return {Kind::Pad, button}; }
	[[nodiscard]] bool Bound() const { return kind != Kind::None; }
	bool operator==(const InputKey& o) const { return kind == o.kind && code == o.code; }
	bool operator!=(const InputKey& o) const { return !(*this == o); }
};

// In the order of the Controls tab and the [controls] section.
enum class BindAction : std::uint8_t {
	MoveLeft,
	MoveRight,
	ClimbUp,
	ClimbDown,
	Jump,
	Sprint,
	Attack,
	Interact,
	Drag, // take hold of a mirror, let go (plan mirror-drag)
	LookLeft,
	LookRight,
	LookUp,
	LookDown,
	EquipMelee,
	EquipRanged,
	QuickHeal,
	QuickStamina,
	QuickAntidote,
	StaminaSkills,
	Inventory,
	Map,
	Journal,
	Skills,
};
constexpr int BIND_ACTION_COUNT = static_cast<int>(BindAction::Skills) + 1;

// Key slots 0 and 1, then the mouse button, then the gamepad button.
constexpr int BINDING_SLOTS = 4;
constexpr int MOUSE_SLOT = 2;
constexpr int PAD_SLOT = 3;

struct Binding {
	std::array<InputKey, 2> keys{};
	InputKey mouse{};
	InputKey pad{};
	[[nodiscard]] const InputKey& Slot(int slot) const {
		return slot == MOUSE_SLOT ? mouse : slot == PAD_SLOT ? pad : keys[static_cast<size_t>(slot)];
	}
	InputKey& Slot(int slot) {
		return slot == MOUSE_SLOT ? mouse : slot == PAD_SLOT ? pad : keys[static_cast<size_t>(slot)];
	}
	[[nodiscard]] bool Has(const InputKey& key) const {
		return keys[0] == key || keys[1] == key || mouse == key || pad == key;
	}
	// The first bound key, else the mouse button, else null (never the pad button: the keyboard's caps).
	[[nodiscard]] const InputKey* First() const;
};

// The slot an input goes into: keys into 0 / 1, a mouse button into MOUSE_SLOT, a pad button into PAD_SLOT.
[[nodiscard]] bool fitsSlot(const InputKey& key, int slot);

class Bindings {
  public:
	Bindings(); // the defaults
	[[nodiscard]] const Binding& Of(BindAction action) const { return table[std::to_underlying(action)]; }
	void Set(BindAction action, const Binding& binding) { table[std::to_underlying(action)] = binding; }
	[[nodiscard]] std::optional<BindAction> ActionFor(const InputKey& key) const;
	// Puts the key into the action's slot (fitsSlot). A key bound elsewhere moves here; returns the action it was
	// taken from.
	std::optional<BindAction> Bind(BindAction action, int slot, const InputKey& key);
	void Clear(BindAction action, int slot) { table[std::to_underlying(action)].Slot(slot) = {}; }
	// Removes a key bound to more than one action from all but the first; returns how many it removed.
	int RemoveDuplicates();
	bool operator==(const Bindings& o) const;

  private:
	std::array<Binding, BIND_ACTION_COUNT> table;
};

// What a key does on a menu screen (plan keyboard-menus): the arrows, Enter and Space, and the keys bound to the move
// and climb actions (a direction) and to Interact (activate). Esc stays with the screen. A gamepad's d-pad and South
// button are fixed (the arrows and Enter: input.cpp sends them as those), never read here.
enum class NavKey : std::uint8_t { None, Up, Down, Left, Right, Activate };
[[nodiscard]] NavKey navKeyOf(const InputKey& key, const Bindings& bindings);

// "move_left": the key in [controls].
[[nodiscard]] const char* bindActionName(BindAction action);
// "Move left": the Controls tab row.
[[nodiscard]] const char* bindActionLabel(BindAction action);

// Esc (menu / back) and F1-F11 (toon, hitboxes) are fixed, so a bad mapping can always be undone; so is a gamepad's
// Start (the menu, as Esc).
[[nodiscard]] bool isReservedKey(const InputKey& key);

// The name in the file: "a", "space", "left", "page_up", "f12", "shift", "mouse_left", "pad_a" (a pad button by its
// place, the Xbox name). Empty for an unnamed key.
[[nodiscard]] std::string keyName(const InputKey& key);
[[nodiscard]] std::optional<InputKey> parseKeyName(const std::string& name);
// The key cap label: "A", "Space", "Left", "PgUp", "F12", "Shift", "LMB"; a pad button by its family's name ("LB",
// "R1", PadGlyph::CROSS), the Xbox one with Caps::Keys.
[[nodiscard]] std::string keyCap(const InputKey& key, Caps caps = Caps::Keys);
// Key cap labels for the HUD: the action's first key ("H"); with a pad's caps its pad button. Empty when unbound.
[[nodiscard]] std::string keyCapOf(const Bindings& bindings, BindAction action, Caps caps = Caps::Keys);
// "1-2" while the melee and ranged keys are two keys in a row, else their first keys one after the other; with a pad's
// caps "LB RB".
[[nodiscard]] std::string equipKeysCap(const Bindings& bindings, Caps caps = Caps::Keys);

// "v, enter, mouse_left", or "none": the keys and the mouse button ([controls]).
[[nodiscard]] std::string formatBinding(const Binding& binding);
// Empty on success, else what is wrong (the binding is then untouched). Sets the keys and the mouse button; the pad
// button stays.
std::string parseBinding(const std::string& value, Binding& binding);
// "pad_a", or "none": the pad button ([gamepad]).
[[nodiscard]] std::string formatPadBinding(const Binding& binding);
// Empty on success, else what is wrong (the binding is then untouched). Sets the pad button only.
std::string parsePadBinding(const std::string& value, Binding& binding);

#endif
