#include "player_hud_view.h"
#include "inventory.h"
#include "../input/input_actions.h"
#include "../core/timer.h"
#include "../state/game_state.h"
#include <optional>

namespace {
// A quick slot: the potion H / 0 would drink now, empty when none of that kind is left.
PlayerHud::Slot quickSlot(QuickKind kind, const char* key) {
	const Inventory& inventory = *Game().ui.inventory;
	PlayerHud::Slot slot;
	slot.key = key;
	if (std::optional<ItemKind> potion = inventory.QuickChoice(kind)) {
		slot.icon = PlayerHud::Icon::Potion;
		slot.tint = Inventory::PotionColor(*potion);
		slot.count = inventory.Count(*potion);
	}
	if (std::optional<int> drunk = inventory.QuickDrinkMs(kind))
		slot.flashAgeMs = GameClock::now() - *drunk;
	return slot;
}

PlayerHud::Icon weaponIcon(const Inventory& inventory) {
	switch (inventory.EquippedKind()) {
	case ItemKind::Sword:
		return PlayerHud::Icon::Sword;
	case ItemKind::Spear:
		return PlayerHud::Icon::Spear;
	case ItemKind::Bow:
		return PlayerHud::Icon::Bow;
	default:
		return PlayerHud::Icon::Club;
	}
}

} // namespace

PlayerHud::View playerHudView() {
	const PlayerStats& stats = Game().player->stats;
	PlayerHud::View view;
	view.hp = stats.CurrentHP();
	view.maxHp = stats.CurrentMaxHP();
	view.stamina = stats.Stamina();
	view.maxStamina = stats.MaxStamina();
	if (std::optional<int> refused = stats.StaminaRefusedMs())
		view.staminaRefusedAgeMs = GameClock::now() - *refused;
	view.xpRatio = stats.LevelProgress();
	view.keysHeld = Game().dungeon.KeysHeld();
	view.levelKeys = Game().dungeon.LevelKeys();
	view.slots[0].icon = weaponIcon(*Game().ui.inventory);
	view.slots[0].key = EQUIP_KEYS_LABEL;
	view.slots[1] = quickSlot(QuickKind::Health, QUICK_HEAL_KEY_LABEL);
	view.slots[2] = quickSlot(QuickKind::Stamina, QUICK_STAMINA_KEY_LABEL);
	return view;
}
