#include "../../external/doctest/doctest.h"
#include "../../src/core/timer.h"
#include "../../src/entities/player_stats.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
// The stamina timers read the game clock: a virtual one, moved by hand. Enable it before a PlayerStats is made.
PlayerStats freshStats() {
	GameClock::enableVirtual();
	return PlayerStats{};
}

// One update tick after `ms` of game time, the player walking (or not).
void tick(PlayerStats& stats, WorldEvents& events, int ms, bool walked) {
	GameClock::advance(ms);
	if (walked)
		stats.NoteWalked();
	stats.UpdateStamina(events);
}
} // namespace

TEST_CASE("a level up raises max HP, heals fully, cures poison and tells the app") {
	PlayerStats stats = freshStats();
	WorldEvents events;
	stats.LoseHP(30);
	stats.SetStamina(10);
	stats.poison.Apply(PoisonTier::Weak);
	stats.AddXP(999, events);
	CHECK(stats.CurrentLevel() == 1);
	CHECK(events.Take().empty());

	stats.AddXP(1, events);
	CHECK(stats.CurrentLevel() == 2);
	CHECK(stats.CurrentMaxHP() == 50 + HP_PER_LEVEL);
	CHECK(stats.CurrentHP() == stats.CurrentMaxHP());
	CHECK(stats.Stamina() == stats.MaxStamina());
	CHECK(stats.MaxStamina() == 110);
	CHECK_FALSE(stats.poison.Any());
	CHECK(stats.LevelUpMs().has_value());
	const std::vector<WorldEvent> list = events.Take();
	REQUIRE(list.size() == 2);
	CHECK(list[0].text == "Now you are level 2");
	CHECK(list[1].note == FieldNote::Levels);
}

TEST_CASE("one big XP gain climbs several levels: armour every 5, might every 8") {
	PlayerStats stats = freshStats();
	WorldEvents events;
	stats.AddXP(static_cast<int>(PlayerStats::LevelXP(8)) + 1, events);
	CHECK(stats.CurrentLevel() == 8);
	CHECK(stats.CurrentArmor() == 1);
	CHECK(stats.CurrentMight() == 1);
	CHECK(stats.CurrentMaxHP() == 50 + 7 * HP_PER_LEVEL);
	CHECK(events.Take().size() == 14); // a status line and a note per level
}

TEST_CASE("healing is a share of max HP, capped at the max") {
	PlayerStats stats = freshStats();
	stats.LoseHP(40);
	stats.Heal(20);
	CHECK(stats.CurrentHP() == 20);
	stats.Heal(100);
	CHECK(stats.CurrentHP() == 50);
	CHECK(stats.HealthRatio() == doctest::Approx(1.f));
	stats.LoseHP(60);
	CHECK_FALSE(stats.Alive());
	CHECK(stats.HealthRatio() == doctest::Approx(0.f));
}

TEST_CASE("stamina is spent only when there is enough") {
	PlayerStats stats = freshStats();
	CHECK(stats.ConsumeStamina(30));
	CHECK(stats.Stamina() == 70);
	CHECK_FALSE(stats.ConsumeStamina(71));
	CHECK(stats.Stamina() == 70);
	CHECK(stats.ConsumeStamina(0));
	stats.AddStamina(500);
	CHECK(stats.Stamina() == 100);
	stats.SetStamina(-5);
	CHECK(stats.Stamina() == 0);

	WorldEvents events;
	stats.RefuseStamina(events);
	CHECK(stats.StaminaRefusedMs() == GameClock::now());
	const std::vector<WorldEvent> list = events.Take();
	REQUIRE(list.size() == 1);
	CHECK(list[0].note == FieldNote::Stamina);
}

TEST_CASE("sprinting drains 5% of max stamina a second, standing still regenerates it") {
	PlayerStats stats = freshStats();
	WorldEvents events;
	stats.SetSprintRequested(true);
	CHECK(stats.SprintMoveMultiplier() == doctest::Approx(3.f));
	tick(stats, events, 1000, true);
	CHECK(stats.IsSprinting());
	CHECK(stats.Stamina() == 95);
	tick(stats, events, 500, true);
	CHECK(stats.Stamina() == 95); // the drain timer is not up yet
	tick(stats, events, 500, true);
	CHECK(stats.Stamina() == 90);

	// Shift held against a wall: no walk step, so no drain and no refusal.
	tick(stats, events, 1000, false);
	CHECK_FALSE(stats.IsSprinting());
	CHECK(stats.Stamina() == 95);
	CHECK(events.Take().empty());

	stats.SetSprintRequested(false);
	tick(stats, events, 1000, true);
	CHECK(stats.Stamina() == 100);
}

TEST_CASE("sprinting with no stamina left is refused and walks at normal speed") {
	PlayerStats stats = freshStats();
	WorldEvents events;
	stats.SetStamina(0);
	stats.SetSprintRequested(true);
	CHECK(stats.SprintMoveMultiplier() == doctest::Approx(1.f));
	tick(stats, events, 10, true);
	CHECK_FALSE(stats.IsSprinting());
	CHECK(stats.StaminaRefusedMs().has_value());
}

TEST_CASE("a hit on the player: resistances, then armour, at least 1") {
	PlayerStats stats = freshStats();
	WorldEvents events;
	stats.AddArmor(3);
	const DamageMix blunt = {100, 0, 0};
	CHECK(stats.HitDamage(10, blunt, false) == 7);
	CHECK(stats.HitDamage(10, blunt, true) == 10);
	CHECK(stats.HitDamage(2, blunt, false) == 1);
	stats.resist = {50, 100, 100};
	CHECK(stats.HitDamage(10, blunt, false) == 2);
	CHECK(stats.Damage(10) == 10);
	stats.AddMight(2);
	CHECK(stats.Damage(10) == 12);
}

TEST_CASE("an amulet adds to might, armour and max HP; the HP keeps its share") {
	PlayerStats stats = freshStats();
	stats.LoseHP(25); // 25 of 50
	AmuletBonus bonus;
	bonus.maxHpPercent = 20;
	bonus.might = 2;
	bonus.armor = 1;
	stats.Wear(bonus, true);
	CHECK(stats.CurrentMaxHP() == 60);
	CHECK(stats.CurrentHP() == 30);
	CHECK(stats.CurrentMight() == 2);
	CHECK(stats.CurrentArmor() == 1);
	stats.Wear(AmuletBonus{}, true);
	CHECK(stats.CurrentMaxHP() == 50);
	CHECK(stats.CurrentHP() == 25);
	CHECK(stats.CurrentMight() == 0);

	stats.Wear(bonus, false); // after a load: the HP is already the worn one's
	CHECK(stats.CurrentHP() == 25);
}

TEST_CASE("a trap amulet cuts trap damage; the hundredths carry to the next hit") {
	PlayerStats stats = freshStats();
	CHECK(stats.TrapDamage(3) == 3);
	AmuletBonus bonus;
	bonus.trapCutPercent = 50;
	stats.Wear(bonus, true);
	CHECK(stats.TrapDamage(3) == 1);
	CHECK(stats.TrapDamage(3) == 2);
	bonus.trapCutPercent = 100;
	stats.Wear(bonus, true);
	CHECK(stats.TrapDamage(40) == 0);
}

TEST_CASE("a regeneration amulet heals only while safe and not poisoned") {
	PlayerStats stats = freshStats();
	stats.LoseHP(10);
	stats.Regenerate(true, 1000);
	CHECK(stats.CurrentHP() == 40); // no amulet
	AmuletBonus bonus;
	bonus.regenHpPerSecond = 2;
	stats.Wear(bonus, true);
	stats.Regenerate(true, 250);
	CHECK(stats.CurrentHP() == 40);
	stats.Regenerate(true, 250);
	CHECK(stats.CurrentHP() == 41);
	stats.Regenerate(false, 1000);
	CHECK(stats.CurrentHP() == 41);
	stats.poison.Apply(PoisonTier::Weak);
	stats.Regenerate(true, 1000);
	CHECK(stats.CurrentHP() == 41);
	stats.poison.Cure();
	stats.Regenerate(true, 10000);
	CHECK(stats.CurrentHP() == 50);
}

TEST_CASE("a potion's gain and its status line") {
	PlayerStats stats = freshStats();
	stats.LoseHP(20);
	PotionGain heal;
	heal.healPercent = 30;
	CHECK(stats.Drink(heal) == "Healed 15 health");
	PotionGain might;
	might.might = 1;
	CHECK(stats.Drink(might) == "Might rises to 1");
	PotionGain armor;
	armor.armor = 2;
	CHECK(stats.Drink(armor) == "Armor rises to 2");
	PotionGain cure;
	cure.cure = true;
	stats.poison.Apply(PoisonTier::Weak);
	CHECK(stats.Drink(cure) == "The poison is gone");
	CHECK_FALSE(stats.poison.Any());
	PotionGain maxHp;
	maxHp.maxHpPercent = 10;
	CHECK(stats.Drink(maxHp) == "Max health rises to 55");
	CHECK(stats.CurrentHP() == 55);
	stats.SetStamina(50);
	PotionGain stamina;
	stamina.staminaPercent = 25;
	CHECK(stats.Drink(stamina) == "Restored 25 stamina");
}

TEST_CASE("a resistance potion adds to the amulet's ward for 2 minutes, up to 100%") {
	PlayerStats stats = freshStats();
	AmuletBonus amulet;
	amulet.poisonResistPercent = 20;
	stats.Wear(amulet, true);
	PotionGain lesser;
	lesser.resistPercent = 50;
	stats.poison.Apply(PoisonTier::Weak);
	CHECK(stats.Drink(lesser) == "Resists poison 70% for 2 min");
	CHECK(stats.poison.Any()); // the lesser one does not cure
	CHECK(stats.PoisonResistPercent() == 70);
	stats.AdvanceResistance(PotionEffect::RESIST_MS - 1);
	CHECK(stats.PoisonResistPercent() == 70);
	stats.AdvanceResistance(1);
	CHECK(stats.PoisonResistPercent() == 20);
	PotionGain greater;
	greater.cure = true;
	greater.resistPercent = 95;
	CHECK(stats.Drink(greater) == "Cured. Resists poison 100% for 2 min");
	CHECK_FALSE(stats.poison.Any());
	CHECK(stats.PotionResistPercent() == 95);
	stats.EndResistance();
	CHECK(stats.PoisonResistPercent() == 20);
}

TEST_CASE("the stats survive a save and a load") {
	PlayerStats stats = freshStats();
	WorldEvents events;
	stats.AddXP(5000, events);
	stats.LoseHP(7);
	stats.SetStamina(33);
	stats.poison.Apply(PoisonTier::Weak);
	PotionGain lesser;
	lesser.resistPercent = 50;
	stats.Drink(lesser);
	stats.AdvanceResistance(1000);
	const std::filesystem::path path = std::filesystem::temp_directory_path() / "player_stats_test.sav";
	{
		std::ofstream out(path);
		stats.Dump(out);
	}
	PlayerStats loaded = freshStats();
	{
		std::ifstream in(path);
		loaded.LoadDump(in);
	}
	std::filesystem::remove(path);
	CHECK(loaded.CurrentLevel() == stats.CurrentLevel());
	CHECK(loaded.CurrentXP() == doctest::Approx(stats.CurrentXP()));
	CHECK(loaded.CurrentHP() == stats.CurrentHP());
	CHECK(loaded.CurrentMaxHP() == stats.CurrentMaxHP());
	CHECK(loaded.CurrentArmor() == stats.CurrentArmor());
	CHECK(loaded.Stamina() == 33);
	CHECK(loaded.poison.Any());
	CHECK(loaded.PotionResistPercent() == 50);
	CHECK(loaded.PotionResistLeftMs() == PotionEffect::RESIST_MS - 1000);
}
