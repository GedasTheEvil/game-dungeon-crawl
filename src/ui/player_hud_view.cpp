#include "player_hud_view.h"
#include "inventory.h"
#include "../input/input_actions.h"
#include "../core/timer.h"
#include "../state/game_state.h"
#include <optional>

namespace {
// A quick slot: the potion its key would drink now, empty when none of that kind is left.
PlayerHud::Slot quickSlot(QuickKind kind, const std::string& key) {
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

// The antidote's slot (plan hud-antidote-and-level): its count, empty when none is left; the HUD blinks it while the
// player is poisoned and has one.
PlayerHud::Slot antidoteSlot() {
	const Inventory& inventory = *Game().ui.inventory;
	PlayerHud::Slot slot;
	slot.key = keyCapOf(BindAction::QuickAntidote);
	if (const int count = inventory.Count(ItemKind::Antidote); count > 0) {
		slot.icon = PlayerHud::Icon::Potion;
		slot.tint = Inventory::PotionColor(ItemKind::Antidote);
		slot.count = count;
	}
	if (std::optional<int> drunk = inventory.AntidoteMs())
		slot.flashAgeMs = GameClock::now() - *drunk;
	return slot;
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
	view.level = stats.CurrentLevel();
	view.keysHeld = Game().dungeon.KeysHeld();
	view.levelKeys = Game().dungeon.LevelKeys();
	view.slots[0].icon = PlayerHud::weaponIcon(Game().ui.inventory->EquippedKind());
	view.slots[0].key = equipKeysCap();
	if (const ItemKind weapon = Game().ui.inventory->EquippedKind(); isRanged(weapon)) {
		const ItemBag& bag = Game().ui.inventory->Bag();
		view.slots[0].shots = stats.Shots(weapon);
		view.slots[0].shotsLeft = bag.ShotsLeft(weapon, view.slots[0].shots);
		view.slots[0].shotRecovery = bag.ShotRecovery(weapon, stats.ShotRecoveryPace());
	}
	if (const std::optional<int> refused = Game().player->shotRefusedMs) {
		view.slots[0].shotRefusedAgeMs = GameClock::now() - *refused;
		view.slots[0].shotTooClose = Game().player->shotTooClose;
	}
	view.slots[1] = quickSlot(QuickKind::Health, keyCapOf(BindAction::QuickHeal));
	view.slots[2] = quickSlot(QuickKind::Stamina, keyCapOf(BindAction::QuickStamina));
	view.slots[PlayerHud::ANTIDOTE_SLOT] = antidoteSlot();
	if (std::optional<ItemKind> worn = Game().ui.inventory->Bag().Worn()) {
		constexpr const char* TIERS[AMULET_TIER_COUNT] = {"I", "II", "III", "IV"};
		const Amulet amulet = amuletOf(*worn);
		view.slots[PlayerHud::AMULET_SLOT].icon = PlayerHud::amuletIcon(amulet.type);
		view.slots[PlayerHud::AMULET_SLOT].badge = TIERS[static_cast<size_t>(amulet.tier)];
	}
	for (int hand = 0; hand < HAND_COUNT; hand++)
		if (std::optional<ItemKind> ring = Game().ui.inventory->Bag().RingOn(static_cast<Hand>(hand))) {
			const Rgb c = ringGemColour(ringOf(*ring).gem);
			view.ringWorn[hand] = true;
			view.ringGem[hand] = {c.r, c.g, c.b};
		}
	view.statuses = statusList(stats, Game().player->burning.LeftMs(), Game().player->dazeMs);
	view.poisoned = stats.poison.Any();
	view.staminaSkillShown = stats.heka.HasStaminaSkill();
	view.staminaSkillOn = Game().player->staminaSkills;
	if (std::optional<int> refused = Game().player->staminaSkillRefusedMs)
		view.staminaSkillRefusedAgeMs = GameClock::now() - *refused;
	view.staminaSkillKey = keyCapOf(BindAction::StaminaSkills);
	view.hekaWaiting = stats.heka.Free() > 0;
	view.hekaKey = keyCapOf(BindAction::Skills);
	view.journalNew = Game().journal.AnyNew();
	view.journalKey = keyCapOf(BindAction::Journal);
	return view;
}
