#ifndef BINDINGS_H
#define BINDINGS_H

#include <array>
#include <cstdint>
#include <optional>
#include <string>

// The player's key bindings (Options > Controls, [controls] in saves/settings.ini): each action has up to two keys
// and one mouse button. GL-free, so the unit tests and the settings parser use it too.

// One input: an ASCII key (stored lower case, so both cases match), a GLUT special key (GLUT_KEY_*: arrows, F-keys,
// PgUp, Shift) or a mouse button (0 left, 1 middle, 2 right).
struct InputKey {
	enum class Kind : std::uint8_t { None, Char, Special, Mouse };
	Kind kind = Kind::None;
	int code = 0;

	static InputKey Char(unsigned char c);
	static InputKey Special(int key) { return {Kind::Special, key}; }
	static InputKey Mouse(int button) { return {Kind::Mouse, button}; }
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
	LookLeft,
	LookRight,
	LookUp,
	LookDown,
	EquipMelee,
	EquipRanged,
	QuickHeal,
	QuickStamina,
	QuickAntidote,
	Inventory,
	Map,
	Journal,
};
constexpr int BIND_ACTION_COUNT = static_cast<int>(BindAction::Journal) + 1;

// Key slots 0 and 1, then the mouse button.
constexpr int BINDING_SLOTS = 3;
constexpr int MOUSE_SLOT = 2;

struct Binding {
	std::array<InputKey, 2> keys{};
	InputKey mouse{};
	[[nodiscard]] const InputKey& Slot(int slot) const { return slot == MOUSE_SLOT ? mouse : keys[slot]; }
	InputKey& Slot(int slot) { return slot == MOUSE_SLOT ? mouse : keys[slot]; }
	[[nodiscard]] bool Has(const InputKey& key) const { return keys[0] == key || keys[1] == key || mouse == key; }
	[[nodiscard]] const InputKey* First() const; // the first bound key, else the mouse button, else null
};

class Bindings {
  public:
	Bindings(); // the defaults
	[[nodiscard]] const Binding& Of(BindAction action) const { return table[static_cast<size_t>(action)]; }
	void Set(BindAction action, const Binding& binding) { table[static_cast<size_t>(action)] = binding; }
	[[nodiscard]] std::optional<BindAction> ActionFor(const InputKey& key) const;
	// Puts the key into the action's slot (keys into 0 / 1, a mouse button into MOUSE_SLOT). A key bound elsewhere
	// moves here; returns the action it was taken from.
	std::optional<BindAction> Bind(BindAction action, int slot, const InputKey& key);
	void Clear(BindAction action, int slot) { table[static_cast<size_t>(action)].Slot(slot) = {}; }
	// Removes a key bound to more than one action from all but the first; returns how many it removed.
	int RemoveDuplicates();
	bool operator==(const Bindings& o) const;

  private:
	std::array<Binding, BIND_ACTION_COUNT> table;
};

// "move_left": the key in [controls].
[[nodiscard]] const char* bindActionName(BindAction action);
// "Move left": the Controls tab row.
[[nodiscard]] const char* bindActionLabel(BindAction action);

// Esc (menu / back) and F1-F11 (toon, hitboxes) are fixed, so a bad mapping can always be undone.
[[nodiscard]] bool isReservedKey(const InputKey& key);

// The name in the file: "a", "space", "left", "page_up", "f12", "shift", "mouse_left". Empty for an unnamed key.
[[nodiscard]] std::string keyName(const InputKey& key);
[[nodiscard]] std::optional<InputKey> parseKeyName(const std::string& name);
// The key cap label: "A", "Space", "Left", "PgUp", "F12", "Shift", "LMB".
[[nodiscard]] std::string keyCap(const InputKey& key);

// "v, enter, mouse_left", or "none".
[[nodiscard]] std::string formatBinding(const Binding& binding);
// Empty on success, else what is wrong (the binding is then untouched).
std::string parseBinding(const std::string& value, Binding& binding);

#endif
