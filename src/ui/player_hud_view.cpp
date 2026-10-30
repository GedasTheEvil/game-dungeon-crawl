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
	if (std::optional<ItemKind> worn = Game().ui.inventory->Bag().Worn()) {
		constexpr const char* TIERS[AMULET_TIER_COUNT] = {"I", "II", "III", "IV"};
		const Amulet amulet = amuletOf(*worn);
		view.slots[3].icon = PlayerHud::amuletIcon(amulet.type);
		view.slots[3].badge = TIERS[static_cast<size_t>(amulet.tier)];
	}
	for (int hand = 0; hand < HAND_COUNT; hand++)
		if (std::optional<ItemKind> ring = Game().ui.inventory->Bag().RingOn(static_cast<Hand>(hand))) {
			const Rgb c = ringGemColour(ringOf(*ring).gem);
			view.ringWorn[hand] = true;
			view.ringGem[hand] = {c.r, c.g, c.b};
		}
	for (int t = 0; t < POISON_TIER_COUNT; t++)
		view.poisonLeftMs[t] = stats.poison.LeftMs(static_cast<PoisonTier>(t));
	view.resistLeftMs = stats.PotionResistLeftMs();
	view.waterLeftMs = stats.WaterBlessingLeftMs();
	view.resistGreater = stats.PotionResistPercent() >= PotionEffect::GREATER_RESIST_PERCENT;
	for (int c = 0; c < CURSE_COUNT; c++) {
		view.curseLeftMs[c] = stats.curses.LeftMs(static_cast<Curse>(c));
		view.curseGreater[c] = stats.curses.Greater(static_cast<Curse>(c));
	}
	view.dazeLeftMs = Game().player->dazeMs;
	view.staminaSkillShown = stats.heka.HasStaminaSkill();
	view.staminaSkillOn = Game().player->staminaSkills;
	if (std::optional<int> refused = Game().player->staminaSkillRefusedMs)
		view.staminaSkillRefusedAgeMs = GameClock::now() - *refused;
	view.staminaSkillKey = keyCapOf(BindAction::StaminaSkills);
	view.hekaWaiting = stats.heka.Free() > 0;
	view.hekaKey = keyCapOf(BindAction::Skills);
	return view;
}
