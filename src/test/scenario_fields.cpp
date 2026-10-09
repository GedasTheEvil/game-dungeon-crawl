#include "scenario_fields.h"
#include "../state/game_state.h"
#include "../ui/inventory.h"
#include <algorithm>
#include <optional>

using namespace Scenario;

float Scenario::fieldValue(const Command& cmd) {
	Field field = cmd.field;
	float x = 0.f;
	float y = 0.f;
	Game().dungeon.getC(x, y);
	switch (field) {
	case Field::X:
		return x;
	case Field::Y:
		return y;
	case Field::Hp:
		return static_cast<float>(Game().player->stats.CurrentHP());
	case Field::Stamina:
		return static_cast<float>(Game().player->stats.Stamina());
	case Field::Level:
		return static_cast<float>(Game().dungeon.LevelNumber());
	case Field::Alive:
		return Game().player->Alive() ? 1.f : 0.f;
	case Field::Won:
		return Game().dungeon.Won() ? 1.f : 0.f;
	case Field::Might:
		return static_cast<float>(Game().player->stats.CurrentMight());
	case Field::Armor:
		return static_cast<float>(Game().player->stats.CurrentArmor());
	case Field::EquipType:
		return static_cast<float>(fileIdOf(Game().ui.inventory->EquippedKind()).type);
	case Field::EquipId:
		return static_cast<float>(fileIdOf(Game().ui.inventory->EquippedKind()).id);
	case Field::Keys:
		return static_cast<float>(Game().dungeon.KeysHeld());
	case Field::Poison:
		return static_cast<float>(Game().player->stats.poison.Mask());
	case Field::Resist:
		return static_cast<float>(Game().player->stats.PoisonResistPercent());
	case Field::XpTotal:
		return static_cast<float>(Game().player->stats.CurrentXP());
	case Field::Riddle:
		return Game().ui.screen == Screen::Riddle ? 1.f : 0.f;
	case Field::Bars:
		return static_cast<float>(Game().dungeon.MonsterBarsShown());
	case Field::Boss:
		return static_cast<float>(Game().dungeon.BossHealth());
	case Field::Minions:
		return static_cast<float>(Game().dungeon.LivingMinions());
	case Field::DecorTier:
		return static_cast<float>(Game().dungeon.DecorTierUsed());
	case Field::Attacking: // a swing or a bow draw under way
		return Game().player->attackStartMs >= 0 ? 1.f : 0.f;
	case Field::Nearest:
		return static_cast<float>(Game().dungeon.NearestMonsterHealth());
	case Field::NearestPoison:
		return static_cast<float>(Game().dungeon.NearestMonsterPoison());
	case Field::Coffins:
		return static_cast<float>(Game().dungeon.CoffinCount());
	case Field::Chests:
		return static_cast<float>(Game().dungeon.ChestCount());
	case Field::JournalRiddles:
		return static_cast<float>(Game().journal.Riddles().size());
	case Field::JournalNotes:
		return static_cast<float>(Game().journal.Notes().size());
	case Field::MaxHp:
		return static_cast<float>(Game().player->stats.CurrentMaxHP());
	case Field::Worn: {
		const std::optional<ItemKind> worn = Game().ui.inventory->Bag().Worn();
		return worn ? static_cast<float>(fileIdOf(*worn).id) : -1.f;
	}
	case Field::Safe:
		return Game().dungeon.PlayerSafe() ? 1.f : 0.f;
	case Field::Tab:
		return static_cast<float>(Game().ui.inventory->OpenTab());
	case Field::Selected:
		return static_cast<float>(Game().ui.inventory->SelectedPosition());
	case Field::JournalTried: {
		int known = 0;
		for (const JournalCreature& c : Game().journal.Creatures()) {
			for (int d = 0; d < DAMAGE_TYPE_COUNT; d++)
				known += c.Tried(static_cast<DamageType>(d)) ? 1 : 0;
			known += c.TriedPoison() ? 1 : 0;
		}
		return static_cast<float>(known);
	}
	case Field::JournalSolved: {
		const auto& riddles = Game().journal.Riddles();
		return static_cast<float>(
			std::count_if(riddles.begin(), riddles.end(), [](const JournalRiddle& r) { return r.solved; }));
	}
	case Field::ItemCount:
		return static_cast<float>(Game().ui.inventory->Count(cmd.item));
	case Field::ItemLevel:
		return static_cast<float>(Game().ui.inventory->Level(cmd.item));
	}
	return 0.f;
}
