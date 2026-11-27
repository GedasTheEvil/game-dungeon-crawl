#include "bindings.h"
#include <cctype>
#include <cstdio>
#include <utility>

namespace {
constexpr unsigned char ESCAPE = 27;
constexpr unsigned char ENTER = 13;

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
	{"left", "Left", {InputKey::Kind::Special, SpecialKey::LEFT}},
	{"right", "Right", {InputKey::Kind::Special, SpecialKey::RIGHT}},
	{"up", "Up", {InputKey::Kind::Special, SpecialKey::UP}},
	{"down", "Down", {InputKey::Kind::Special, SpecialKey::DOWN}},
	{"page_up", "PgUp", {InputKey::Kind::Special, SpecialKey::PAGE_UP}},
	{"page_down", "PgDn", {InputKey::Kind::Special, SpecialKey::PAGE_DOWN}},
	{"home", "Home", {InputKey::Kind::Special, SpecialKey::HOME}},
	{"end", "End", {InputKey::Kind::Special, SpecialKey::END}},
	{"insert", "Ins", {InputKey::Kind::Special, SpecialKey::INSERT}},
	{"num_lock", "NumLk", {InputKey::Kind::Special, SpecialKey::NUM_LOCK}},
	{"begin", "Begin", {InputKey::Kind::Special, SpecialKey::BEGIN}},
	{"keypad_delete", "Del", {InputKey::Kind::Special, SpecialKey::KEYPAD_DELETE}},
	{"shift", "Shift", {InputKey::Kind::Special, SpecialKey::SHIFT_LEFT}},
	{"right_shift", "RShift", {InputKey::Kind::Special, SpecialKey::SHIFT_RIGHT}},
	{"ctrl", "Ctrl", {InputKey::Kind::Special, SpecialKey::CTRL_LEFT}},
	{"right_ctrl", "RCtrl", {InputKey::Kind::Special, SpecialKey::CTRL_RIGHT}},
	{"alt", "Alt", {InputKey::Kind::Special, SpecialKey::ALT_LEFT}},
	{"right_alt", "RAlt", {InputKey::Kind::Special, SpecialKey::ALT_RIGHT}},
	{"mouse_left", "LMB", {InputKey::Kind::Mouse, 0}},
	{"mouse_middle", "MMB", {InputKey::Kind::Mouse, 1}},
	{"mouse_right", "RMB", {InputKey::Kind::Mouse, 2}},
};

// A gamepad button: its name in the file (the Xbox name, by place), its Xbox cap and its PlayStation cap.
struct NamedPad {
	const char* name;
	const char* xbox;
	const char* playstation;
	int button;
};
const char PS_CROSS[] = {PadGlyph::CROSS, 0};
const char PS_CIRCLE[] = {PadGlyph::CIRCLE, 0};
const char PS_SQUARE[] = {PadGlyph::SQUARE, 0};
const char PS_TRIANGLE[] = {PadGlyph::TRIANGLE, 0};
const char DPAD_UP[] = {'D', PadGlyph::ARROW_UP, 0};
const char DPAD_DOWN[] = {'D', PadGlyph::ARROW_DOWN, 0};
const char DPAD_LEFT[] = {'D', PadGlyph::ARROW_LEFT, 0};
const char DPAD_RIGHT[] = {'D', PadGlyph::ARROW_RIGHT, 0};
const NamedPad PADS[] = {
	{"pad_a", "A", PS_CROSS, PadButton::SOUTH},
	{"pad_b", "B", PS_CIRCLE, PadButton::EAST},
	{"pad_x", "X", PS_SQUARE, PadButton::WEST},
	{"pad_y", "Y", PS_TRIANGLE, PadButton::NORTH},
	{"pad_back", "Back", "Share", PadButton::BACK},
	{"pad_start", "Start", "Options", PadButton::START},
	{"pad_ls", "LS", "L3", PadButton::LEFT_STICK},
	{"pad_rs", "RS", "R3", PadButton::RIGHT_STICK},
	{"pad_lb", "LB", "L1", PadButton::LEFT_SHOULDER},
	{"pad_rb", "RB", "R1", PadButton::RIGHT_SHOULDER},
	{"pad_dpad_up", DPAD_UP, DPAD_UP, PadButton::DPAD_UP},
	{"pad_dpad_down", DPAD_DOWN, DPAD_DOWN, PadButton::DPAD_DOWN},
	{"pad_dpad_left", DPAD_LEFT, DPAD_LEFT, PadButton::DPAD_LEFT},
	{"pad_dpad_right", DPAD_RIGHT, DPAD_RIGHT, PadButton::DPAD_RIGHT},
	{"pad_lt", "LT", "L2", PadButton::LEFT_TRIGGER},
	{"pad_rt", "RT", "R2", PadButton::RIGHT_TRIGGER},
};

const NamedPad* namedPad(const InputKey& key) {
	if (key.kind != InputKey::Kind::Pad)
		return nullptr;
	for (const NamedPad& entry : PADS)
		if (entry.button == key.code)
			return &entry;
	return nullptr;
}

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
	{"climb_down", "Down: climb, crouch"},
	{"jump", "Jump"},
	{"sprint", "Sprint (hold)"},
	{"attack", "Attack"},
	{"interact", "Interact"},
	{"drag", "Drag, let go"},
	{"look_left", "Look left"},
	{"look_right", "Look right"},
	{"look_up", "Look up"},
	{"look_down", "Look down"},
	{"equip_melee", "Melee weapon"},
	{"equip_ranged", "Ranged weapon"},
	{"quick_heal", "Healing potion"},
	{"quick_stamina", "Stamina potion"},
	{"quick_antidote", "Antidote"},
	{"stamina_skills", "Stamina skills"},
	{"inventory", "Inventory"},
	{"map", "Draft map"},
	{"journal", "Journal"},
	{"skills", "Heka (skills)"},
};

InputKey ch(unsigned char c) { return InputKey::Char(c); }
InputKey sp(int key) { return InputKey::Special(key); }
InputKey pad(int button) { return InputKey::Pad(button); }
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
		table[std::to_underlying(action)] = {{a, b}, mouse, {}};
	};
	set(BindAction::MoveLeft, ch('a'), sp(SpecialKey::LEFT), none());
	set(BindAction::MoveRight, ch('d'), sp(SpecialKey::RIGHT), none());
	set(BindAction::ClimbUp, ch('w'), sp(SpecialKey::UP), none());
	set(BindAction::ClimbDown, ch('s'), sp(SpecialKey::DOWN), none());
	set(BindAction::Jump, ch(' '), none(), InputKey::Mouse(2));
	set(BindAction::Sprint, sp(SpecialKey::SHIFT_LEFT), sp(SpecialKey::SHIFT_RIGHT), none());
	set(BindAction::Attack, ch('v'), ch(13), InputKey::Mouse(0));
	set(BindAction::Interact, ch('e'), sp(SpecialKey::F12), InputKey::Mouse(1));
	set(BindAction::Drag, ch('r'), none(), none());
	set(BindAction::LookLeft, sp(SpecialKey::HOME), none(), none());
	set(BindAction::LookRight, sp(SpecialKey::END), none(), none());
	set(BindAction::LookUp, sp(SpecialKey::PAGE_UP), none(), none());
	set(BindAction::LookDown, sp(SpecialKey::PAGE_DOWN), none(), none());
	set(BindAction::EquipMelee, ch('1'), none(), none());
	set(BindAction::EquipRanged, ch('2'), none(), none());
	// 1-5 read as one hotbar under the left hand: melee, ranged, heal, stamina, antidote (plan controls-smoothing).
	set(BindAction::QuickHeal, ch('3'), ch('h'), none());
	set(BindAction::QuickStamina, ch('4'), ch('0'), none());
	set(BindAction::QuickAntidote, ch('5'), ch('='), none());
	set(BindAction::StaminaSkills, ch('q'), none(), none());
	set(BindAction::Inventory, ch('i'), none(), none());
	set(BindAction::Map, ch('m'), none(), none());
	set(BindAction::Journal, ch('j'), none(), none());
	set(BindAction::Skills, ch('k'), none(), none());

	// The gamepad (plan sdl3-gamepad): the left stick walks and climbs and the right stick looks (fixed), so the d-pad
	// is the potions and the map; the face buttons jump, attack, interact and drag; Start is the menu (fixed).
	auto setPad = [this](BindAction action, int button) { table[std::to_underlying(action)].pad = pad(button); };
	setPad(BindAction::Jump, PadButton::SOUTH);
	setPad(BindAction::Interact, PadButton::EAST);
	setPad(BindAction::Attack, PadButton::WEST);
	setPad(BindAction::Drag, PadButton::NORTH);
	setPad(BindAction::EquipMelee, PadButton::LEFT_SHOULDER);
	setPad(BindAction::EquipRanged, PadButton::RIGHT_SHOULDER);
	setPad(BindAction::Sprint, PadButton::LEFT_TRIGGER);
	setPad(BindAction::StaminaSkills, PadButton::RIGHT_TRIGGER);
	setPad(BindAction::QuickHeal, PadButton::DPAD_UP);
	setPad(BindAction::QuickStamina, PadButton::DPAD_RIGHT);
	setPad(BindAction::QuickAntidote, PadButton::DPAD_LEFT);
	setPad(BindAction::Map, PadButton::DPAD_DOWN);
	setPad(BindAction::Inventory, PadButton::BACK);
	setPad(BindAction::Journal, PadButton::LEFT_STICK);
	setPad(BindAction::Skills, PadButton::RIGHT_STICK);
}

bool fitsSlot(const InputKey& key, int slot) {
	switch (key.kind) {
	case InputKey::Kind::Mouse:
		return slot == MOUSE_SLOT;
	case InputKey::Kind::Pad:
		return slot == PAD_SLOT;
	case InputKey::Kind::Char:
	case InputKey::Kind::Special:
		return slot == 0 || slot == 1;
	case InputKey::Kind::None:
		break;
	}
	return true;
}

std::optional<BindAction> Bindings::ActionFor(const InputKey& key) const {
	if (!key.Bound())
		return std::nullopt;
	for (size_t i = 0; i < table.size(); i++)
		if (table[i].Has(key))
			return static_cast<BindAction>(i);
	return std::nullopt;
}

NavKey navKeyOf(const InputKey& key, const Bindings& bindings) {
	if (key == InputKey::Special(SpecialKey::UP))
		return NavKey::Up;
	if (key == InputKey::Special(SpecialKey::DOWN))
		return NavKey::Down;
	if (key == InputKey::Special(SpecialKey::LEFT))
		return NavKey::Left;
	if (key == InputKey::Special(SpecialKey::RIGHT))
		return NavKey::Right;
	if (key == InputKey::Char(ENTER) || key == InputKey::Char(' '))
		return NavKey::Activate;
	switch (bindings.ActionFor(key).value_or(BindAction::Skills)) {
	case BindAction::ClimbUp:
		return NavKey::Up;
	case BindAction::ClimbDown:
		return NavKey::Down;
	case BindAction::MoveLeft:
		return NavKey::Left;
	case BindAction::MoveRight:
		return NavKey::Right;
	case BindAction::Interact:
		return NavKey::Activate;
	default:
		return NavKey::None;
	}
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
	table[std::to_underlying(action)].Slot(slot) = key;
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

const char* bindActionName(BindAction action) { return ACTIONS[std::to_underlying(action)].name; }

const char* bindActionLabel(BindAction action) { return ACTIONS[std::to_underlying(action)].label; }

bool isReservedKey(const InputKey& key) {
	if (key.kind == InputKey::Kind::Pad)
		return key.code == PadButton::START;
	if (key.kind == InputKey::Kind::Char)
		return key.code == ESCAPE;
	return key.kind == InputKey::Kind::Special && key.code >= SpecialKey::F1 && key.code <= SpecialKey::F11;
}

std::string keyName(const InputKey& key) {
	if (const NamedPad* entry = namedPad(key))
		return entry->name;
	if (const NamedKey* entry = named(key))
		return entry->name;
	if (key.kind == InputKey::Kind::Char && key.code > ' ' && key.code < 127)
		return std::string(1, static_cast<char>(key.code));
	if (key.kind == InputKey::Kind::Special && key.code >= SpecialKey::F1 && key.code <= SpecialKey::F12)
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
	for (const NamedPad& entry : PADS)
		if (lower == entry.name)
			return InputKey::Pad(entry.button);
	if (lower.size() == 1 && lower[0] > ' ' && lower[0] < 127)
		return InputKey::Char(static_cast<unsigned char>(lower[0]));
	int f = 0;
	char rest = 0;
	if (lower.size() > 1 && lower[0] == 'f' && std::sscanf(lower.c_str() + 1, "%d%c", &f, &rest) == 1 &&
		f >= SpecialKey::F1 && f <= SpecialKey::F12)
		return InputKey::Special(f);
	return std::nullopt;
}

std::string keyCapOf(const Bindings& bindings, BindAction action, Caps caps) {
	if (caps != Caps::Keys) {
		const InputKey& pad = bindings.Of(action).pad;
		return pad.Bound() ? keyCap(pad, caps) : "";
	}
	const InputKey* key = bindings.Of(action).First();
	return key ? keyCap(*key) : "";
}

std::string equipKeysCap(const Bindings& controls, Caps caps) {
	if (caps != Caps::Keys) {
		const std::string melee = keyCapOf(controls, BindAction::EquipMelee, caps);
		const std::string ranged = keyCapOf(controls, BindAction::EquipRanged, caps);
		return melee + (melee.empty() || ranged.empty() ? "" : " ") + ranged;
	}
	const InputKey* melee = controls.Of(BindAction::EquipMelee).First();
	const InputKey* ranged = controls.Of(BindAction::EquipRanged).First();
	if (melee != nullptr && ranged != nullptr && melee->kind == InputKey::Kind::Char &&
		ranged->kind == InputKey::Kind::Char && ranged->code == melee->code + 1)
		return keyCap(*melee) + "-" + keyCap(*ranged);
	return keyCapOf(controls, BindAction::EquipMelee) + keyCapOf(controls, BindAction::EquipRanged);
}

std::string keyCap(const InputKey& key, Caps caps) {
	if (const NamedPad* entry = namedPad(key))
		return caps == Caps::PlayStation ? entry->playstation : entry->xbox;
	if (const NamedKey* entry = named(key))
		return entry->cap;
	if (key.kind == InputKey::Kind::Char && key.code > ' ' && key.code < 127)
		return std::string(1, static_cast<char>(std::toupper(key.code)));
	if (key.kind == InputKey::Kind::Special && key.code >= SpecialKey::F1 && key.code <= SpecialKey::F12)
		return "F" + std::to_string(key.code);
	return "?";
}

std::string formatBinding(const Binding& binding) {
	std::string out;
	for (int s = 0; s < PAD_SLOT; s++)
		if (const InputKey& key = binding.Slot(s); key.Bound())
			out += (out.empty() ? "" : ", ") + keyName(key);
	return out.empty() ? "none" : out;
}

std::string parseBinding(const std::string& value, Binding& binding) {
	Binding parsed;
	parsed.pad = binding.pad;
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
		if (key->kind == InputKey::Kind::Pad)
			return "'" + name + "' is a gamepad button: bind it in [gamepad]";
		if (parsed.keys[0] == *key || parsed.keys[1] == *key || parsed.mouse == *key)
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

std::string formatPadBinding(const Binding& binding) { return binding.pad.Bound() ? keyName(binding.pad) : "none"; }

std::string parsePadBinding(const std::string& value, Binding& binding) {
	const std::string name = trim(value);
	if (name == "none") {
		binding.pad = {};
		return "";
	}
	std::optional<InputKey> key = parseKeyName(name);
	if (!key || key->kind != InputKey::Kind::Pad)
		return "'" + name + "' is no gamepad button";
	if (isReservedKey(*key))
		return "'" + name + "' cannot be bound";
	binding.pad = *key;
	return "";
}
