#include "controls_help.h"

namespace {
std::string firstCap(const Binding& binding) {
	const InputKey* key = binding.First();
	return key != nullptr ? keyCap(*key) : "-";
}

// "Home / End": the first key of each of two actions that read as one row (look left / right).
std::string pairCaps(const Bindings& bindings, BindAction a, BindAction b) {
	return firstCap(bindings.Of(a)) + " / " + firstCap(bindings.Of(b));
}

HelpRow row(const Bindings& bindings, BindAction action, const char* label = nullptr) {
	return {label != nullptr ? label : bindActionLabel(action), bindingCaps(bindings.Of(action))};
}

// "Space or RMB": the hint's key part, the first key and the mouse button.
std::string spoken(const Binding& binding) {
	const InputKey* key = nullptr;
	for (const InputKey& k : binding.keys)
		if (k.Bound()) {
			key = &k;
			break;
		}
	if (key != nullptr && binding.mouse.Bound())
		return keyCap(*key) + " or " + keyCap(binding.mouse);
	if (const InputKey* any = binding.First())
		return keyCap(*any);
	return "";
}
} // namespace

std::string bindingCaps(const Binding& binding) {
	std::string out;
	for (int slot = 0; slot < BINDING_SLOTS; slot++)
		if (const InputKey& key = binding.Slot(slot); key.Bound())
			out += (out.empty() ? "" : " / ") + keyCap(key);
	return out.empty() ? "-" : out;
}

std::vector<HelpGroup> controlsHelp(const Bindings& b) {
	using A = BindAction;
	return {
		{"Move",
		 {
			 row(b, A::MoveLeft),
			 row(b, A::MoveRight),
			 row(b, A::ClimbUp, "Climb up a ladder"),
			 row(b, A::ClimbDown),
			 row(b, A::Jump),
			 row(b, A::Sprint),
			 {"Look left / right", pairCaps(b, A::LookLeft, A::LookRight)},
			 {"Look up / down", pairCaps(b, A::LookUp, A::LookDown)},
			 {"Look around", "Mouse"},
		 }},
		{"Fight",
		 {
			 row(b, A::Attack),
			 {"Power hit: hold, let go", firstCap(b.Of(A::Attack))},
			 row(b, A::EquipMelee),
			 row(b, A::EquipRanged),
			 row(b, A::StaminaSkills, "Stamina skills on / off"),
		 }},
		{"Items",
		 {
			 row(b, A::Interact, "Open, pull, read"),
			 row(b, A::QuickHeal, "Drink healing"),
			 row(b, A::QuickStamina, "Drink stamina"),
			 row(b, A::QuickAntidote, "Drink antidote"),
		 }},
		{"Screens",
		 {
			 row(b, A::Inventory),
			 row(b, A::Map),
			 row(b, A::Journal),
			 row(b, A::Skills),
			 {"Menu, back", "Esc"},
			 {"Toon shading", "F1"},
		 }},
		{"Saves",
		 {
			 {"Quick save", "F5"},
			 {"Quick load", "F9"},
		 }},
	};
}

std::string hintText(FirstHint hint, const Bindings& bindings) {
	struct Text {
		BindAction action;
		const char* what;
		const char* after;
	};
	// In FirstHint order.
	static constexpr Text TEXTS[FIRST_HINT_COUNT] = {
		{BindAction::Jump, "Jump", ""},
		{BindAction::ClimbUp, "Climb", ""},
		{BindAction::Attack, "Attack", ""},
		{BindAction::Sprint, "Sprint", " (hold, uses stamina)"},
		{BindAction::Attack, "Power hit: hold", ", let go to strike"},
		{BindAction::QuickHeal, "Drink a healing potion", ""},
		{BindAction::QuickStamina, "Drink a stamina potion", ""},
		{BindAction::QuickAntidote, "Drink an antidote", ""},
	};
	const Text& t = TEXTS[static_cast<size_t>(hint)];
	const std::string keys = spoken(bindings.Of(t.action));
	if (keys.empty())
		return std::string(t.what) + ": bind a key in Options > Controls";
	if (hint == FirstHint::PowerHit)
		return std::string(t.what) + " " + keys + t.after;
	if (const std::string down = spoken(bindings.Of(BindAction::ClimbDown)); hint == FirstHint::Climb && !down.empty())
		return std::string(t.what) + ": " + keys + " up, " + down + " down";
	return std::string(t.what) + ": " + keys + t.after;
}

bool HintPacer::Pending(std::uint32_t& shown, FirstHint hint, bool done) {
	if ((shown & hintBit(hint)) != 0)
		return false;
	if (done) {
		shown |= hintBit(hint);
		return false;
	}
	return true;
}

bool HintPacer::Offer(std::uint32_t& shown, FirstHint hint, bool possible, int now) {
	if (!possible || now - lastHintMs < HINT_GAP_MS)
		return false;
	shown |= hintBit(hint);
	lastHintMs = now;
	return true;
}
