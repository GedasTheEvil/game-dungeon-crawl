#include "bindings.h"
#include <cctype>
#include <cstdio>

namespace {
// GLUT special key codes (GLUT_KEY_*; Shift, Ctrl and Alt are freeglut's). input.h names the ones the game uses.
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
constexpr int SHIFT_LEFT = 112;
constexpr int SHIFT_RIGHT = 113;
constexpr unsigned char ESCAPE = 27;

struct NamedKey {
	const char* name;
	const char* cap;
	InputKey key;
};

// Keys whose name is not the character itself: the ones the file format needs (",", ";", "#", "=") and the ones
// without a glyph. F-keys are added by number.
const NamedKey NAMED[] = {
	{"space", "Space", {InputKey::Kind::Char, ' '}},
	{"enter", "Enter", {InputKey::Kind::Char, 13}},
	{"tab", "Tab", {InputKey::Kind::Char, 9}},
	{"backspace", "Bksp", {InputKey::Kind::Char, 8}},
	{"delete", "Del", {InputKey::Kind::Char, 127}},
	{"comma", ",", {InputKey::Kind::Char, ','}},
	{"semicolon", ";", {InputKey::Kind::Char, ';'}},
	{"hash", "#", {InputKey::Kind::Char, '#'}},
	{"equals", "=", {InputKey::Kind::Char, '='}},
	{"left", "Left", {InputKey::Kind::Special, LEFT}},
	{"right", "Right", {InputKey::Kind::Special, RIGHT}},
	{"up", "Up", {InputKey::Kind::Special, UP}},
	{"down", "Down", {InputKey::Kind::Special, DOWN}},
	{"page_up", "PgUp", {InputKey::Kind::Special, PAGE_UP}},
	{"page_down", "PgDn", {InputKey::Kind::Special, PAGE_DOWN}},
	{"home", "Home", {InputKey::Kind::Special, HOME}},
	{"end", "End", {InputKey::Kind::Special, END}},
	{"insert", "Ins", {InputKey::Kind::Special, 108}},
	{"num_lock", "NumLk", {InputKey::Kind::Special, 109}},
	{"begin", "Begin", {InputKey::Kind::Special, 110}},
	{"keypad_delete", "Del", {InputKey::Kind::Special, 111}},
	{"shift", "Shift", {InputKey::Kind::Special, SHIFT_LEFT}},
	{"right_shift", "RShift", {InputKey::Kind::Special, SHIFT_RIGHT}},
	{"ctrl", "Ctrl", {InputKey::Kind::Special, 114}},
	{"right_ctrl", "RCtrl", {InputKey::Kind::Special, 115}},
	{"alt", "Alt", {InputKey::Kind::Special, 116}},
	{"right_alt", "RAlt", {InputKey::Kind::Special, 117}},
	{"mouse_left", "LMB", {InputKey::Kind::Mouse, 0}},
	{"mouse_middle", "MMB", {InputKey::Kind::Mouse, 1}},
	{"mouse_right", "RMB", {InputKey::Kind::Mouse, 2}},
};

const NamedKey* named(const InputKey& key) {
	for (const NamedKey& entry : NAMED)
		if (entry.key == key)
			return &entry;
	return nullptr;
}

struct ActionInfo {
	const char* name;
	const char* label;
};

// In BindAction order.
constexpr ActionInfo ACTIONS[BIND_ACTION_COUNT] = {
	{"move_left", "Move left"},
	{"move_right", "Move right"},
	{"climb_up", "Climb up"},
	{"climb_down", "Climb down"},
	{"jump", "Jump"},
	{"sprint", "Sprint (hold)"},
	{"attack", "Attack"},
	{"interact", "Interact"},
	{"look_left", "Look left"},
	{"look_right", "Look right"},
	{"look_up", "Look up"},
	{"look_down", "Look down"},
	{"equip_melee", "Melee weapon"},
	{"equip_ranged", "Ranged weapon"},
	{"quick_heal", "Healing potion"},
	{"quick_stamina", "Stamina potion"},
	{"quick_antidote", "Antidote"},
	{"inventory", "Inventory"},
	{"map", "Draft map"},
	{"journal", "Journal"},
};

InputKey ch(unsigned char c) { return InputKey::Char(c); }
InputKey sp(int key) { return InputKey::Special(key); }
InputKey none() { return {}; }

std::string trim(const std::string& s) {
	size_t begin = s.find_first_not_of(" \t");
	if (begin == std::string::npos)
		return "";
	return s.substr(begin, s.find_last_not_of(" \t") - begin + 1);
}
} // namespace

InputKey InputKey::Char(unsigned char c) { return {Kind::Char, std::tolower(c)}; }

const InputKey* Binding::First() const {
	for (const InputKey& key : keys)
		if (key.Bound())
			return &key;
	return mouse.Bound() ? &mouse : nullptr;
}

Bindings::Bindings() {
	auto set = [this](BindAction action, InputKey a, InputKey b, InputKey mouse) {
		table[static_cast<size_t>(action)] = {{a, b}, mouse};
	};
	set(BindAction::MoveLeft, ch('a'), sp(LEFT), none());
	set(BindAction::MoveRight, ch('d'), sp(RIGHT), none());
	set(BindAction::ClimbUp, ch('w'), sp(UP), none());
	set(BindAction::ClimbDown, ch('s'), sp(DOWN), none());
	set(BindAction::Jump, ch(' '), none(), InputKey::Mouse(2));
	set(BindAction::Sprint, sp(SHIFT_LEFT), sp(SHIFT_RIGHT), none());
	set(BindAction::Attack, ch('v'), ch(13), InputKey::Mouse(0));
	set(BindAction::Interact, ch('e'), sp(F12), InputKey::Mouse(1));
	set(BindAction::LookLeft, sp(HOME), none(), none());
	set(BindAction::LookRight, sp(END), none(), none());
	set(BindAction::LookUp, sp(PAGE_UP), none(), none());
	set(BindAction::LookDown, sp(PAGE_DOWN), none(), none());
	set(BindAction::EquipMelee, ch('1'), none(), none());
	set(BindAction::EquipRanged, ch('2'), none(), none());
	set(BindAction::QuickHeal, ch('h'), none(), none());
	set(BindAction::QuickStamina, ch('0'), none(), none());
	set(BindAction::QuickAntidote, ch('='), none(), none());
	set(BindAction::Inventory, ch('i'), none(), none());
	set(BindAction::Map, ch('m'), none(), none());
	set(BindAction::Journal, ch('j'), none(), none());
}

std::optional<BindAction> Bindings::ActionFor(const InputKey& key) const {
	if (!key.Bound())
		return std::nullopt;
	for (size_t i = 0; i < table.size(); i++)
		if (table[i].Has(key))
			return static_cast<BindAction>(i);
	return std::nullopt;
}

std::optional<BindAction> Bindings::Bind(BindAction action, int slot, const InputKey& key) {
	std::optional<BindAction> from;
	for (size_t i = 0; i < table.size(); i++)
		for (int s = 0; s < BINDING_SLOTS; s++)
			if (table[i].Slot(s) == key && (static_cast<BindAction>(i) != action || s != slot)) {
				table[i].Slot(s) = {};
				if (static_cast<BindAction>(i) != action)
					from = static_cast<BindAction>(i);
			}
	table[static_cast<size_t>(action)].Slot(slot) = key;
	return from;
}

int Bindings::RemoveDuplicates() {
	int removed = 0;
	for (size_t i = 0; i < table.size(); i++)
		for (int s = 0; s < BINDING_SLOTS; s++) {
			InputKey& key = table[i].Slot(s);
			if (!key.Bound())
				continue;
			bool earlier = false;
			for (size_t j = 0; j <= i && !earlier; j++)
				for (int t = 0; t < BINDING_SLOTS && !earlier; t++)
					earlier = (j < i || t < s) && table[j].Slot(t) == key;
			if (earlier) {
				key = {};
				removed++;
			}
		}
	return removed;
}

bool Bindings::operator==(const Bindings& o) const {
	for (size_t i = 0; i < table.size(); i++)
		for (int s = 0; s < BINDING_SLOTS; s++)
			if (table[i].Slot(s) != o.table[i].Slot(s))
				return false;
	return true;
}

const char* bindActionName(BindAction action) { return ACTIONS[static_cast<size_t>(action)].name; }

const char* bindActionLabel(BindAction action) { return ACTIONS[static_cast<size_t>(action)].label; }

bool isReservedKey(const InputKey& key) {
	if (key.kind == InputKey::Kind::Char)
		return key.code == ESCAPE;
	return key.kind == InputKey::Kind::Special && key.code >= F1 && key.code <= F11;
}

std::string keyName(const InputKey& key) {
	if (const NamedKey* entry = named(key))
		return entry->name;
	if (key.kind == InputKey::Kind::Char && key.code > ' ' && key.code < 127)
		return std::string(1, static_cast<char>(key.code));
	if (key.kind == InputKey::Kind::Special && key.code >= F1 && key.code <= F12)
		return "f" + std::to_string(key.code);
	return "";
}

std::optional<InputKey> parseKeyName(const std::string& name) {
	std::string lower;
	for (char c : name)
		lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	for (const NamedKey& entry : NAMED)
		if (lower == entry.name)
			return entry.key;
	if (lower.size() == 1 && lower[0] > ' ' && lower[0] < 127)
		return InputKey::Char(static_cast<unsigned char>(lower[0]));
	int f = 0;
	char rest = 0;
	if (lower.size() > 1 && lower[0] == 'f' && std::sscanf(lower.c_str() + 1, "%d%c", &f, &rest) == 1 && f >= F1 &&
		f <= F12)
		return InputKey::Special(f);
	return std::nullopt;
}

std::string keyCap(const InputKey& key) {
	if (const NamedKey* entry = named(key))
		return entry->cap;
	if (key.kind == InputKey::Kind::Char && key.code > ' ' && key.code < 127)
		return std::string(1, static_cast<char>(std::toupper(key.code)));
	if (key.kind == InputKey::Kind::Special && key.code >= F1 && key.code <= F12)
		return "F" + std::to_string(key.code);
	return "?";
}

std::string formatBinding(const Binding& binding) {
	std::string out;
	for (int s = 0; s < BINDING_SLOTS; s++)
		if (const InputKey& key = binding.Slot(s); key.Bound())
			out += (out.empty() ? "" : ", ") + keyName(key);
	return out.empty() ? "none" : out;
}

std::string parseBinding(const std::string& value, Binding& binding) {
	Binding parsed;
	if (trim(value) == "none") {
		binding = parsed;
		return "";
	}
	int keys = 0;
	for (size_t pos = 0; pos <= value.size();) {
		size_t end = value.find(',', pos);
		if (end == std::string::npos)
			end = value.size();
		std::string name = trim(value.substr(pos, end - pos));
		pos = end + 1;
		std::optional<InputKey> key = parseKeyName(name);
		if (!key)
			return "unknown key '" + name + "'";
		if (isReservedKey(*key))
			return "'" + name + "' cannot be bound";
		if (parsed.Has(*key))
			continue;
		if (key->kind == InputKey::Kind::Mouse) {
			if (parsed.mouse.Bound())
				return "more than one mouse button";
			parsed.mouse = *key;
		} else {
			if (keys == 2)
				return "more than two keys";
			parsed.keys[static_cast<size_t>(keys++)] = *key;
		}
	}
	binding = parsed;
	return "";
}
